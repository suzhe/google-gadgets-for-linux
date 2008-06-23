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

// Globals
var todoList = null;
var todoListDisplay = null;
var todoDetails = null;
var selectedItemIndex = -1;
var moveItemToIndex = -1;

var todoPreviousWidth = 200;
var todoPreviousHeight = 200;

// todoitem slide animation time (in millisecs)
var TODOITEM_SLIDE_DURATION = 200;

// ASCII values for keypresses
var TODO_KEYPRESS_ENTER = 13;
var TODO_KEYPRESS_ESCAPE = 27;

// todo list options string separators
var TODOLIST_ITEMS_SEPARATOR = '|';
var TODOLIST_PROPERTIES_SEPARATOR = ',';

// minimum dimensions for the gadget
var TODO_MINIMUM_WIDTH = 150;
var TODO_MINIMUM_HEIGHT = 80;

// default options
options.putDefaultValue(TODO_OPTIONS_SORT, true);
options.putDefaultValue(TODO_OPTIONS_SHOW_COMPLETED, true);


function view_onOpen() {
  debug.trace("view_onOpen()");
  
  HandleResize(view.width, view.height);

  // read the current todo items from the options
  todoList = new TodoList();
  todoList.restore();
  if (todoList.todoList_ != undefined && options(TODO_OPTIONS_SORT)) 
    todoList.todoList_.sort(TodoItemCompare);

  // display the current todo items
  todoListDisplay = new TodoListDisplay(TodoItems);
  for (var i in todoList.todoList_) {
    todoListDisplay.addTodoItemDisplay(todoList.todoList_[i]);
  }
}

function AddItem_OnKeyPress() {
  if (event.keyCode == TODO_KEYPRESS_ENTER ||
      event.keyCode == TODO_KEYPRESS_ESCAPE) {
    // add a new todo item, if enter was hit
    if (event.keyCode == TODO_KEYPRESS_ENTER) {
      EnsureDetailsViewIsClosed();
      
      var newTodoItem = new TodoItem(AddItem.value, false, null, "low");
      todoList.todoList_.push(newTodoItem);
      todoListDisplay.addTodoItemDisplay(newTodoItem);
      todoList.refreshItemPosition(todoList.todoList_.length - 1, true);
    }
    // ignore the entry, if escape was hit
    AddItem.value = "";
    AddItem.killFocus();
  }
}

function AddItem_OnFocusIn() {
  debug.trace("AddItem_OnFocusIn()");
  AddItem.value = "";
  AddItem.color = "#000000";
  AddItem.background = "#FFFFFF";
}

function AddItem_OnFocusOut() {
  debug.trace("AddItem_OnFocusOut()");
  AddItem.value = ADD_ITEM_STRING;
  AddItem.color = "#666666";
  AddItem.background = "#F5F5F5";
}

function AddItem_OnMouseOver() {
  debug.trace("AddItem_OnMouseOver()");
  AddItem.background = "#FFFFFF";
}

function AddItem_OnMouseOut() {
  debug.trace("AddItem_OnMouseOut()");
  if (AddItem.value == ADD_ITEM_STRING)
    AddItem.background = "#F5F5F5";
}

function TodoItemDiv_OnClick(index) {
  debug.trace("TodoItemDiv_OnClick: " + todoList.todoList_[index].name_)
  
  var openDetailsView = (todoDetails == null ||
    todoDetails.detailsViewData.getValue(TODODETAILS_INDEX_STRING) != index);

  EnsureDetailsViewIsClosed();

  if (openDetailsView) {
    // no details view for this item - open a new one
    debug.trace("opening details view");

    todoListDisplay.todoListDisplayItems_[index].item_.background = "#FFDDF4FD";

    todoDetails = new DetailsView();
    todoDetails.contentIsView = true;
    todoDetails.detailsViewData.putValue(TODODETAILS_DATA_STRING,
                                         todoList.todoList_[index]);
    todoDetails.detailsViewData.putValue(TODODETAILS_INDEX_STRING, index);
    todoDetails.setContent(undefined, undefined, "detail.xml", false, 0);

    pluginHelper.showDetailsView(todoDetails, DETAILS_TITLE, 
                                 gddDetailsViewFlagNone,
                                 ProcessDetailsViewFeedback);
  }
}

