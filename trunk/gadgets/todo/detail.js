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

// Global
var dueDateCalendar = null;

// Calendar constants
var TODOCALENDAR_DAYS_IN_WEEK = 7;
var TODOCALENDAR_WEEKS_IN_VIEW = 6;

var TODOCALENDAR_MONTH_JAN = 0;
var TODOCALENDAR_MONTH_FEB = 1;
var TODOCALENDAR_MONTH_MAR = 2;
var TODOCALENDAR_MONTH_APR = 3;
var TODOCALENDAR_MONTH_MAY = 4;
var TODOCALENDAR_MONTH_JUN = 5;
var TODOCALENDAR_MONTH_JUL = 6;
var TODOCALENDAR_MONTH_AUG = 7;
var TODOCALENDAR_MONTH_SEP = 8;
var TODOCALENDAR_MONTH_OCT = 9;
var TODOCALENDAR_MONTH_NOV = 10;
var TODOCALENDAR_MONTH_DEC = 11;

var TODOCALENDAR_DAY_SUN = 0;
var TODOCALENDAR_DAY_MON = 1;
var TODOCALENDAR_DAY_TUE = 2;
var TODOCALENDAR_DAY_WED = 3;
var TODOCALENDAR_DAY_THU = 4;
var TODOCALENDAR_DAY_FRI = 5;
var TODOCALENDAR_DAY_SAT = 6;

// Month display strings will be loaded into this array
var months_strings = [];

// Day display strings will be loaded into this array
var days_strings = [];

// Strings separator
var TODO_STRINGS_SEPARATOR = ',';

function Detail_OnOpen() {
  debug.trace("Detail_OnOpen");

  // set the view based on the selected todo item data
  var todoItem = detailsViewData.getValue(TODODETAILS_DATA_STRING);
  if (todoItem != undefined) {
    Description.value = todoItem.name_;
    Completed.value = todoItem.completed_;
    setPriority(todoItem.priority_);
  }

  dueDateCalendar = new Calendar(Days, Dates);
  dueDateCalendar.initializeDisplay();
  if (todoItem.duedate_ != null) {
    dueDateCalendar.setSelectedDate(todoItem.duedate_);
  } else {
    dueDateCalendar.setDisplayDate(new Date(Date()));
  }
}

function setPriority(level) {
  switch(level) {
    case TODOITEM_PRIORITY_HIGH:
      Priority_High.value = true;
      break;

    case TODOITEM_PRIORITY_MEDIUM:
      Priority_Medium.value = true;
      break;

    default:
      Priority_Low.value = true;
      break;
  }
}

function getPriority() {
	var value = "";
	if (Priority_Low.value) {
	  value = TODOITEM_PRIORITY_LOW;
	} else if (Priority_Medium.value) {
	  value = TODOITEM_PRIORITY_MEDIUM;
	} else if (Priority_High.value) {
	  value = TODOITEM_PRIORITY_HIGH;
	}
	return value;
}

function Priority_OnChange() {
  detailsViewData.getValue(TODODETAILS_DATA_STRING).priority_ = getPriority();
}

function Priority_OnChange() {
  detailsViewData.getValue(TODODETAILS_DATA_STRING).priority_ = getPriority();
}

function Priority_OnChange() {
  detailsViewData.getValue(TODODETAILS_DATA_STRING).priority_ = getPriority();
}

function Completed_OnClick() {
  detailsViewData.getValue(TODODETAILS_DATA_STRING).completed_ =
    !detailsViewData.getValue(TODODETAILS_DATA_STRING).completed_;
}

function Description_OnChange() {
  detailsViewData.getValue(TODODETAILS_DATA_STRING).name_ = Description.value;
}

function Date_OnClick(year, month, date, weekIndex, dayIndex) {
  debug.trace(year + " " + month + " " + date + " " + weekIndex + " " +
              dayIndex)
  dueDateCalendar.setSelectedDate(new Date(year, month, date));

  detailsViewData.getValue(TODODETAILS_DATA_STRING).duedate_ =
    dueDateCalendar.selectedDate_;
}

function  CalendarBack_OnClick() {
  dueDateCalendar.setDisplayDate(
    dueDateCalendar.getPreviousMonth(dueDateCalendar.displayDate_));
}

function  CalendarFront_OnClick() {
  dueDateCalendar.setDisplayDate(
    dueDateCalendar.getNextMonth(dueDateCalendar.displayDate_));
}

