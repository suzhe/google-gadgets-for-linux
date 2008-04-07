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

var kDownloadURLOption = "DOWNLOAD_URL";
var kModuleURLOption = "MODULE_URL_PREFIX";
var kLocaleOption = "LOCALE";

var g_user_pref_names = null;
var g_gadget_attribs = null;

var kUserPrefPrefix = "up_";
var kUserAgent = "Mozilla/5.0 (compatible; Google Desktop)";

var kDefaultLocale = "en-US";
var kDefaultModuleURLPrefix = "http://gmodules.com/ig/ifr?url=";
options.putDefaultValue(kModuleURLOption, kDefaultModuleURLPrefix);
options.putDefaultValue(kLocaleOption, kDefaultLocale);

var g_download_url = options.getValue(kDownloadURLOption);
var g_module_url_prefix = options.getValue(kModuleURLOption);
var g_commonparams = null;
var g_xml_request_gadget = null;
var g_xml_request_options = null;

// FOR TESTING
//g_download_url= "http://orawiz.googlepages.com/hangman.xml";
//g_download_url= "http://www.google.com/ig/modules/datetime.xml";

/// Gadget init code.

plugin.onAddCustomMenuItems = OnAddCustomMenuItems;
SetUpCommonParams();
LoadGadget();

/// Functions Start

function OnAddCustomMenuItems(menu) { 
  // Grayed if gadget hasn't been loaded yet.
  var flags = (g_user_pref_names == null) ? gddMenuItemFlagGrayed : 0;

  menu.AddItem(strings.GADGET_REFRESH, flags, RefreshMenuHandler);    
  menu.AddItem(strings.GADGET_ABOUTMENU, flags, AboutMenuHandler);
}

function RefreshMenuHandler(item_text) {
  RefreshGadget();
}

function AboutMenuHandler(item_text) {
  var html =  "<html><head><style type=\"text/css\">"
    + "p {font-size: 12px;}</style></head>"
    + kHTMLMeta
    + "<body><h5>" + g_gadget_attribs.title + "</h5><p>"
    + g_gadget_attribs.author + "<br>" 
    + g_gadget_attribs.author_email + "</p><p>"
    + g_gadget_attribs.description
    + "</p></body></html>";

  var control = new DetailsView();
  control.SetContent("", null, html, true, 0);
  control.html_content = true;
  plugin.showDetailsView(control, strings.GADGET_ABOUTMENU, 0, null);
}

// Options view

function LoadOptions() {
  // Cannot use a frame to load the content here because of security 
  // restrictions involving scripts on pages from different domains.
  g_xml_request_options = new XMLHttpRequest();
  g_xml_request_options.onreadystatechange = ProcessOptions;
  g_xml_request_options.open("GET", GetOptionsURL(), true);
  g_xml_request_options.setRequestHeader("User-Agent", kUserAgent);
  g_xml_request_options.send(null);
}

function ProcessOptions() {
  var readystate = g_xml_request_options.readyState;
  if (4 == readystate) { // complete
    if (200 == g_xml_request_options.status) {     
      options.putValue(kOptionsHTML, g_xml_request_options.responseText);
    } else {
      gadget.debug.error("Error loading options HTML for gadget.");
    }
    g_xml_request_options = null;
  }
}

function OnOptionChanged() {
  if (event.propertyName == kRefreshOption) {
    RefreshGadget();
  }
}

/// Main view

function OnOpenURL(url) {
  // The frame containing the gadget URL will also trigger onOpenURL signal,
  // so return false to let it be opened in place.
  return url != GetGadgetURL();
}

function RefreshGadget() {
  if (HasUnsetUserPrefs()) {
    // Must show options dialog before showing gadget.
    DisplayMessage(GADGET_REQUIRED);

    plugin.ShowOptionsDialog();
  } else {
    gadget.debug.trace("Showing gadget.");

    // We can show the gadget.
    var html = "<html>"
      + kHTMLMeta
      + "<frameset border=\"0\" cols=\"100%\" frameborder=\"no\">"
      + "<frame height=\"100%\" width=\"100%\" scrolling=\"no\" src=\"" 
      + GetGadgetURL() + "\" marginheight=\"0\" marginwidth=\"0\"/>"
      + "</frameset></html>";
    browser.innerText = html;
    browser.onOpenURL = OnOpenURL;
  }
}

function LoadGadget() {
  if (!g_download_url) {
    // This should never happen.
    DisplayMessage(strings.GADGET_ERROR);
    return;
  }

  DisplayMessage(strings.GADGET_LOADING);

  LoadRawXML();
  LoadOptions();
}

function LoadRawXML() {
  gadget.debug.trace("Loading raw XML.");

  g_xml_request_gadget = new XMLHttpRequest();
  g_xml_request_gadget.onreadystatechange = ProcessXML;
  g_xml_request_gadget.open("GET", GetConfigURL(), true);
  g_xml_request_gadget.setRequestHeader("User-Agent", kUserAgent);
  g_xml_request_gadget.send(null);
}

function ProcessXML() {
  var readystate = g_xml_request_gadget.readyState;
  if (4 == readystate) { // complete
    if (200 == g_xml_request_gadget.status) {
      ParseRawXML();
    } else {
      DisplayMessage(g_xml_request_gadget.status + ": " + strings.GADGET_ERROR);
    }
    g_xml_request_gadget = null;
  } 
}