function TodoItemDiv_OnDblClick(index) {
  debug.trace("TodoItemDiv_OnDblClick: " + todoList.todoList_[index].name_)

  // show the inplace edit box and focus to it, to edit the item name
  var nameEdit = todoListDisplay.todoListDisplayItems_[index].nameEdit_;
  var name = todoListDisplay.todoListDisplayItems_[index].name_;
  nameEdit.value = name.innerText;
  nameEdit.height = name.offsetHeight;
  nameEdit.visible = true;
  nameEdit.focus();
}

// mouse event handlers to set the background based on mouse movements

function TodoItemDiv_OnMouseOver(index) {
  todoListDisplay.todoListDisplayItems_[index].item_.background = "#DDF4FD";
}

function TodoItemDiv_OnMouseOut(index) {
  if (!todoDetails ||
      todoDetails.detailsViewData(TODODETAILS_INDEX_STRING) != index) {
    todoListDisplay.todoListDisplayItems_[index].item_.background = "#00FFFFFF";
  }
}

function TodoItemDiv_OnMouseDown(index) {
  todoListDisplay.todoListDisplayItems_[index].item_.background = "#B9D4E1";
  
  if (!options(TODO_OPTIONS_SORT)) {
    // capture item index for drag/drop
    selectedItemIndex = index;
    moveItemToIndex = index;
  }
}

function TodoItemDiv_OnMouseUp(index) {
  todoListDisplay.todoListDisplayItems_[index].item_.background = "#B9D4E1";
  
  if (selectedItemIndex != -1 &&
      moveItemToIndex != -1 && 
      selectedItemIndex != moveItemToIndex) {
    // move the dragged item to the dropped position
    EnsureDetailsViewIsClosed();
    UpdateDividerOnDrag(index, -1);

    todoList.moveItem(selectedItemIndex, moveItemToIndex);
    todoListDisplay.moveItem(selectedItemIndex, moveItemToIndex);
    todoList.save();
  }
  
  // reset drag/drop indices
  selectedItemIndex = -1;
  moveItemToIndex = -1;
}

function TodoItemDiv_OnMouseMove(index) {
  if (selectedItemIndex != -1) {
    // if we are dragging an item, update the divider to show drop position
    var newMoveIndex = todoListDisplay.getClosestIndex(event.y, index);
    UpdateDividerOnDrag(index, newMoveIndex);
    moveItemToIndex = newMoveIndex;
    debug.trace("closest index to move to: " + moveItemToIndex);
  }
}

function UpdateDividerOnDrag(from, to) {
  var items = todoListDisplay.todoListDisplayItems_;
  
  if (to == moveItemToIndex)
    return;
  
  if (moveItemToIndex != -1) {
    var lastDividerPosition = (from < moveItemToIndex)? 
                              moveItemToIndex: 
                              moveItemToIndex - 1;
   if (lastDividerPosition >= 0)
     items[lastDividerPosition].divider_.opacity = 80;
  }
    
  if (to != -1) {
    var dividerPosition = (from < to)? to: to - 1;
    if (dividerPosition >= 0)
      items[dividerPosition].divider_.opacity = 255;
  }
}

function NameEdit_OnKeyPress(index) {
  if (event.keyCode == TODO_KEYPRESS_ENTER ||
      event.keyCode == TODO_KEYPRESS_ESCAPE) {
    var nameEdit = todoListDisplay.todoListDisplayItems_[index].nameEdit_;
    // modify the todo item, if enter was hit
    if (event.keyCode == TODO_KEYPRESS_ENTER) {
      EnsureDetailsViewIsClosed();
      
      var editTodoItem = todoList.todoList_[index];
      editTodoItem.name_ = nameEdit.value;
      todoListDisplay.updateItemDisplay(index, editTodoItem, false);
      todoList.refreshItemPosition(todoList.todoList_.length - 1, true);
    }
    // ignore the entry, if escape was hit
    nameEdit.value = "";
    nameEdit.killFocus();
  }
}

function NameEdit_OnFocusOut(index) {
  todoListDisplay.todoListDisplayItems_[index].nameEdit_.visible = false;
}

