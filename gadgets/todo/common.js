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

// options and details view data constant strings
var TODOLIST_OPTIONS_STRING = "TodoList";
var TODODETAILS_INDEX_STRING = "TodoItemIndex";
var TODODETAILS_DATA_STRING = "TodoItemData";
var TODO_OPTIONS_SORT = "TodoSort";
var TODO_OPTIONS_SHOW_COMPLETED = "TodoShowCompleted";

// priority constants
var TODOITEM_PRIORITY_LOW = "low";
var TODOITEM_PRIORITY_MEDIUM = "medium";
var TODOITEM_PRIORITY_HIGH = "high";

// utils function to compare 2 dates
function DateCompare(date1, date2) {
  if (date1 == null || date2 == null) {
    if (date1 == null) {
      return 1;
    } else if (date2 == null) {
      return -1;
    } else {
      return 0;
    }
  }
  
  if (date1.getFullYear() != date2.getFullYear())
    return (date1.getFullYear() > date2.getFullYear())? 1: -1;

  if (date1.getMonth() != date2.getMonth())
    return (date1.getMonth() > date2.getMonth())? 1: -1;

  if (date1.getDate() != date2.getDate())
    return (date1.getDate() > date2.getDate())? 1: -1;

  return 0;
}
