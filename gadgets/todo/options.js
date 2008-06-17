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

function Options_OnOpen() {
  Option_Sort.value = options(TODO_OPTIONS_SORT);
  Option_Show_Completed.value = options(TODO_OPTIONS_SHOW_COMPLETED);
}

function Options_OnOk() {
  if (options(TODO_OPTIONS_SORT) != Option_Sort.value)
    options(TODO_OPTIONS_SORT) = Option_Sort.value;
  if (options(TODO_OPTIONS_SHOW_COMPLETED) != Option_Show_Completed.value)
    options(TODO_OPTIONS_SHOW_COMPLETED) = Option_Show_Completed.value;
}