function Completed_OnClick(index) {
  debug.trace("Completed_OnClick:" + todoList.todoList_[index].name_)

  EnsureDetailsViewIsClosed();
  
  var todoItem = todoList.todoList_[index];
  var itemHeight = todoListDisplay.todoListDisplayItems_[index].item_.height;
  todoItem.completed_ = !todoItem.completed_;
  todoListDisplay.updateItemDisplay(index, todoItem, false);
  todoList.refreshItemPosition(index, false);

  if (!options(TODO_OPTIONS_SHOW_COMPLETED)) {
    todoListDisplay.moveItems(index + 1, todoList.todoList_.length, 
                              -(itemHeight));
  }
}

function PageBack_OnClick() {
  debug.trace("PageBack_OnClick");
  EnsureDetailsViewIsClosed();
  todoListDisplay.previousPage();
}

function PageFront_OnClick() {
  debug.trace("PageFront_OnClick");
  EnsureDetailsViewIsClosed();
  todoListDisplay.nextPage();
}

function View_OnSizing() {
  debug.trace("View_OnSizing");
  EnsureDetailsViewIsClosed();
  
  if (event.width < TODO_MINIMUM_WIDTH ||
      event.height < TODO_MINIMUM_HEIGHT) {
    event.returnValue = false;
  } else {
    HandleResize(event.width, event.height);
  }
}

function View_OnOptionChanged() {
  debug.trace("View_OnOptionChanged");

  if (event.propertyName == TODO_OPTIONS_SORT ||
      event.propertyName == TODO_OPTIONS_SHOW_COMPLETED) {
    if (options(TODO_OPTIONS_SORT)) 
      todoList.todoList_.sort(TodoItemCompare);
    todoListDisplay.refreshDisplay(todoList.todoList_);
  }
}

// handles display resize 
function HandleResize(width, height) {
  var dWidth = width - todoPreviousWidth;
  var dHeight = height - todoPreviousHeight;

  if (dWidth != 0)
    UpdateControlsOnWidthResize(dWidth);

  if (dHeight != 0)
    UpdateControlsOnHeightResize(dHeight);

  todoPreviousWidth = width;
  todoPreviousHeight = height;
}

// update controls for the new width resize
function UpdateControlsOnWidthResize(dWidth) {
  AddItem.width += dWidth;
  Backupground_Top.width += dWidth;
  Backupground_TopRight.x += dWidth;
  Backupground_Center.width += dWidth;
  Backupground_Right.x += dWidth;
  Backupground_Bottom.width += dWidth;
  Backupground_BottomRight.x += dWidth;
  Footer_Center.width += dWidth;
  Footer_Right.x += dWidth;
  Divider.width += dWidth;
  Page_Control.x += dWidth / 2;
  
  if (todoListDisplay != null) {
    todoListDisplay.area_.width += dWidth;
    todoListDisplay.listArea_.width += dWidth;
    todoListDisplay.refreshDisplay(todoList.todoList_);
  }
}

// update controls for the new height resize
function UpdateControlsOnHeightResize(dHeight) {
  Backupground_Left.height += dHeight;
  Backupground_Center.height += dHeight;
  Backupground_Right.height += dHeight;
  Backupground_BottomLeft.y += dHeight;
  Backupground_Bottom.y += dHeight;
  Backupground_BottomRight.y += dHeight;
  Footer_Left.y += dHeight;
  Footer_Center.y += dHeight;
  Footer_Right.y += dHeight;
  Page_Control.y += dHeight;

  if (todoListDisplay != null) {
    todoListDisplay.area_.height += dHeight;
    todoListDisplay.listArea_.height += dHeight;
    todoListDisplay.refreshDisplay(todoList.todoList_);
  }
}

// checks if the details view is open and closes it if it is open
function EnsureDetailsViewIsClosed() {
  if (todoDetails != null)
    pluginHelper.closeDetailsView();
}

