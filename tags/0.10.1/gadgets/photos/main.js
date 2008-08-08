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

view.onopen = function() {
  initMain();
  kSlideShower.bindSlideShow();
  kSlideShower.slideStart();
  kSlideShower.resizedView();
};

var kSlideShower;

var kOptions = {
  feedurls: [],
  slideInterval: 15,
  showers: [shower0, shower1],
  matrixShowers: [mwimg0, mwimg1, mwimg2, mwimg3, mwimg4,
      mwimg5, mwimg6, mwimg7, mwimg8],
  slideWrap: popin_wrap,
  matrixWrap: popout_wrap,
  showersWrap: showers_wrap,
  slideUI: slide_ui
};

var kBlackList = {};

function initMain() {
  optionsPut("feedurls",
      [
      "http://images.google.com/images?q=nature&imgsz=xxlarge"
        ],
        1);
  optionsPut("slideInterval", 15, 1);
  optionsPut("readyMax", 10, 1);
  optionsPut("blackList", {}, 1);
  kOptions.feedurls = optionsGet("feedurls");
  kOptions.slideInterval = optionsGet("slideInterval") - 0;
  kBlackList = optionsGet("blackList");

  kSlideShower = new SlideShower(kOptions);
  kSlideShower.onSlideStop = function() {
    playpause.image = "images/play_1.png";
    playpause.overImage = "images/play_2.png";
    playpause.downimage = "images/play_3.png";
  };
  kSlideShower.onSlideStart = function() {
    playpause.image = "images/pause_1.png";
    playpause.overImage = "images/pause_2.png";
    playpause.downImage = "images/pause_3.png";
  };
  plugin.onAddCustomMenuItems = function(menu) {
    menu.AddItem(MENU_LANDSCAPE, 0, function() {
      viewRationResize(4, 3);
    });
    menu.AddItem(MENU_PORTRAIT, 0, function() {
      viewRationResize(3, 4);
    });
    menu.AddItem(
      MENU_NO_CROP,
      kSlideShower.states.scaleModel == "photo" ? 0 : gddMenuItemFlagChecked,
      function() {
        if (kSlideShower.states.scaleModel == "photo") {
          kSlideShower.states.scaleModel = "nocrop";
        } else {
          kSlideShower.states.scaleModel = "photo";
        }
        kSlideShower.resizedView();//force resize check
      }
    );
  };
}

// w, h must be integer >= 1
function viewRationResize(w, h) {
  var vw = view.width, vh = view.height;
  var dw, dh;
  dw = Math.round(Math.sqrt(vw * vh / w / h) * w);
  dh = Math.round(dw * h / w);
  if (system && system.screen) {
    var maxw = system.screen.size.width;
    var maxh = system.screen.size.height;
    if (dw > maxw || dh > maxh) {
      var to = {width: w, height: h};
      scaleToMax(to, {width: maxw, height: maxh});
      dw = to.width;
      dh = to.height;
    }
  }
  //debug.trace("resize to w=" + dw + " h=" + dh);
  view.resizeTo(dw, dh);
}

view.onoptionchanged = function() {
  kOptions.feedurls = optionsGet("feedurls");
  kOptions.slideInterval = optionsGet("slideInterval");
  kSlideShower.onOptionsChanged(kOptions);
};

view.onsize = function() {
  if (view.width < 95) view.width = 95;
  if (view.height < 50) view.height = 50;
  kSlideShower.resizedView();
};

view.ondock = function() {
  //debug.trace("DOCK HAPPEND...................................");
};

view.onundock = function() {
  //debug.trace("UNDOCK HAPPEND...................................");
};

view.onpopout = function() {
  kSlideShower.openMatrixView();
};

view.onpopin = function() {
  kSlideShower.closeMatrixView();
};