// helper class to display and update the calendar control
function Calendar(daysArea, calendarArea) {
  this.daysArea_ = daysArea;
  this.calendarArea_ = calendarArea;
  this.datesArea_ = null;
  this.dates_ = null;
  this.todayArea_ = null;
  this.today = null;
  this.selectedDate_ = null; // current due date of the item
  this.displayDate_ = null; // date taken as reference for display
}

// creates the controls needed for a calendar display
Calendar.prototype.initializeDisplay = function() {
  debug.trace("Calendar.initializeDisplay");
  var dateHeight = this.calendarArea_.height / TODOCALENDAR_WEEKS_IN_VIEW;
  var dateWidth = this.calendarArea_.width / TODOCALENDAR_DAYS_IN_WEEK;
  this.datesArea_ = new Array(TODOCALENDAR_WEEKS_IN_VIEW);
  this.dates_ = new Array(TODOCALENDAR_WEEKS_IN_VIEW);
  var i = 0;
  var j = 0;

  for (i = 0; i < TODOCALENDAR_DAYS_IN_WEEK; ++i) {
    var divHtml = "<div enabled='true' x='" + dateWidth * i + "'" +
                                     " width='" + dateWidth + "'" +
                                     " height='" + this.daysArea_.height + "'" +
                                     "/>";
    var div = this.daysArea_.appendElement(divHtml);

    var labelHtml = "<label align='center' font='Tahoma' size='7' width='" +
                    dateWidth + "' height='" + dateHeight + "' " +
                    "color='#FFFFFF'/>";
    var label = div.appendElement(labelHtml);
    label.innerText = this.getDayDisplayName(i);
  }

  for (i = 0; i < TODOCALENDAR_WEEKS_IN_VIEW; ++i) {
    this.datesArea_[i] = new Array(TODOCALENDAR_DAYS_IN_WEEK);
    this.dates_[i] = new Array(TODOCALENDAR_DAYS_IN_WEEK);
    for (j = 0; j < TODOCALENDAR_DAYS_IN_WEEK; ++j) {
      var divHtml = "<div enabled='true' x='" + dateWidth * j + "'" +
                                       " y='" + dateHeight * i + "'" +
                                       " width='" + dateWidth + "'" +
                                       " height='" + dateHeight + "'" +
                                       "/>";
      this.datesArea_[i][j] = this.calendarArea_.appendElement(divHtml);

      var label = "<label align='center' font='Tahoma' size='7' width='" +
                  dateWidth + "' height='" + dateHeight + "' />";
      this.dates_[i][j] = this.datesArea_[i][j].appendElement(label);
    }
  }
}

Calendar.prototype.setSelectedDate = function(date) {
  this.selectedDate_ = new Date(date);
  Calendar_DueDate.innerText = this.getDateDisplay(this.selectedDate_);
  this.displayDate_ = new Date(date);
  this.updateDisplay();
}

Calendar.prototype.setDisplayDate = function(date) {
  this.displayDate_ = new Date(date);
  this.updateDisplay();
}

