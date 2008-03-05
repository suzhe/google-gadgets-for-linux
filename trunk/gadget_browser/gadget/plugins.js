/*
  Copyright 2007 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

var kPluginDownloadURLPrefix = "http://desktop.google.com";

// gPlugins is a two-level table. It is first indexed with language names,
// then with category names. The elements of the table are arrays of plugins.
// For example, gPlugins may contain the following value:
// {en:{news:[plugin1,plugin2,...],...}}.
var gPlugins = {};
var gPluginIdIndex = {};

// Plugin updated in recent two months are listed in the "new" category.
var kNewPluginPeriod = 2 * 30 * 86400000;
var kIGoogleFeedModuleId = 25;

var kSupportedLanguages = [
  "bg", "ca", "cs", "da", "de", "el", "en", "en-gb", "es", "fi", "fr", "hi",
  "hr", "hu", "id", "it", "ja", "ko", "nl", "no", "pl", "pt-br", "pt-pt",
  "ro", "ru", "sk", "sl", "sv", "th", "tr", "zh-cn", "zh-tw",
];

var kPluginTypeSearch = "search";
var kPluginTypeSidebar = "sidebar";
var kPluginTypeIGoogle = "igoogle";
var kPluginTypeFeed = "feed";

var kCategoryAll = "all";
var kCategoryNew = "new";
var kCategoryRecommendations = "recommendations";
var kCategoryGoogle = "google";
var kCategoryRecentlyUsed = "recently_used";
var kCategoryUpdates = "updates";

var kCategoryNews = "news";
var kCategorySports = "sports";
var kCategoryLifestyle = "lifestyle";
var kCategoryTools = "tools";
var kCategoryFinance = "finance";
var kCategoryFunGames = "fun_games";
var kCategoryTechnology = "technology";
var kCategoryCommunication = "communication";
var kCategoryHoliday = "holiday";

var kTopCategories = [
  kCategoryAll, kCategoryNew, kCategoryRecommendations, kCategoryGoogle,
  kCategoryRecentlyUsed, kCategoryUpdates,
];

var kBottomCategories = [
  kCategoryNews, kCategorySports, kCategoryLifestyle, kCategoryTools,
  kCategoryFinance, kCategoryFunGames, kCategoryTechnology,
  kCategoryCommunication, kCategoryHoliday,
];

function SplitValues(src) {
  return src ? src.split(",") : [];
}

function LoadMetadata() {
  debug.trace("begin loading metadata");
  var metadata = gadgetBrowserUtils.gadgetMetadata;
  gPlugins = {};
  var currentTime = new Date().getTime();
  for (var i = 0; i < metadata.length; i++) {
    var plugin = metadata[i];
    gPluginIdIndex[plugin.id] = plugin;
    plugin.download_status = kDownloadStatusNone;

    var attrs = plugin.attributes;
    if ((attrs.module_id != kIGoogleFeedModuleId ||
         attrs.type != kPluginTypeIGoogle) &&
        attrs.sidebar == "false") {
      // This plugin is not for desktop, ignore it.
      // IGoogle feeding gadgets are supported even if marked sidebar="false".
      continue;
    }

    var languages = SplitValues(attrs.language);
    if (languages.length == 0)
      languages.push("en");
    var categories = SplitValues(attrs.category);
    // TODO: other special categories.
    plugin.rank = parseFloat(attrs.rank);

    for (var lang_i in languages) {
      var language = languages[lang_i];
      var language_categories = gPlugins[language];
      if (!language_categories) {
        language_categories = gPlugins[language] = {};
        language_categories[kCategoryAll] = [];
        language_categories[kCategoryNew] = [];
        language_categories[kCategoryRecentlyUsed] = [];
        language_categories[kCategoryUpdates] = [];
        // TODO: recommendations.
      }

      language_categories[kCategoryAll].push(plugin);
      if (currentTime - plugin.updated_date.getTime() < kNewPluginPeriod)
        language_categories[kCategoryNew].push(plugin);

      for (var cate_i in categories) {
        var category = categories[cate_i];
        var category_plugins = language_categories[category];
        if (!category_plugins)
          category_plugins = language_categories[category] = [];
        category_plugins.push(plugin);
      }
    }
  }
  debug.trace("finished loading metadata");
}

function GetPluginTitle(plugin) {
  if (!plugin) return "";
  var result = plugin.titles[gCurrentLanguage];
  return result ? result : plugin.attributes.name;
}

function GetPluginDescription(plugin) {
  if (!plugin) return "";
  var result = plugin.descriptions[gCurrentLanguage];
  return result ? result : plugin.attributes.product_summary;
}

function GetPluginOtherData(plugin) {
  if (!plugin) return "";
  var result = "";
  var attrs = plugin.attributes;
  if (attrs.version) {
    result += strings.PLUGIN_VERSION;
    result += attrs.version;
  }
  if (attrs.size_kilobytes) {
    if (result) result += strings.PLUGIN_DATA_SEPARATOR;
    result += attrs.size_kilobytes;
    result += strings.PLUGIN_KILOBYTES;
  }
  if (plugin.updated_date.getTime()) {
    if (result) result += strings.PLUGIN_DATA_SEPARATOR;
    result += plugin.updated_date.toLocaleDateString();
  }
  if (attrs.author) {
    if (result) result += strings.PLUGIN_DATA_SEPARATOR;
    result += attrs.author;
  }
  if (result)
    result = strings.PLUGIN_DATA_BULLET + result;
  // For debug:
  // result += "/" + attrs.uuid;
  // result += "/" + attrs.id;
  // result += "/" + attrs.rank;
  return result;
}

function DownloadPlugin(plugin, is_updating) {
  var download_url = plugin.attributes.download_url;
  if (!download_url) {
    SetDownloadStatus(plugin, kDownloadStatusError);
    return;
  }

  if (!download_url.match(/^https?:\/\//))
    download_url = kPluginDownloadURLPrefix + download_url;
  gadget.debug.trace("Start downloading gadget: " + download_url);

  var request = new XMLHttpRequest();
  try {
    request.open("GET", download_url);
    request.onreadystatechange = function() {
      OnRequestStateChange();
    };
    request.send();
  } catch (e) {
    gadget.debug.error("Request error: " + e);
  }

  function OnRequestStateChange() {
    if (request.readyState == 4) {
      if (request.status == 200) {
        gadget.debug.trace("Finished downloading a gadget: " + download_url);
        // gPlugins may have been updated during the download, so re-get the
        // plugin with its id.
        plugin = gPluginIdIndex[plugin.id];
        if (plugin) {
          var result = -1;
          if (gadgetBrowserUtils.saveGadget(plugin.id, request.responseBody)) {
            if (!is_updating)
              result = gadgetBrowserUtils.addGadget(plugin.id);
          }

          if (result >= 0)
            SetDownloadStatus(plugin, kDownloadStatusAdded);
          else
            SetDownloadStatus(plugin, kDownloadStatusError);
        }
      } else {
        gadget.debug.error("Download request " + download_url +
                           " returned status: " + request.status);
        SetDownloadStatus(plugin, kDownloadStatusError);
      }
    }
  }
}

function SearchPlugins(search_string) {
  debug.trace("search begins");
  var terms = search_string.toUpperCase().split(" ");
  var plugins = gPlugins[gCurrentLanguage][kCategoryAll];
  var result = [];
  if (plugins && plugins.length) {
    for (var i = 0; i < plugins.length; i++) {
      var plugin = plugins[i];
      var attrs = plugin.attributes;
      var indexable_text = [
        attrs.author ? attrs.author : "",
        plugin.updated_date ? plugin.updated_date.toLocaleDateString() : "",
        attrs.download_url ? attrs.download_url : "",
        attrs.info_url ? attrs.info_url : "",
        attrs.keywords ? attrs.keywords : "",
        GetPluginTitle(plugin),
        GetPluginDescription(plugin),
      ].join(" ").toUpperCase();

      var ok = true;
      for (var j in terms) {
        if (terms[j] && indexable_text.indexOf(terms[j]) == -1) {
          ok = false;
          break;
        }
      }
      if (ok) result.push(plugin);
    }
  }

  debug.trace("search ends");
  return result;
}
