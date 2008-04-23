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

//  Array of user pref (name, value) pairs
var g_user_prefs = null;

/// Gadget init code.
opbrowser.onGetProperty = OnGetProperty;
opbrowser.onCallback = OnCallback;

// Options view

function OnGetProperty(p) {
  if (p == kSetPrefs) {
    return "\"function\"";
  }
}

function OnCallback(f, pref) {
  if (f == kSetPrefs) {
    gadget.debug.trace("SetPref " + pref);
    if (null == g_user_prefs) {
      g_user_prefs = new Array();
    }
    var index = Number(pref[0]) * 2;
    g_user_prefs[index] = pref[1];
    g_user_prefs[index + 1] = pref[2];
  }
}

function OnOK() {
  gadget.debug.trace("OK pressed");

  if (null != g_user_prefs) {
    for (var i = 0; i < g_user_prefs.length; i += 2) {
      var name = g_user_prefs[i];
      if (name) {
        var value = g_user_prefs[i + 1];
        options.putValue(name, value);
      }
    }

    g_user_prefs = null;
    // Toggle kRefreshOption
    var refresh = options.getValue(kRefreshOption);
    if (!refresh || refresh == "0") {
      options.putValue(kRefreshOption, "1");
    } else {
      options.putValue(kRefreshOption, "0");
    }
  }
}

function OnCancel() {
  // Do nothing
  gadget.debug.trace("cancel pressed");
}

function GeneratePage() {
  var code = options.getValue(kOptionsHTML);

  var html = "<html>" + kHTMLMeta;
  if (!code) {
    html += "<body><center><h6>" + strings.GADGET_NOTLOADED + "</h6></center>";
  }
  else {
    var preset = options.getValue(kPresetOptions);

    html += "<script type=\"text/javascript\">"
      + "function _gel(a){return document.getElementById(a);}"
      + "function OnChange(e){"
        + "var obj=e.target;"
        + "if (obj.value!=null){"
          +"window.external.SetPrefs(obj.id.substring(2,obj.id.length),"
          +"obj.name.substring(2,obj.name.length),obj.value);}}"
      + "function OnLoad(){"
        + "var lang=_gel('lang_selector');"
        + "if (lang!=null) lang.style.display='none';"
        + "var num_fields=_gel('m_numfields').value;"
        + "for(var i=0;i<num_fields;i++){"
          + "var obj=_gel('m_'+i);if(obj==null){continue;}"
          + preset
          + "obj.onchange=OnChange;obj.onclick=OnChange;}}"
      + "</script>"
      + "<body bgcolor=\"" + options.getvalue(kBGColorOption) 
      + "\" marginheight=\"0\" marginwidth=\"0\" onload=\"OnLoad()\">"
      + code;
  }
  html += "</body></html>";

  return html;
}

function OnOptionChanged() {
  if (event.propertyName == kOptionsHTML || 
      event.propertyName == kPresetOptions) {
    opbrowser.innerText = GeneratePage();
  }
}

function OnOpen() {
  opbrowser.innerText = GeneratePage();
}
