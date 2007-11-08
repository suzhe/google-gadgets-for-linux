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
 * Google Gadgets LCD Digital Clock
 * 
 * This gadget demonstrates a lot of useful API features:
 * 
 * <ul>
 *   <li>Using custom fonts with your gadget.  See "gadget.manifest" and "main.xml" files.</li>
 *   <li>Storing and retrieving values with the "options" object.</li>
 *   <li>The "onoptionchanged" event, 
 *     which fires when a property in the "options" object is changed.</li>
 *   <li>Adding menu items to the gadget's menu.</li>
 *   <li>Application themes (see "milktheme.js")</li>
 * </ul>
 * 
 * Plus, it looks really cool!
 */

/*
 * Setting "options" object default values:
 * 
 * The default value is returned when "options" does not hold a value
 * for the specified key.
 * (e.g. the value was removed or was just never set)
 */

// Set the default clock face color
options.putDefaultValue(MILKTHEME_COLOR, COLORS[2]);
// Set "24 Hour Mode" as the default
options.putDefaultValue("24 Hour Mode", DEFAULT_24HOURMODE);

var DAY_NAMES = new Array(7);
DAY_NAMES[0] = DAY_NAME0;
DAY_NAMES[1] = DAY_NAME1;
DAY_NAMES[2] = DAY_NAME2;
DAY_NAMES[3] = DAY_NAME3;
DAY_NAMES[4] = DAY_NAME4;
DAY_NAMES[5] = DAY_NAME5;
DAY_NAMES[6] = DAY_NAME6;

/**
 * Indicates whether the gadget is minimized.
 * The application will need to know this because
 * it behaves and draws differently when minimized.
 */
var _minimized = false;

function view_onMinimize() {
  _minimized = true;
  updateClock();
}

function view_onRestore() {
  _minimized = false;
  // While the gadget was minimized, the caption (titlebar text)
  // was changed to display the time.
  // The regular caption must now be restored.
  view.caption = PLUGIN_NAME;
}

function view_onOpen() {
  // Set to custom menu handler.  
  // We'll be adding "AM/PM" and "24 Hour" items to the menu.
  plugin.onAddCustomMenuItems = addCustomMenuItems;
  // Set the background color of the clock face
  Face.background = options.getValue(MILKTHEME_COLOR);
  // First call to recurring timer
  onTimeout();
}

/**
 * Add the "AM/PM" and "24 Hour Mode" menu items to the gadget menu.
 */
function addCustomMenuItems(menu) {
  // Detect whether the "24 Hour Mode" option is set.
  var twentyFourMode = (options.getValue("24 Hour Mode") === true);

  // Add items to the menu.  
  menu.AddItem(MENU_AMPM,
               twentyFourMode ? 0 : gddMenuItemFlagChecked,
               onAMPM);
  menu.AddItem(MENU_24HOUR, 
               twentyFourMode ? gddMenuItemFlagChecked : 0,
               on24Hour);
}

/**
 * "AM/PM" menu item clicked handler
 */
function onAMPM() {
  // Modifying "options", the "onoptionchanged" handler will be called
  options.putValue("24 Hour Mode", false);
}

/**
 * "24 Hour Mode" menu item clicked handler
 */
function on24Hour() {
  // Modifying "options", the "onoptionchanged" handler will be called
  options.putValue("24 Hour Mode", true);
}

/**
 * Called when the "options" object has been modified. 
 * 
 * Within "main.xml",
 * this function was set as the "view" object's "onoptionchanged" handler
 */
function view_onOptionChanged() {
  switch(event.propertyName) {
    // The "24 Hour Mode" changed, update the clock
    case "24 Hour Mode":
      updateClock();
      break;
    // The theme color changed, change the face background
    case MILKTHEME_COLOR:
      Face.background = options.getValue(MILKTHEME_COLOR);
      break;
  }
}

/**
 * The main timer is processed and set here
 */
function onTimeout() {
  updateClock();

  var currentTime = new Date();
  // Timeout is the number of seconds until the next minute.
  var timeout = (61 - currentTime.getSeconds()) * 1000;
  // Set this function to be called again after the Timeout elapses.
  view.setTimeout("onTimeout()", timeout);
}

/**
 * Output the current date/time to the gadget
 */
function updateClock() {
  var currentTime = new Date();

  var currentMonth = 1 + currentTime.getMonth();
  var currentDay = currentTime.getDate();
  DateLabel.innerText = currentMonth + "/" + currentDay;

  MyWeekday.innerText = DAY_NAMES[currentTime.getDay()];

  var hours = currentTime.getHours();
  var minutes = currentTime.getMinutes();

  if (minutes < 10) {
    minutes = "0" + minutes;
  }

  if (options.getValue("24 Hour Mode") === true) {
    if (hours < 10) {
      hours = "0" + hours;
    }

    TimeLabel.innerText = hours + ":" + minutes;
    AmPm.visible = false;
  } else {
    AmPm.visible = true;

    if(hours >= 12) {
      hours -= 12;
      AmPm.innerText = PM_TEXT;
    }
    else {
      AmPm.innerText = AM_TEXT;
    }

    if(hours === 0) {
      hours = 12;
    }

    TimeLabel.innerText = hours + ":" + minutes;
  }

  // If the gadget is minimized, use the caption to display the current time
  if (_minimized) {
    view.caption = TimeLabel.innerText;
  }
}
