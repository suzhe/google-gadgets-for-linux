/*
  Copyright 2008 Google Inc.

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
function init() {
  if (LoadMetadata()) {
    UpdateLanguageBox();
    main_ui.visible = true;
  } else {
    updating.visible = true;
    gadgetBrowserUtils.onMetadataUpdated = function(success) {
      updating.visible = false;
      gadgetBrowserUtils.onMetadataUpdated = null;
      if (success && LoadMetadata()) {
        UpdateLanguageBox();
        main_ui.visible = true;
      } else {
        update_fail.visible = true;
      }
    };
    gadgetBrowserUtils.updateMetadata(true);
  }
}

function view_onopen() {
  // We do the init in timer because gadgetBrowserUtils is not ready when
  // gadget is created and will be registered by c++ code right after the
  // gadget is created.
  setTimeout(init, 0);
}

function view_onclose() {
  ClearThumbnailTasks();
  ClearPluginDownloadTasks();
}

// Disable context menus.
function view_oncontextmenu() {
  event.returnValue = false;
}

function language_label_onsize() {
  language_box_div.x = language_label.offsetX +
                       language_label.offsetWidth + 7;
}

function plugin_description_onsize() {
  var max_height = 2 * plugin_title.offsetHeight;
  if (plugin_description.offsetHeight > max_height) {
    plugin_description.height = max_height;
    plugin_other_data.y = plugin_description.offsetY + max_height;
  } else {
    plugin_other_data.y = plugin_description.offsetY +
                          plugin_description.offsetHeight + 2;
  }
}

var gPageButtonAdjusted = false;
function page_label_onsize() {
  if (!gPageButtonAdjusted && page_label.innerText) {
    page_label.width = page_label.offsetWidth + 20;
    page_label.x = next_button.x - page_label.width;
    previous_button.x = page_label.x - previous_button.offsetWidth;
    gPageButtonAdjusted = true;
  }
}

function previous_button_onclick() {
  if (gCurrentPage > 0)
    SelectPage(gCurrentPage - 1);
}

function next_button_onclick() {
  if (gCurrentPage < GetTotalPages() - 1)
    SelectPage(gCurrentPage + 1);
}

function language_box_onchange() {
  if (!gUpdatingLanguageBox)
    SelectLanguage(language_box.children.item(language_box.selectedIndex).name);
}

var gCurrentCategory = "";
var gCurrentLanguage = "";
var gCurrentPage = 0;
var gCurrentPlugins = [];

// UI constants.
var kCategoryButtonHeight = category_active_img.height;
var kCategoryGap = 15;
var kPluginWidth = 134;
var kPluginHeight = 124;
var kPluginRows = 3;
var kPluginColumns = 4;
var kPluginsPerPage = kPluginRows * kPluginColumns;

// Plugin download status.
var kDownloadStatusNone = 0;
var kDownloadStatusAdding = 1;
var kDownloadStatusAdded = 2;
var kDownloadStatusError = 3;

function GetDisplayLanguage(language) {
  return strings["LANGUAGE_" + language.replace("-", "_").toUpperCase()];
}

function GetDisplayCategory(category) {
  return strings["CATEGORY_" + category.toUpperCase()];
}

var gUpdatingLanguageBox = false;
function UpdateLanguageBox() {
  gUpdatingLanguageBox = true;
  var default_language = framework.system.languageCode().toLowerCase();
  if (!default_language || !gPlugins[default_language] ||
      !GetDisplayLanguage(default_language))
    default_language = "en";
  gadget.debug.trace("Default language: " + default_language);

  language_box.removeAllElements();
  var languages = [];
  for (var language in gPlugins) {
    var disp_lang = GetDisplayLanguage(language);
    if (disp_lang)
      languages.push({lang: language, disp: disp_lang});
  }
  languages.sort(function(a, b) { return a.disp.localeCompare(b.disp); });
  for (var i = 0; i < languages.length; i++) {
    var language = languages[i].lang;
    language_box.appendElement(
      "<item name='" + language +
      "'><label vAlign='middle' size='10'>" + languages[i].disp +
      "</label></item>");

    if (default_language == language)
      language_box.selectedIndex = language_box.children.count - 1;
  }
  SelectLanguage(default_language);
  gUpdatingLanguageBox = false;
}

function SelectLanguage(language) {
  gCurrentLanguage = language;
  UpdateCategories();
}

function AddCategoryButton(category, y) {
  categories_div.appendElement(
    "<label align='left' vAlign='middle' enabled='true' color='#FFFFFF' name='" +
    category +
    "' size='10' width='100%' height='" + kCategoryButtonHeight + "' y='" + y +
    "' onmouseover='category_onmouseover()' onmouseout='category_onmouseout()'" +
    " onclick='SelectCategory(\"" + category + "\")'>&#160;" +
    GetDisplayCategory(category) + "</label>");
}

function category_onmouseover() {
  category_hover_img.y = event.srcElement.offsetY;
  category_hover_img.visible = true;
}

function category_onmouseout() {
  category_hover_img.visible = false;
}

function SelectCategory(category) {
  gCurrentCategory = category;
  if (category) {
    category_active_img.y = categories_div.children.item(category).offsetY;
    category_active_img.visible = true;
    gCurrentPlugins = GetPluginsOfCategory(gCurrentLanguage, gCurrentCategory);
    SelectPage(0);
    ResetSearchBox();
  } else {
    category_active_img.visible = false;
    gCurrentPlugins = [];
  }
}

function AddPluginBox(plugin, index, row, column) {
  var box = plugins_div.appendElement(
    "<div x='" + (column * kPluginWidth) + "' y='" + (row * kPluginHeight) +
    "' width='" + kPluginWidth + "' height='" + kPluginHeight + "' enabled='true'" +
    " onmouseover='pluginbox_onmouseover(" + index + ")'" +
    " onmouseout='pluginbox_onmouseout(" + index + ")'>" +
    " <img width='100%' height='100%' stretchMiddle='true'/>" +
    " <label x='7' y='6' size='10' width='120' align='center' color='#FFFFFF' " +
    "  trimming='character-ellipsis'/>" +
    " <img x='16' y='75' opacity='70' src='images/thumbnails_shadow.png'/>" +
    " <div x='27' y='33' width='80' height='83' background='#FFFFFF'>" +
    "  <img width='80' height='60' src='images/default_thumbnail.jpg' cropMaintainAspect='true'/>" +
    "  <img y='60' width='80' height='60' src='images/default_thumbnail.jpg' flip='vertical' cropMaintainAspect='true'/>" +
    "  <img src='images/thumbnails_default_mask.png'/>" +
    " </div>" +
    " <button x='22' y='94' width='90' height='30' visible='false' color='#FFFFFF'" +
    "  size='10' stretchMiddle='true' trimming='character-ellipsis'" +
    "  downImage='images/add_button_down.png' overImage='images/add_button_hover.png'" +
    "  onmousedown='add_button_onmousedown(" + index + ")'" +
    "  onmouseover='add_button_onmouseover(" + index + ")'" +
    "  onmouseout='add_button_onmouseout(" + index + ")'" +
    "  onclick='add_button_onclick(" + index + ")'/>" +
    "</div>");

  // Set it here to prevent problems caused by special chars in the title.
  box.children.item(1).innerText = GetPluginTitle(plugin, gCurrentLanguage);

  var thumbnail_element1 = box.children.item(3).children.item(0);
  var thumbnail_element2 = box.children.item(3).children.item(1);
  if (plugin.source == 1) { // built-in gadgets
    thumbnail_element1.src = plugin.attributes.thumbnail_url;
    thumbnail_element2.src = plugin.attributes.thumbnail_url;
  } else if (plugin.source == 2) { // from plugins.xml
    AddThumbnailTask(plugin, index, thumbnail_element1, thumbnail_element2);
  }

  plugin.button = box.children.item(4);
  UpdateAddButtonVisualStatus(plugin);
}

function SetDownloadStatus(plugin, status) {
  plugin.download_status = status;
  var button = plugin.button;
  if (!button)
    return;

  try {
    // Test if the button has been destroyed.
    button.enabled = button.enabled;
  } catch (e) {
    plugin.button = null;
    // The button has been destroyed, so don't update its visual status.
    return;
  }
  UpdateAddButtonVisualStatus(plugin);
}

var kAddingStatusLabels = [
  strings.ADD_BUTTON_LABEL,
  strings.STATUS_ADDING,
  strings.STATUS_ADDED,
  strings.STATUS_ERROR_ADDING,
];
var kUpdatingStatusLabels = [
  strings.UPDATE_BUTTON_LABEL,
  strings.STATUS_UPDATING,
  strings.STATUS_UPDATED,
  strings.STATUS_ERROR_UPDATING,
];
var kStatusButtonImages = [
  "images/add_button_default.png",
  "images/status_adding_default.png",
  "images/status_added_default.png",
  "images/status_failed_default.png",
];

function UpdateAddButtonVisualStatus(plugin) {
  var button = plugin.button;
  var status = plugin.download_status;
  var labels = gCurrentCategory == kCategoryUpdates ?
               kUpdatingStatusLabels : kAddingStatusLabels;
  button.caption = labels[status];
  button.image = kStatusButtonImages[status];
  // Don't disable the button when the download status is kDownloadStatusAdding
  // to ensure the button can get mouseout.

  if (status == kDownloadStatusNone) {
    button.overImage = "images/add_button_hover.png";
    button.visible = plugin.mouse_over;
  } else {
    button.visible = true;
    button.overImage = kStatusButtonImages[status];
  }
  button.downImage = status == kDownloadStatusAdding ?
      kStatusButtonImages[status] : "images/add_button_down.png";
}

function add_button_onmousedown(index) {
  // Reset the button status if the user clicks on it.
  SetDownloadStatus(gCurrentPlugins[index], kDownloadStatusNone);
}

function add_button_onclick(index) {
  var plugin = gCurrentPlugins[index];
  if (plugin.download_status != kDownloadStatusAdding) {
    plugin.button = event.srcElement;
    SetDownloadStatus(plugin, kDownloadStatusAdding);
    if (gadgetBrowserUtils.needDownloadGadget(plugin.id)) {
      DownloadPlugin(plugin);
    } else {
      if (AddPlugin(plugin) >= 0)
        SetDownloadStatus(plugin, kDownloadStatusAdded);
      else
        SetDownloadStatus(plugin, kDownloadStatusNone);
    }
  }
}

function pluginbox_onmouseover(index) {
  MouseOverPlugin(event.srcElement, index);
}

function pluginbox_onmouseout(index) {
  MouseOutPlugin(event.srcElement, index);
}

function add_button_onmouseover(index) {
  if (gCurrentPlugins[index].download_status != kDownloadStatusAdding)
    SetDownloadStatus(gCurrentPlugins[index], kDownloadStatusNone);
  MouseOverPlugin(event.srcElement.parentElement, index);
}

function add_button_onmouseout(index) {
  if (gCurrentPlugins[index].download_status != kDownloadStatusAdding)
    SetDownloadStatus(gCurrentPlugins[index], kDownloadStatusNone);
  MouseOutPlugin(event.srcElement.parentElement, index);
}

function MouseOverPlugin(box, index) {
  box.children.item(0).src = "images/thumbnails_hover.png";
  box.children.item(3).children.item(2).src = "images/thumbnails_hover_mask.png";
  // Show the "Add" button.
  box.children.item(4).visible = true;

  var plugin = gCurrentPlugins[index];
  plugin_title.innerText = GetPluginTitle(plugin, gCurrentLanguage);
  plugin_description.innerText = GetPluginDescription(plugin, gCurrentLanguage);
  plugin_description.height = undefined;
  plugin_other_data.innerText = GetPluginOtherData(plugin);
  plugin.mouse_over = true;
}

function MouseOutPlugin(box, index) {
  box.children.item(0).src = "";
  box.children.item(3).children.item(2).src = "images/thumbnails_default_mask.png";
  // Hide the "Add" button when it's in normal state.
  if (!gCurrentPlugins[index].download_status)
    box.children.item(4).visible = false;
  plugin_title.innerText = "";
  plugin_description.innerText = "";
  plugin_other_data.innerText = "";
  gCurrentPlugins[index].mouse_over = false;
}

function GetTotalPages() {
  if (!gCurrentPlugins || !gCurrentPlugins.length) {
    // Return 1 instead of 0 to avoid '1 of 0'.
    return 1;
  }
  return Math.ceil(gCurrentPlugins.length / kPluginsPerPage);
}

function SelectPage(page) {
  plugins_div.removeAllElements();
  plugin_title.innerText = "";
  plugin_description.innerText = "";
  plugin_other_data.innerText = "";

  ClearThumbnailTasks();
  if (!gCurrentPlugins || !gCurrentPlugins.length) {
    gCurrentPage = 0;
  } else {
    gCurrentPage = page;
    var start = page * kPluginsPerPage;
outer:
    for (var r = 0; r < kPluginRows; r++) {
      for (var c = 0; c < kPluginColumns; c++) {
        var i = start + c;
        if (i >= gCurrentPlugins.length)
          break outer;
        AddPluginBox(gCurrentPlugins[i], i, r, c);
      }
      start += kPluginColumns;
    }
  }
  page_label.innerText = strings.PAGE_LABEL.replace("{{PAGE}}", page + 1)
                         .replace("{{TOTAL}}", GetTotalPages());
  previous_button.visible = next_button.visible = page_label.visible = true;
}

function UpdateCategories() {
  category_hover_img.visible = category_active_img.visible = false;
  var y = 0;
  categories_div.removeAllElements();
  var language_categories = gPlugins[gCurrentLanguage];
  if (!language_categories)
    return;

  for (var i in kTopCategories) {
    var category = kTopCategories[i];
    if (category in language_categories) {
      AddCategoryButton(category, y);
      y += kCategoryButtonHeight;
    }
  }
  y += kCategoryGap;
  for (var i in kBottomCategories) {
    var category = kBottomCategories[i];
    if (category in language_categories) {
      AddCategoryButton(category, y);
      y += kCategoryButtonHeight;
    }
  }
  SelectCategory(kCategoryAll);
}

function ResetSearchBox() {
  search_box.value = strings.SEARCH_GADGETS;
  search_box.color = "#808080";
}

var kSearchDelay = 500;
var gSearchTimer = 0;
var gInFocusHandler = false;

function search_box_onfocusout() {
  gInFocusHandler = true
  if (search_box.value == "")
    ResetSearchBox();
  gInFocusHandler = false;
}

function search_box_onfocusin() {
  gInFocusHandler = true;
  if (search_box.value == strings.SEARCH_GADGETS) {
    search_box.value = "";
    search_box.color = "#000000";
  }
  gInFocusHandler = false;
}

var gSearchPluginsTimer = 0;
function search_box_onchange() {
  if (gSearchTimer) cancelTimer(gSearchTimer);
  if (gInFocusHandler || search_box.value == strings.SEARCH_GADGETS)
    return;

  if (search_box.value == "") {
    SelectCategory(kCategoryAll); // TODO: kCategoryRecommendations.
    // Undo the actions in ResetSearchBox() called by SelectCategory().
    search_box_onfocusin();
  } else {
    if (gSearchPluginsTimer)
      view.clearTimeout(gSearchPluginsTimer);
    gSearchPluginsTimer = setTimeout(function () {
      if (search_box.value && search_box.value != strings.SEARCH_GADGETS) {
        SelectCategory(null);
        gCurrentPlugins = SearchPlugins(search_box.value, gCurrentLanguage);
        SelectPage(0);
        gSearchPluginsTimer = 0;
      }
    }, kSearchDelay);
  }
}

