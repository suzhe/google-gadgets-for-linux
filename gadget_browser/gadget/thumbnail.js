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
var g_pending_tasks = [];
var g_working_tasks = [];

function ClearThumbnailTasks() {
  g_pending_tasks = [];
  for (var i in g_working_tasks)
    g_working_tasks[i].request.abort();
  g_working_tasks = [];
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
    if (g_working_tasks.length < kMaxWorkingTasks)
      StartTask(new_task);
    else
      g_pending_tasks.push(new_task);
  }
}

function StartTask(task) {
  var request = new XMLHttpRequest();
  var thumbnail_url = task.plugin.attributes.thumbnail_url;
  if (!thumbnail_url)
    return;
  if (!thumbnail_url.match(/^https?:\/\//))
    thumbnail_url = kThumbnailPrefix + thumbnail_url;
  gadget.debug.trace("Start loading thumbnail: " + thumbnail_url);
 
  try {
    request.open("GET", thumbnail_url);
    task.request = request;
    request.onreadystatechange = function() {
      OnRequestStateChange(task);
    };
    request.send();
    g_working_tasks.push(task);
  } catch (e) {
    gadget.debug.error("Request error: " + e);
  }
}

function OnRequestStateChange(task) {
  var request = task.request;
  if (request.readyState == 4) {
    if (request.status == 200) {
      gadget.debug.trace("Finished loading a thumbnail: " + task.plugin.id);
      var data = request.responseStream;
      task.thumbnail_element1.src = data;
      task.thumbnail_element2.src = data;
      gadgetBrowserUtils.saveThumbnailToCache(task.plugin.id, data);
    } else {
      gadget.debug.error("Request returned status: " + request.status);
    }

    for (var i = 0; i < g_working_tasks.length; i++) {
      if (g_working_tasks[i] == task) {
        g_working_tasks.splice(i, 1);
        WakeUpPendingTasks();
        break;
      }
    }
  }
}

function WakeUpPendingTasks() {
  while (g_working_tasks.length < kMaxWorkingTasks &&
         g_pending_tasks.length > 0) {
    StartTask(g_pending_tasks.shift());
  }
}