// called when the details view is closed
function ProcessDetailsViewFeedback() {
  var todoItem = todoDetails.detailsViewData.getValue(TODODETAILS_DATA_STRING);
  var index = todoDetails.detailsViewData.getValue(TODODETAILS_INDEX_STRING);
  debug.trace("ProcessDetailsViewFeedback:" + todoList.todoList_[index].name_)

  // update the main list with the edited data
  todoListDisplay.todoListDisplayItems_[index].item_.background = "#00FFFFFF";
  todoList.todoList_[index] = todoItem;
  todoListDisplay.updateItemDisplay(index, todoItem, true);
  todoList.refreshItemPosition(index, false);
  todoDetails.detailsViewData.removeAll();
  todoDetails = null;
}

// helper class to store data regarding a TodoItem
function TodoItem(name, completed, duedate, priority) {
  this.name_ = name;
  this.completed_ = completed;
  this.duedate_ = duedate;
  this.priority_ = priority;
}

// saves the data into a string
TodoItem.prototype.save = function() {
  var todoItemString = escape(this.name_) + "," +
                       escape(this.completed_) + "," +
                       (this.duedate_ != null? escape(this.duedate_): "") +
                       "," + escape(this.priority_);
  return todoItemString;
}

// restores data from a string
TodoItem.prototype.restore = function(oldTodoItemString) {
  var oldTodoItemMemberStrings = oldTodoItemString.split(",");
  if (oldTodoItemMemberStrings.length == 4) {
    this.name_ = unescape(oldTodoItemMemberStrings[0]);
    this.completed_ = unescape(oldTodoItemMemberStrings[1]) == "true"?
                      true: false;
    this.duedate_ = oldTodoItemMemberStrings[2] != ""?
                    new Date(unescape(oldTodoItemMemberStrings[2])): null;
    this.priority_ = unescape(oldTodoItemMemberStrings[3]);
  }
  debug.trace("restored item: " + this.name_);
}

// helper class to store data regarding a TodoList
function TodoList() {
  this.todoList_ = [];
}

TodoList.prototype.add = function(todoItem) {
  this.todoList_.push(todoItem);
}

// saves the list data into a string
TodoList.prototype.save = function() {
  debug.trace("TodoList.save()");

  var todoListString = "";
  for (var i in this.todoList_) {
    todoListString += this.todoList_[i].save();
    todoListString += TODOLIST_ITEMS_SEPARATOR; // '|' is the separator we use
  }
  options.putValue(TODOLIST_OPTIONS_STRING, todoListString);
  debug.trace("Saved Todo List: " + todoListString);
}

// restores the list data from a string
TodoList.prototype.restore = function() {
  debug.trace("TodoList.restore()");
  var oldTodoListString = options(TODOLIST_OPTIONS_STRING);
  debug.trace("Restoring Todo List: " + oldTodoListString);
  if (oldTodoListString != null) {
    var oldTodoItemStrings = oldTodoListString.split(
      TODOLIST_ITEMS_SEPARATOR);
    for (var i in oldTodoItemStrings) {
      var item = oldTodoItemStrings[i];
      debug.trace(item);
      var newTodoItem = new TodoItem();
      if (item != "") {
        newTodoItem.restore(item);
        this.add(newTodoItem);
      }
    }
  }
}

// move item in the list
TodoList.prototype.moveItem = function(from, to) {
  var moveTodoItem = todoList.todoList_[from];
  todoList.todoList_.splice(from, 1);
  todoList.todoList_.splice(to, 0, moveTodoItem);
}

// repositions the display items based on updates
TodoList.prototype.refreshItemPosition = function(index, newItem) {
  debug.trace("TodoList.refreshItemPosition: " + 
              todoList.todoList_[index].name_);

  var sort = options(TODO_OPTIONS_SORT);
  if (newItem || sort) {
    var todoItem = todoList.todoList_[index];
    var newIndex = 0;

    if (sort) {
      todoList.todoList_.sort(TodoItemCompare);

      for (var i in todoList.todoList_) {
        if (TodoItemCompare(todoList.todoList_[i], todoItem) >= 0)
          break;
        ++newIndex;
      }
    } else if (newItem){
      todoList.moveItem(index, newIndex);
    }
    
    todoListDisplay.moveItem(index, newIndex);
  }

  todoList.save();
}

