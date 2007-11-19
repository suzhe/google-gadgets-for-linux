/*
Copyright (C) 2007 Google Inc.

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

/**
 * Theming system
 */

/**
 * The key to use in the "options" object
 * for storing the current color.
 */
var MILKTHEME_COLOR = "MilkTheme_Color";

var COLORS = new Array(8);
COLORS[0] = "#CECEC2";
COLORS[1] = "#FFA72B";
COLORS[2] = "#DEFF00";
COLORS[3] = "#89EFAA";
COLORS[4] = "#90D7FF";
COLORS[5] = "#B6B0FF";
COLORS[6] = "#D58CFF";
COLORS[7] = "#FF86EA";

/**
 * Color changer button, "onmouseover" handler
 */
function NextColor_onMouseOver(mouseEvent) {
  ColorButton.background = getNextTheme();
}

/**
 * Color changer button, "onmouseout" handler
 */
function NextColor_onMouseOut(mouseEvent) {
  ColorButton.background = "#DDDDDD";
}

function NextColor_onClick() {
  var color = getNextTheme();
  // Change the theme color option.  
  // This will trigger the "view.onoptionchanged" handler, if present.
  options.putValue(MILKTHEME_COLOR, color);

  ColorButton.background = color;
}

function getNextTheme() {
  var currentColor = options.getValue(MILKTHEME_COLOR);

  var i = 0;
  for( ; i < COLORS.length; ++i) {
    if(COLORS[i] == currentColor) {
      break;
    }
  }

  ++i;
  if(i >= COLORS.length) {
    i = 0;
  }

  return COLORS[i];
}