function HasUnsetUserPrefs() {
  var result = false;

  // Also generate preset options script fragment here.
  var preset = "";

  var len  = g_user_pref_names.length;
  for (var i = 0; i < len; i++) {
    var pref = g_user_pref_names[i];
    var value = options.getValue(pref);
    if (!value) {
      gadget.debug.trace("Unset pref: " + pref);
      // Do not break to continue generating script fragment.
      result = true; 
    } else { // pref is set, add to prefix
      if (preset != "") {
        preset += "else ";
      }
      preset += "if(obj.name=='m_" + EncodeJSString(pref) + "'){"
          + "obj.value='" + EncodeJSString(value) + "';}";
    }
  }

  options.putValue(kPresetOptions, preset);

  return result;
}

function ViewOnSizing() {
  if (null != g_gadget_attribs) {
    var w = g_gadget_attribs.width;
    var h = g_gadget_attribs.height;
    // Disallow resizing to larger than specified w, h values.
    if (null != w && event.width > w) {
      event.width = w;
    }
    if (null != h && event.height > h) {
      event.height = h;
    }
    gadget.debug.trace("OnSizing: " + event.width + ", " + event.height);
  }
}

function ShowGadget() {
  view.caption = g_gadget_attribs.title;

  var w = g_gadget_attribs.width;
  var h = g_gadget_attribs.height;

  if (w == null) {
    w = view.width;
  }
  if (h == null) {
    h = view.height;
  }

  gadget.debug.trace("Resizing to " + w + ", " + h);
  view.resizeTo(w, h);

  RefreshGadget();
}

function ParseRawXML() {
  var xml = g_xml_request_gadget.responseXML;
  if (xml == null) {
    DisplayMessage(strings.GADGET_ERROR);
    return false;
  }

  // Read gadget attributes.
  var mod_prefs = xml.getElementsByTagName("ModulePrefs");
  if (null == mod_prefs) {
    DisplayMessage(strings.GADGET_ERROR);
    return false;
  }

  var attribs = mod_prefs[0];
  g_gadget_attribs = {};
  g_gadget_attribs.title = GetElementAttrib(attribs, "title");
  g_gadget_attribs.description = GetElementAttrib(attribs, "description");
  g_gadget_attribs.author = GetElementAttrib(attribs, "author");
  g_gadget_attribs.author_email 
    = GetElementAttrib(attribs, "author_email");

  var param = GetElementAttrib(attribs, "width");
  g_gadget_attribs.width = (param == "") ? null : Number(param);
  param = GetElementAttrib(attribs, "height");
  g_gadget_attribs.height = (param == "") ? null : Number(param);

  // Note: it is important that the loop below do not overwrite prefs
  // that the user may have already set in a previous run of this gadget
  // instance. Thus only the default value is set here.
  var prefs = xml.getElementsByTagName("UserPref");
  g_user_pref_names = new Array();
  if (null != prefs) {
    var len  = prefs.length;
    for (var i = 0; i < len; i++) {
      var pref = prefs[i];
      var name = g_user_pref_names[i] = 
          kUserPrefPrefix + GetElementAttrib(pref, "name");
      // var required = GetElementAttrib(pref, "required");      
      var def = GetElementAttrib(pref, "default_value");
      if (def != "") {
        options.putDefaultValue(name, def);
      }
    }
  }

  ShowGadget();

  return true;
}

function GetElementAttrib(elem, attrib_name) {
  var attrib = elem.getAttribute(attrib_name);
  return TrimString(attrib);
}

function TrimString(s) {
  s.replace(/\s+$/, "");
  s.replace(/^\s+/, "");
  return s;
}

function EncodeJSString(s) {
  s.replace(/\"/g, "\\\"");
  s.replace(/\\/g, "\\\\");
  s.replace(/\n/g, "\\n");
  s.replace(/\r/g, "\\r");

  return s;
}

function DisplayMessage(msg) {
  // Assume these messages are properly escaped.
  browser.innerText = "<html>"
    + kHTMLMeta
    + "<body><center><h6>" 
    + msg 
    + "</h6></center></body></html>";
}

function SetUpCommonParams() {
  var locale = options.getValue(kLocaleOption).split('-', 2); 
  g_commonparams = "&.lang=" + locale[0] + "&.country=" + locale[1]
    + "&synd=open";
}

function GetConfigURL() {
  return g_module_url_prefix + encodeURI(g_download_url)
    + "&output=rawxml" + g_commonparams;
}

function GetGadgetURL() {
  var len  = g_user_pref_names.length;
  var params = "";
  for (var i = 0; i < len; i++) {
    var pref = g_user_pref_names[i];
    var value = options.getValue(pref);
    params += "&" + encodeURIComponent(pref) + "=" + encodeURIComponent(value);
  }

  return g_module_url_prefix + encodeURI(g_download_url)
    + params + g_commonparams;
}

function GetOptionsURL() {
  return g_module_url_prefix + encodeURI(g_download_url)
    + "&output=edithtml" + g_commonparams;
}