// helper function to compare to todoitems
function TodoItemCompare(todoItem1, todoItem2) {
  // first on if it is completed or not
  if (todoItem1.completed_ != todoItem2.completed_) {
    return (todoItem2.completed_? -1 : 1);
  }

  // next on priority
  if (todoItem1.priority_ != todoItem2.priority_) {
    if ((todoItem1.priority_ == TODOITEM_PRIORITY_HIGH) ||
        (todoItem1.priority_ == TODOITEM_PRIORITY_MEDIUM &&
         todoItem2.priority_ == TODOITEM_PRIORITY_LOW)) {
      return -1;
    } else {
      return 1;
    }
  }

  // then on the duedate
  if (todoItem1.duedate_ != todoItem2.duedate_) {
    var dateComparison = DateCompare(todoItem1.duedate_,todoItem2.duedate_);
    if (dateComparison != 0)
      return dateComparison;
  }

  // finally on the name
  if (todoItem1.name_ > todoItem2.name_) {
    return 1;
  } else if (todoItem1.name_ < todoItem2.name_) {
    return -1;
  }

  return 0;
}

// REMOVE_GADGET_BEGIN
// TODO(v6:arasu): Need to support 2 line view
// REMOVE_GADGET_END

// helper class to hold display elements of an item
function TodoItemDisplay(item, priority, completed, name, nameEdit, duedate, 
                         divider) {
  this.item_ = item;
  this.priority_ = priority;
  this.completed_ = completed;
  this.name_ = name;
  this.nameEdit_ = nameEdit;
  this.duedate_ = duedate;
  this.divider_ = divider;
}

// helper class to hold display elements of a list
function TodoListDisplay(divArea) {
  this.area_ = divArea;
  this.listArea_ = this.area_.appendElement("<div x='1' y='0' />");
  this.currentY_ = 0;
  this.todoListDisplayItems_ = [];
  this.listArea_.width = this.area_.width - 2;
  this.listArea_.height = 0;
}

// adds an item to the bottom of the display
TodoListDisplay.prototype.addTodoItemDisplay = function(todoItem) {
  debug.trace("Adding todo item display: " + todoItem.name_);
  var index = this.todoListDisplayItems_.length;
  var i = "<div enabled='true' dropTarget='true' x='0' height='24' />";
  var p = "<img width='8' y='0' />";
  var c = "<checkbox height='14' width='14' " +
          "downImage='todo_chk_unsel_down.png' " +
          "image='todo_chk_unsel_up.png' " +
          "overImage='todo_chk_unsel_over.png' " +
          "checkedDownImage='todo_chk_sel_down.png' " +
          "checkedImage='todo_chk_sel_up.png' " +
          "checkedOverImage='todo_chk_sel_over.png' " +
          "x='10' y='5' />";
  var n = "<label size='8' font='Tahoma' align='left' x='30' y='5' " +
          "height='14' trimming='character-ellipsis'/>";
  var e = "<edit font='Tahoma' size='8' align='left' x='28' y='5' " +
          "visible='false' />";
  var d = "<label font='Tahoma' size='8' align='right' y='5' opacity='150'/>";
  var dv = "<img opacity='80' src='todo_divider.png' />";

  var item = this.listArea_.appendElement(i);
  var priority = item.appendElement(p);
  var completed = item.appendElement(c);
  var name = item.appendElement(n);
  var duedate = item.appendElement(d);
  var nameEdit = item.appendElement(e);
  var divider = item.appendElement(dv);

  this.listArea_.height += item.height;
  this.setEventHandlers(item, completed, nameEdit, index);

  item.y = this.currentY_;
  item.width = this.listArea_.width;
  name.width = item.width - name.x;
  duedate.x = item.width - duedate.width;
  divider.width = item.width;
  duedate.onsize = function() {
    duedate.x = item.width - duedate.offsetWidth;
    name.width = item.width - name.x - duedate.offsetWidth - 3;
  };

  var itemDisplay = new TodoItemDisplay(item, priority, completed, name,
                                        nameEdit, duedate, divider);
  this.todoListDisplayItems_.push(itemDisplay);
  this.updateItemDisplay(this.todoListDisplayItems_.length - 1, todoItem,
                         true);

  divider.y = item.height - 1;
  this.currentY_ += item.height;
  
  this.updatePageDisplay();
}

