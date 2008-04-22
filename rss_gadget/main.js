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

var kURLOption = "rss_url";
var kRefreshInterval = 600000; // every 10 minutes

var kItemHeadingColor = "#ffffff";
var kItemSourceColor = "#00ffff";
var kItemTimeColor = "#00ffff";
var kWidthOffset = 12;
var kHeightOffset = 23;

var g_xml_request = null;

var g_feed_title = null;
var g_feed_items = null;

options.putDefaultValue(kURLOption, "http://googleblog.blogspot.com/atom.xml");

contents.contentFlags = gddContentFlagHaveDetails;

plugin.onAddCustomMenuItems = OnAddCustomMenuItems;

view.setInterval("Refresh()", kRefreshInterval);

function OnAddCustomMenuItems(menu) { 
  menu.AddItem(strings.GADGET_REFRESH, 0, RefreshMenuHandler);
}

function RefreshMenuHandler(item_text) {
  Refresh();
}

function OnSize() {
  contents.width = view.width - kWidthOffset;
  contents.height = view.height - kHeightOffset;
}

function Refresh() {
  gadget.debug.trace("Refreshing page.");

  UpdateCaption(strings.GADGET_LOADING);

  var url = options.getValue(kURLOption);
  url = NormalizeURL(url);
  LoadDocument(url);
}

function LoadDocument(url) {
  if (null == url) {
    UpdateCaption(strings.GADGET_ERROR); // This should never happen.
  }

  gadget.debug.trace("Loading " + url);
  if ("" == url) { // Not an error, the URL could be unset.
    return;
  }  

  g_xml_request = new XMLHttpRequest();
  g_xml_request.onreadystatechange = ProcessRequest;
  g_xml_request.open("GET", url, true);

  // disable caching
  g_xml_request.setRequestHeader("Cache-Control", "no-cache");
  g_xml_request.setRequestHeader("Cache-Control", "must-revalidate");  
  g_xml_request.setRequestHeader("If-Modified-Since", "Sat, 1 Jan 1978 00:00:00 GMT");
  g_xml_request.setRequestHeader("Pragma", "no-cache");

  g_xml_request.send(null);
}

function ProcessRequest() {
  var readystate = g_xml_request.readyState;
  if (4 == readystate) { // complete
    if (200 == g_xml_request.status) {
      ParseDocument();
    } else {
      UpdateCaption(strings.GADGET_ERROR + ": " + g_xml_request.status);
    }

    g_xml_request = null;
  }
}

function ParseDocument() {
  var xml = g_xml_request.responseXML;
  if (xml == null) {
    UpdateCaption(strings.GADGET_UNKNOWN);
    return;
  }

  // Try various formats
  if (BuildAtomDoc(xml)) {
    gadget.debug.trace("ATOM document parsed");
  } else if (BuildRSSDoc(xml)) {
    gadget.debug.trace("RSS document parsed");
  } else { // Unsupported format.
    UpdateCaption(strings.GADGET_UNKNOWN);
    return;
  }

  gadget.debug.trace("finished parsing");
  DisplayFeedItems();
}

function BuildAtomDoc(xml) {
  var feed = xml.getElementsByTagName("feed");
  if (null == feed) {
    return false;
  }

  var feed_elem = feed[0];
  if (null == feed_elem) {
    return false;
  }

  g_feed_title = GetElementText("title", feed_elem);
  feed_items = new Array();

  var items = xml.getElementsByTagName("entry");
  if (null != items) {
    var len  = items.length;
    for (var i = 0; i < len; i++) {
      var item_elem = items[i];
      var item = {};

      item.title = GetElementText("title", item_elem);
      item.url = GetElementAttrib("link", item_elem, "href");

      // description
      var desc = GetElementText("summary", item_elem);
      if (0 == desc.length) {
        desc = GetElementText("content", item_elem);
      }
      item.description = desc;

      var date = GetElementText("modified", item_elem);
      if (0 == date.length) {
        date = GetElementText("updated", item_elem);
      }
      if (0 == date.length) {
        date = GetElementText("created", item_elem);
      }
      if (0 == date.length) {
        date = GetElementText("published", item_elem);
      }
      item.date = ParseISO8601Date(date); 

      feed_items[i] = item;
    }
  }

  UpdateGlobalItems(feed_items);

  return true;
}

