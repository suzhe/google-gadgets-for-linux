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

var kThumbnailPrefix = "http://desktop.google.com/plugins/images/";
var kMaxWorkingTasks = 6;
var gPendingTasks = [];
var gWorkingTasks = [];

function ClearThumbnailTasks() {
  gPendingTasks = [];
  for (var i in gWorkingTasks)
    gWorkingTasks[i].request.abort();
  gWorkingTasks = [];
}

function AddThumbnailTask(plugin, thumbnail_element1, thumbnail_element2) {
  var image_data = gadgetBrowserUtils.loadThumbnailFromCache(plugin.id);
  if (image_data) {
    thumbnail_element.src = image_data;
  } else {
    var new_task = {
      plugin: plugin,
      thumbnail_element1: thumbnail_element1,
      thumbnail_element2: thumbnail_element2,
    };
    if (gWorkingTasks.length < kMaxWorkingTasks)
      StartThumbnailTask(new_task);
    else
      gPendingTasks.push(new_task);
  }
}

function StartThumbnailTask(task) {
  var thumbnail_url = task.plugin.attributes.thumbnail_url;
  if (!thumbnail_url)
    return;
  if (!thumbnail_url.match(/^https?:\/\//))
    thumbnail_url = kThumbnailPrefix + thumbnail_url;
  gadget.debug.trace("Start loading thumbnail: " + thumbnail_url);
 
  var request = new XMLHttpRequest();
  try {
    request.open("GET", thumbnail_url);
    task.request = request;
    request.onreadystatechange = function() {
      OnRequestStateChange();
    };
    request.send();
    gWorkingTasks.push(task);
  } catch (e) {
    gadget.debug.error("Request error: " + e);
  }

  function OnRequestStateChange() {
    if (request.readyState == 4) {
      if (request.status == 200) {
        gadget.debug.trace("Finished loading a thumbnail: " + thumbnail_url);
        var data = request.responseStream;
        task.thumbnail_element1.src = data;
        task.thumbnail_element2.src = data;
        gadgetBrowserUtils.saveThumbnailToCache(task.plugin.id, data);
      } else {
        gadget.debug.error("Request " + thumbnail_url + " returned status: " +
                           request.status);
      }
  
      for (var i = 0; i < gWorkingTasks.length; i++) {
        if (gWorkingTasks[i] == task) {
          gWorkingTasks.splice(i, 1);
          while (gWorkingTasks.length < kMaxWorkingTasks &&
                 gPendingTasks.length > 0) {
            StartThumbnailTask(gPendingTasks.shift());
          }
          break;
        }
      }
    }
  }
}