// helper to get the display formatted string for a date
TodoListDisplay.prototype.getDisplayDate = function(date) {
  if (date == null)
    return "";

  var today = new Date();
  if ((today.getFullYear() == date.getFullYear()) &&
      (today.getMonth() == date.getMonth())) {
    if ((today.getDate() + 1) == date.getDate()) {
      return DATE_TOMORROW;
    } else if ((today.getDate() - 1) == date.getDate()) {
      return DATE_YESTERDAY;
    } else if (today.getDate() == date.getDate()) {
      return DATE_TODAY;
    }
  }
  return (date.getMonth() + 1) + "/" + date.getDate();
}

TodoListDisplay.prototype.getPriorityImage = function(level) {
  var img = "";
  switch(level) {
    case "high":
      img = "todo_priority_high.png";
      break;

    case "medium":
      img = "todo_priority_medium.png";
      break;
  }
  return img;
}

// updates the display attributes of an item based on its data
TodoListDisplay.prototype.updateItemDisplay = function(index, todoItem,
                                                       setCompletedCheckbox) {
  var item = this.todoListDisplayItems_[index].item_;
  var name = this.todoListDisplayItems_[index].name_;
  var duedate = this.todoListDisplayItems_[index].duedate_;
  var priority = this.todoListDisplayItems_[index].priority_;
  var completed = this.todoListDisplayItems_[index].completed_;
  var originalItemHeight = item.height;
  
  item.visible = false;
  item.height = 0;

  if (setCompletedCheckbox) {
    this.todoListDisplayItems_[index].completed_.value =
      todoItem.completed_;
  }
  this.todoListDisplayItems_[index].priority_.src =
    this.getPriorityImage(todoItem.priority_);
  name.innerText = todoItem.name_;

  if (todoItem.completed_) {
    name.opacity = 150;
    name.strikeout = true;
    name.color = "#000000";
    duedate.innerText = "";
    duedate.color = "#000000";
  } else {
    name.opacity = 255;
    name.strikeout = false;
    if (DateCompare(todoItem.duedate_, new Date(Date())) < 0) {
      name.color = "#990000"
      duedate.color = "#990000"
    } else {
      name.color = "#000000"
      duedate.color = "#000000"
    }
    duedate.innerText = this.getDisplayDate(todoItem.duedate_);
  }
  
  if (!todoItem.completed_ || options(TODO_OPTIONS_SHOW_COMPLETED)) {
    item.height = 24;
    item.visible = true;
  }

  priority.height = item.height;
  this.listArea_.height += (item.height - originalItemHeight);
}

// slide an item from one position to another
TodoListDisplay.prototype.moveItem = function(from, to) {
  debug.trace("move from " + from + " to " + to);
  if (from == to)
    return;

  var items = this.todoListDisplayItems_;

  var fromItem = items[from];
  var i = 0;
  var dIndex = (to > from)? 1: -1;
  for (i = from; i != to; i += dIndex) {
    items[i] = items[i + dIndex];
    this.setEventHandlers(items[i].item_, items[i].completed_, 
                          items[i].nameEdit_, i);
  }
  items[to] = fromItem;
  this.setEventHandlers(items[to].item_, items[to].completed_, 
                        items[i].nameEdit_, to);

  var toY = (to > from)?
               items[to - dIndex].item_.y + items[to - dIndex].item_.height -
                 items[to].item_.height:
               items[to - dIndex].item_.y;
  beginAnimation("todoListDisplay.todoListDisplayItems_[" + to + "].item_.y = "
                 + "event.value", items[to].item_.y, toY,
                 TODOITEM_SLIDE_DURATION);
  
  this.moveItems(from, to, -(dIndex * items[to].item_.height));
}

// slide a block of items
TodoListDisplay.prototype.moveItems = function(startIndex, endIndex, dy) {
  var items = this.todoListDisplayItems_;
  var dIndex = (dy > 0)? -1: 1;
  var i = startIndex;
  while (i != endIndex) {
    toY = items[i].item_.y + dy;
    beginAnimation("todoListDisplay.todoListDisplayItems_[" + i + "].item_.y = "
                   + "event.value", items[i].item_.y, toY,
                   TODOITEM_SLIDE_DURATION);
    i += dIndex;
  }
}