function BuildRSSDoc(xml) {
  // Older feeds use "RDF;" newer ones use "rss."
  var rss = xml.getElementsByTagName("rss");
  var rss_elem = null;
  if (null != rss) {
    rss_elem = rss[0];
  }

  if (null == rss_elem) {
    rss =  xml.getElementsByTagName("rdf:RDF");
    if (null != rss) {
      rss_elem = rss[0];
    }

    if (null == rss_elem) { 
      return false;
    } else {
      gadget.debug.trace("RDF document found");
    }
  } else {
    gadget.debug.trace("RSS2 document found");
  }

  var channels = xml.getElementsByTagName("channel");
  if (null == channels) {
    return false;
  }

  var chan_elem = channels[0];
  if (null == chan_elem) {
    return false;
  }

  g_feed_title = GetElementText("title", chan_elem);
  feed_items = new Array();

  // In older feeds, the "item" elements are under "rdf". 
  // Newer ones have them under "channel," but fortunately
  // getElementsByTagName will search all nodes under rss/rdf tree 
  // regardless of the item parent.
  var items = rss_elem.getElementsByTagName("item");
  if (null != items) {
    var len  = items.length;
    for (var i = 0; i < len; i++) {
      var item_elem = items[i];
      var item = {};

      item.title = GetElementText("title", item_elem);
      item.url = GetElementText("link", item_elem);
      item.description = GetElementText("description", item_elem);
      item.date = ParseDate(GetElementText("pubDate", item_elem));

      feed_items[i] = item;
    }
  }

  UpdateGlobalItems(feed_items);

  return true;
}

function UpdateGlobalItems(items) {
  // Match based on URL.
  var len  = items.length;
  gadget.debug.trace("Read " + len + " items.");
  for (var i = 0; i < len; i++) {
    var item = items[i];
    var olditem = FindFeedItem(item.url);
    if (olditem == null) {
      item.is_new = true;
      item.is_read = item.is_hidden = false;
    } else {
      item.is_new = false;
      item.is_read = olditem.is_read;
      item.is_hidden = olditem.is_hidden;
    }   
  }

  g_feed_items = items;
}

function GetElementText(elem_name, parent) {
  var elem = parent.getElementsByTagName(elem_name);
  if (null == elem) {
    return "";
  }

  var child = elem[0];
  if (null == child) {
    return "";
  }

  var text = child.text;
  if (null == text) {
    return "";
  }

  return TrimString(text);
}

function GetElementAttrib(elem_name, parent, attrib_name) {
  var elem = parent.getElementsByTagName(elem_name);
  if (null == elem) {
    return "";
  }

  var child = elem[0];
  if (null == child) {
    return "";
  }

  var attrib = child.getAttribute(attrib_name);
  return TrimString(attrib);
}

function TrimString(s) {
  s.replace(/\s+$/, "");
  s.replace(/^\s+/, "");
  return s;
}

