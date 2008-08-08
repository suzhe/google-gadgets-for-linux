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

function GoogleISFeedItem(url, type) {
  if (url.match(GoogleISFeedItem.prototype.url)) {
    this.type = "http";
    this.url = url;
  } else {
    this.type = ":error";
    this.url = "";
  }
}

GoogleISFeedItem.prototype =
  new FeedItem("^http://images.google.com/images",
               ":plugins:GoogleISFeedItem",
               GoogleISFeedItem);

//set it for pass FeedItem check
GoogleISFeedItem.prototype.httpContentTypeMaps = [{pattern: ".",
                                                   type: ":html"}];

GoogleISFeedItem.prototype._htmlFilImgRegExp =
  new RegExp("[a-zA-Z]+\\.Img\\(\"(http://[^\"]+)\","//1 orignal page
             + "\"[^\"]*\","// target = blank
             + "\"([^\"]+)\","//2 googleimage token
             + "\"(http://[^\"]+)\","//3 original image
             + "\"[0-9]+\",\"[0-9]+\"," // width x height
             + "\"([^\"]*)\"," // 4 title
             + "(?:\"[^\"]*\",){7}"
             + "\"(http://[^\"]+\\.google\\.com[^\"]*)\","//5 google image prefix
             + "\"1\",\\[\\]\\);"
            );//TODO: check this with more result and get title...

GoogleISFeedItem.prototype._htmlNewSourceItemFromRegExpMatch = function(mmr) {
  return new SourceItem(mmr[5] + "?q=tbn:" + mmr[2] + mmr[3], this,
                        mmr[4].replace(/\\x3c/g, '<').replace(/\\x3e/g, '>').replace(/<[^>]*>/g, ""),
                        mmr[3],
                        "",
                        "http");
};


function FlickrISFeedItem(url, type) {
  if (url.match(FlickrISFeedItem.prototype.url)) {
    this.type = "http";
    this.url = url;
  } else {
    this.type = ":error";
    this.url = "";
  }
}

FlickrISFeedItem.prototype = new FeedItem("^http://www.flickr.com/search/",
                                          ":plugins:FlickrISFeedItem",
                                          FlickrISFeedItem);

FlickrISFeedItem.prototype.httpContentTypeMaps =
  [{pattern: ".", type: ":html"}];

FlickrISFeedItem.prototype._htmlFilImgRegExp =
  new RegExp("<img [^>]*\\bsrc=['\"]"
             + "(http://farm[0-9]*.static.flickr.com/[^@#?'\">]+)['\"]");


function YahooISFeedItem(url, type) {
  if (url.match(YahooISFeedItem.prototype.url)) {
    this.type = "http";
    this.url = url;
  } else {
    this.type = ":error";
    this.url = "";
  }
}

YahooISFeedItem.prototype =
  new FeedItem("^http://images.search.yahoo.com/search/images",
               ":plugins:YahooISFeedItem",
               YahooISFeedItem);

YahooISFeedItem.prototype.httpContentTypeMaps = [{pattern: ".", type: ":html"}];

YahooISFeedItem.prototype._htmlFilImgRegExp =
  new RegExp("storetds\\[[0-9]+\\]\\.isrc=['\"](http://[^'\"]+)['\"];");