// set event handlers for the item div and the completed checkbox
TodoListDisplay.prototype.setEventHandlers = function(item, completed, nameEdit, 
                                                      index) {
  item.onclick = "TodoItemDiv_OnClick(" + index + ")";
  item.ondblclick = "TodoItemDiv_OnDblClick(" + index + ")";
  item.onmouseover = "TodoItemDiv_OnMouseOver(" + index + ")";
  item.onmouseout = "TodoItemDiv_OnMouseOut(" + index + ")";
  item.onmousedown = "TodoItemDiv_OnMouseDown(" + index + ")";
  item.onmouseup = "TodoItemDiv_OnMouseUp(" + index + ")";
  item.onmousemove = "TodoItemDiv_OnMouseMove(" + index + ")";
  nameEdit.onkeypress = "NameEdit_OnKeyPress(" + index + ")";
  nameEdit.onfocusout= "NameEdit_OnFocusOut(" + index + ")";
  completed.onclick = "Completed_OnClick(" + index + ")";
}

// remove all items and rebuild the display
TodoListDisplay.prototype.refreshDisplay = function(todoItems) {
  debug.trace("TodoListDisplay.refreshDisplay");
  
  this.listArea_.y = 0;
  this.listArea_.height = 0;
  this.currentY_ = 0;

  var i = 0;
  this.listArea_.removeAllElements();
  this.todoListDisplayItems_ = [];

  for (i = 0; i < todoItems.length; ++i)
    this.addTodoItemDisplay(todoItems[i]);
}

// slide to previous page, if available
TodoListDisplay.prototype.previousPage = function() {
  if (todoListDisplay.listArea_.y >= 0)
    return;
  
  // move the listarea div down to show the previous page
  var toY = todoListDisplay.listArea_.y + this.area_.height;
  if (toY > 0)
    toY = 0;
    
  beginAnimation("todoListDisplay.listArea_.y = event.value;", 
    todoListDisplay.listArea_.y, toY, TODOITEM_SLIDE_DURATION);

  if (Current_Page.innerText != "1")
    Current_Page.innerText = parseInt(Current_Page.innerText) - 1;
}

// slide to next page, if available
TodoListDisplay.prototype.nextPage = function() {
  if (todoListDisplay.listArea_.height + todoListDisplay.listArea_.y <=
      this.area_.height)
    return;

  // move the listarea div up to show the next page
  var toY = todoListDisplay.listArea_.y - this.area_.height;
  
  beginAnimation("todoListDisplay.listArea_.y = event.value;", 
    todoListDisplay.listArea_.y, toY, TODOITEM_SLIDE_DURATION);
    
  if (Current_Page.innerText != Total_Pages.innerText)
    Current_Page.innerText = parseInt(Current_Page.innerText) + 1;
}

// calculate and set the total pages and the current page
TodoListDisplay.prototype.updatePageDisplay = function() {
  var totalPages = parseInt(this.listArea_.height / this.area_.height);
  if (this.listArea_.height % this.area_.height > 0)
    ++totalPages;
    
  var currentPage = ((-this.listArea_.y) / this.area_.height) + 1;
  
  Total_Pages.innerText = totalPages;
  Current_Page.innerText = currentPage;
}

// get the item index closest to the specifed y coordinate offset 
TodoListDisplay.prototype.getClosestIndex = function(y, index) {
  var closestIndex = -1;
  var dIndex = (y > 0)? 1: -1;
  var i = 0;
  
  for (i = index; 
       i >= 0 && i < this.todoListDisplayItems_.length; 
       i += dIndex) {
    var dStartY = this.todoListDisplayItems_[i].item_.y - 
                  this.todoListDisplayItems_[index].item_.y;
    var dEndY = (this.todoListDisplayItems_[i].item_.y + 
                  this.todoListDisplayItems_[i].item_.height) - 
                this.todoListDisplayItems_[index].item_.y;
    if (y >= dStartY && y <= dEndY) {
      var dMidY = (dStartY + dEndY) / 2;
      closestIndex = i;
      if (closestIndex == index) {
        break;
      } else if (closestIndex < index && y >= dMidY) {
        ++closestIndex;
      } else if (closestIndex > index && y < dMidY) {
        --closestIndex;
      }
    }
  }
  return closestIndex;
}