function NormalizeURL(url) {
  if (null == url) {
    return "";
  }
  return url.replace(/^feed:\/\//, "http://");
}

function UpdateCaption(text) {
  if (!text) {
    view.caption = strings.GADGET_NAME;
  } else {
    view.caption = text + " - " + strings.GADGET_NAME;
  }
  gadget.debug.trace("new caption: " + view.caption);
}

function DisplayFeedItems() {
  UpdateCaption(g_feed_title); 
  contents.removeAllContentItems();

  if (g_feed_items) {
    // Append in reverse.
    var len = g_feed_items.length;
    for (var i = len - 1; i >= 0; i--) {
      var item = g_feed_items[i];

      if (item.is_hidden) {
	continue;
      }

      var c_item = new ContentItem();
      c_item.heading = c_item.tooltip = item.title;
      c_item.snippet = item.description;
      c_item.source = g_feed_title;
      c_item.open_command = item.url;
      c_item.headingColor = kItemHeadingColor;
      c_item.sourceColor = kItemSourceColor;
      c_item.timeColor = kItemTimeColor;

      if (!item.is_read) {
        c_item.flags |= gddContentItemFlagHighlighted;
      }

      c_item.time_created = item.date;// getvardate?
      gadget.debug.trace("print time " + c_item.time_created.toString());

      c_item.onDetailsView = OnDetailsView;
      c_item.onRemoveItem = OnRemoveItem;

      c_item.layout = gddContentItemLayoutNews;

      var options = gddItemDisplayInSidebar;
      if (item.is_new) {
        options |= gddItemDisplayAsNotification;
        c_item.flags = gddContentItemFlagHighlighted;
      }
      contents.addContentItem(c_item, options);
    }
  }
}

function MarkItemAsRead(c_item) {
  c_item.flags &= ~gddContentItemFlagHighlighted;

  var item = FindFeedItem(c_item.open_command);
  if (item != null) {
    item.is_read = true;
  }
}

function OnDetailsView(item) {
  var html = "<html><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">";
  html += "<head><style type=\"text/css\">body {font-size: 12px;}</style></head><body>";
  html += item.snippet;
  html += "</body></html>";

  var control = new DetailsView();
  control.SetContent("", item.time_created, html, true, item.layout);
  control.html_content = true;

  var details = new Object();
  details.title = item.heading;
  details.details_control = control;
  details.flags = gddDetailsViewFlagRemoveButton;

  MarkItemAsRead(item);

  return details;
}

function OnRemoveItem(c_item) {
  var item = FindFeedItem(c_item.open_command);
  if (item != null) {
    item.is_hidden = true;
  }

  return false;
}

function FindFeedItem(url) {
  if (null != g_feed_items) {
    for (var j = 0; j < g_feed_items.length; j++) {
      var item = g_feed_items[j];
      if (url == item.url) {
	return item;
      }      
    }
  }

  return null;
}

function SetNewFeedURL(url) {
  UpdateCaption(null);
  contents.removeAllContentItems();
  options.putValue(kURLOption, url);
  Refresh();
}

function ParseDate(date) {
  var d = new Date();
  if ("" != date) {
    var time = Date.parse(date);
    d.setTime(time);
  }
  return d;
}

// Based on http://delete.me.uk/2005/03/iso8601.html
function ParseISO8601Date(date) {
  var regexp = "([0-9]{4})(-([0-9]{2})(-([0-9]{2})";
  regexp += "(T([0-9]{2}):([0-9]{2})(:([0-9]{2})(\.([0-9]+))?)?";
  regexp += "(Z|(([-+])([0-9]{2}):([0-9]{2})))?)?)?)?";

  var arr = date.match(new RegExp(regexp));
  var offset = 0;
  var d = new Date(arr[1], 0, 1);

  if (arr[3]) { 
    d.setMonth(arr[3] - 1); 
  }
  if (arr[5]) { 
    d.setDate(arr[5]); 
  }
  if (arr[7]) { 
    d.setHours(arr[7]);
  }
  if (arr[8]) { 
    d.setMinutes(arr[8]); 
  }
  if (arr[10]) { 
    d.setSeconds(arr[10]);
  }
  if (arr[12]) { 
    d.setMilliseconds(Number(arr[12]) / 1000); 
  }
  if (arr[14]) {
    offset = (Number(arr[16]) * 60) + Number(arr[17]);
    if (arr[15] != '-') {
      offset *= -1;
    }
  }

  offset -= d.getTimezoneOffset();
  d.setTime(d.getTime() + (offset * 60000));

  return d;
}