// updates the calendar display based on the display date
Calendar.prototype.updateDisplay = function() {
  debug.trace("Calendar.updateDisplay");
  debug.trace("Selected date: " + this.selectedDate_);
  debug.trace("Display date: " + this.displayDate_);

  var first = new Date(this.displayDate_.getFullYear(),
                       this.displayDate_.getMonth(), 1);
  var previousMonth = this.getPreviousMonth(this.displayDate_);
  var previousMonthStart = this.getDaysInMonth(previousMonth) -
                           first.getDay() + 1;
  var currentDate = new Date(previousMonth.getFullYear(), previousMonth.getMonth(),
                             previousMonthStart);

  if (this.todayArea_ != null) {
    this.todayArea_.removeElement(this.today_);
    this.today_ = null;
    this.todayArea_ = null;
  }

  var i = 0;
  var j = 0;
  for (i = 0; i < TODOCALENDAR_WEEKS_IN_VIEW; ++i) {
    for (j = 0; j < TODOCALENDAR_DAYS_IN_WEEK; ++j) {
      this.dates_[i][j].innerText = currentDate.getDate();

      if (currentDate.getMonth() != this.displayDate_.getMonth()) {
        this.datesArea_[i][j].opacity = 100;
      } else {
        this.datesArea_[i][j].opacity = 255;
      }

      if (DateCompare(currentDate, this.selectedDate_) == 0) {
        this.datesArea_[i][j].background = "#D4E9CF";
      } else if (currentDate.getDay() == TODOCALENDAR_DAY_SUN ||
                 currentDate.getDay() == TODOCALENDAR_DAY_SAT) {
        this.datesArea_[i][j].background = "#EEEEEE";
      } else {
        this.datesArea_[i][j].background = "#FFFFFF";
      }

      if (DateCompare(currentDate, new Date(Date())) == 0) {
        debug.trace("today: " + currentDate);
        var imgHtml = "<img src='todo_date_selected.png' width='" +
                      this.datesArea_[i][j].width + "' height='" +
                      this.datesArea_[i][j].height + "'/>";
        this.todayArea_ = this.datesArea_[i][j];
        this.today_ = this.todayArea_.appendElement(imgHtml);
      }

      this.datesArea_[i][j].onclick = "Date_OnClick(" +
        currentDate.getFullYear() + ", " +
        currentDate.getMonth() + ", " +
        currentDate.getDate() + ", " +
        i + ", " +
        j + ")";

      currentDate = this.getNextDate(currentDate);
    }
  }
  Calendar_Month.innerText =
    this.getMonthDisplayName(this.displayDate_.getMonth()) + " " +
    this.displayDate_.getFullYear();
}

Calendar.prototype.getDaysInMonth = function(date) {
  var days = 0;
  if (date.getMonth() == TODOCALENDAR_MONTH_FEB) {
    if ((date.getFullYear() % 4 == 0 && date.getFullYear() % 100 != 0) ||
        date.getFullYear() % 400 == 0)
      days = 29;
    else
      days = 28;
  } else if (date.getMonth() == TODOCALENDAR_MONTH_APR ||
             date.getMonth() == TODOCALENDAR_MONTH_JUN ||
             date.getMonth() == TODOCALENDAR_MONTH_SEP ||
             date.getMonth() == TODOCALENDAR_MONTH_NOV) {
    days = 30;
  } else {
    days = 31;
  }

  return days;
}

Calendar.prototype.getPreviousMonth = function(date) {
  var previous = new Date(date);
  if (date.getMonth() == TODOCALENDAR_MONTH_JAN) {
    previous.setMonth(TODOCALENDAR_MONTH_DEC);
    previous.setYear(date.getFullYear() - 1);
  } else {
    var previousMonthFirst = new Date(date.getFullYear(), date.getMonth() - 1, 1);
    var daysInPreviousMonth = this.getDaysInMonth(previousMonthFirst);
    if (date.getDate() > daysInPreviousMonth) {
      previous.setMonth(date.getMonth() - 1, daysInPreviousMonth);
    } else {
      previous.setMonth(date.getMonth() - 1);
    }
  }
  return previous;
}

Calendar.prototype.getNextMonth = function(date) {
  var next = new Date(date);
  next.setDate(date.getDate() + this.getDaysInMonth(date));
  return next;
}

Calendar.prototype.getNextDate = function(date) {
  var next = new Date(date);
  next.setDate(date.getDate() + 1);
  return next;
}

Calendar.prototype.getMonthDisplayName = function(month) {
  var name = null;
  if (months_strings.length == 0) {
  	// load month display strings if not available
	  var months_collated_string = MONTHS_SHORT_STR;
	  months_strings = months_collated_string.split(TODO_STRINGS_SEPARATOR);
  }
  if (month >= TODOCALENDAR_MONTH_JAN && month <= TODOCALENDAR_MONTH_DEC)
    name = months_strings[month];
  return name;
}

Calendar.prototype.getDayDisplayName = function(day) {
  var name = null;
  if (days_strings.length == 0) {
  	// load day display strings if not available
	  var days_collated_string = DAYS_SHORT_STR;
	  days_strings = days_collated_string.split(TODO_STRINGS_SEPARATOR);
  }
  if (day >= TODOCALENDAR_DAY_SUN && day <= TODOCALENDAR_DAY_SAT)
    name = days_strings[day];
  return name;
}

Calendar.prototype.getDateDisplay = function(date) {
  // TODO: i18n
  return ((date.getMonth() + 1) + "/" + date.getDate() + "/" + date.getFullYear());
}

