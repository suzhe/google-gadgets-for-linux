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

var kMinX = -2000;
var kMaxX = 2000;
var kMinY = -2000;
var kMaxY = 2000;
var kMinWidth = 0;
var kMinHeight = 0;
var kMaxWidth = 2000;
var kMaxHeight = 2000;
var kSelectionBorderWidth = 6;

var g_selected_element = null;
var g_drag_offset_x = 0;
var g_drag_offset_y = 0;
var g_offset_angle = 0;
var g_dragging = false;
var g_keyboard_sizing = false;
var g_size_width_dir = 0;
var g_size_height_dir = 0;
var g_current_tool = -1;
var g_gadget_file_manager = null;
var g_gadget_file = null;
var g_current_file = null;
var g_current_file_time = 0;
var g_changed = false;
var g_extracted_files = { };
var g_extracted_files_time = { };
var g_global_file_manager = designerUtils.getGlobalFileManager();

var kTools = [
  "div", "img", "button", "edit", "checkbox", "radio", "a", "label",
  "contentarea", "listbox", "combobox", "progressbar", "scrollbar"
];

var g_elements_view = null;
var g_properties_view = null;
var g_events_view = null;

// TODO: config.
var g_system_editors = {
  xml: "gedit",
  js: "gedit",
  gmanifest: "gedit",
  png: "gimp",
  gif: "gimp",
  jpeg: "gimp",
  jpg: "gimp"
};

function ResetGlobals() {
  ResetViewGlobals();
  g_drag_offset_x = 0;
  g_drag_offset_y = 0;
  g_offset_angle = 0;
  g_dragging = false;
  g_keyboard_sizing = false;
  g_size_width_dir = 0;
  g_size_height_dir = 0;
  SelectTool(-1);
  SelectElement(null);
  e_designer_view.removeAllElements();
  e_properties_list.removeAllElements();
  e_events_list.removeAllElements();
  g_elements_view = null;
  g_properties_view = null;
  g_events_view = null;
  g_changed = false;7
  g_current_file = null;
  g_current_file_time = 0;
}

function view_onopen() {
  InitMetadata();
  plugin.onAddCustomMenuItems = AddGlobalMenu;
  view_onsize();
  InitToolBar();
  e_selection.oncontextmenu = AddElementContextMenu;
  for (var i = 0; i < e_selection.children.count; i++)
    e_selection.children.item(i).oncontextmenu = AddElementContextMenu;
}

function view_onsize() {
  e_toolbar.width = view.width - e_tool_views.offsetWidth;
  e_designer.width = view.width - e_tool_views.offsetWidth;
  e_designer.height = view.height - e_toolbar.offsetHeight;
}

function view_onmouseover() {
  CheckExternalFileChange();
}

function view_oncontextmenu() {
  AddGlobalMenu(event.menu);
  event.returnValue = false;
}

function tool_label_onsize(index) {
  var label = event.srcElement;
  var tool = label.parentElement;
  tool.width = Math.max(label.offsetWidth + 6, 30);
  if (index == 0) {
    tool.x = 0;
  } else {
    var prev_tool = e_toolbar.children.item(index - 1);
    tool.x = prev_tool.x + prev_tool.width;
  }
}

function file_list_onchange() {
  var item = e_file_list.selectedItem;
  if (item) {
    var filename = item.children.item(0).innerText;
    if (filename && filename != g_current_file)
      OpenFile(filename);
  }
}

function designer_onsize() {
  if (!g_dragging) {
    e_designer_view.x =
        Math.max(0, Math.round((e_designer.offsetWidth -
                                e_designer_view.offsetWidth) / 2));
    e_designer_view.y =
        Math.max(0, Math.round((e_designer.offsetHeight -
                                e_designer_view.offsetHeight) / 2));
    view.setTimeout(UpdateSelectionPos, 0);
  }
}

function properties_list_onchange() {
  g_properties_view.OnSelectionChange();
}

function events_list_onchange() {
  g_events_view.OnSelectionChange();
}

function elements_list_onchange() {
  if (e_elements_list.selectedItem) {
    var id = g_elements_view.GetIdByNode(e_elements_list.selectedItem);
    SelectElement(g_elements_info[id]);
  } else {
    SelectElement(null);
  }
}

function designer_panel_onmousedown() {
  if (event.button == 1) {
    var x = event.x - e_designer_view.offsetX;
    var y = event.y - e_designer_view.offsetY;
    if (g_current_tool != -1) {
      SelectElement(NewElement(kTools[g_current_tool], x, y));
      g_changed = true;
      SelectTool(-1);
    } else {
      var element = FindElementByPos(e_designer_view, x, y);
      if (element) {
        g_drag_offset_x = x;
        g_drag_offset_y = y;
        SelectElement(GetElementInfo(element));
        e_designer_panel.cursor = "sizeall";
        g_dragging = true;
      } else {
        SelectElement(null);
      }
    }
  }
}

function designer_panel_onmousemove() {
  if (g_dragging && !(event.button & 1)) {
    e_designer_panel.cursor = "arrow";
    EndDragging();
  }
  if (g_selected_element && g_dragging) {
    DragMove(event.x - e_designer_view.offsetX,
             event.y - e_designer_view.offsetY);
  }
}

function designer_panel_onmouseup() {
  designer_panel_onmousemove();
  e_designer_panel.cursor = "arrow";
  EndDragging();
}

function selection_onkeydown() {
  var size_move_func = g_keyboard_sizing ?
                       ResizeSelectedElement : MoveSelectedElement;
  switch (event.keyCode) {
    case 16: // shift
      g_keyboard_sizing = true;
      break;
    case 37: // left
    case 0x64: // numpad 4
      size_move_func(-1, 0);
      break;
    case 38: // up
    case 0x68: // numpad 8
      size_move_func(0, -1);
      break;
    case 39: // right
    case 0x66: // numpad 6
      size_move_func(1, 0);
      break;
    case 40: // down
    case 0x62: // numpad 2
      size_move_func(0, 1);
      break;
    case 46: // delete
      if (g_selected_element && g_selected_element != e_designer_view) {
        // DeleteElement will return the next element to be selected.
        SelectElement(DeleteElement(g_selected_element));
        g_changed = true;
      }
      break;
  }
}

function selection_onkeyup() {
  if (event.keyCode == 16) // shift
    g_keyboard_sizing = false;
}

function selection_onfocusin() {
  g_keyboard_sizing = false;
}

function selection_onmousedown() {
  if (event.button == 1) {
    var parent_pos = ElementCoordToParent(e_selection, event.x, event.y);
    g_drag_offset_x = parent_pos.x - e_designer_view.offsetX;
    g_drag_offset_y = parent_pos.y - e_designer_view.offsetY;
    var new_selected = FindElementByPos(e_designer_view,
                                        g_drag_offset_x, g_drag_offset_y);
    if (new_selected && new_selected != g_selected_element)
      SelectElement(GetElementInfo(new_selected));
    e_selection.cursor = "sizeall";
    g_dragging = true;
  }
}

function selection_onmousemove() {
  if (g_dragging && !(event.button & 1)) {
    e_selection.cursor = "arrow";
    EndDragging();
  }
  if (g_selected_element && g_dragging) {
    var parent_pos = ElementCoordToParent(e_selection, event.x, event.y);
    DragMove(parent_pos.x - e_designer_view.offsetX,
             parent_pos.y - e_designer_view.offsetY);
  }
}

function selection_onmouseup() {
  selection_onmousemove();
  e_selection.cursor = "arrow";
  EndDragging();
}

function selection_border_onmousedown() {
  if (event.button == 1) {
    var panel_pos = designerUtils.elementCoordToAncestor(event.srcElement,
                                                         e_designer_panel,
                                                         event.x, event.y);
    g_drag_offset_x = panel_pos.x - e_designer_view.offsetX;
    g_drag_offset_y = panel_pos.y - e_designer_view.offsetY;
    g_dragging = true;
  }
}

function selection_border_onmousemove() {
  if (g_dragging && !(event.button & 1))
    EndDragging();
  if (g_selected_element && g_dragging) {
    var panel_pos = designerUtils.elementCoordToAncestor(event.srcElement,
                                                         e_designer_panel,
                                                         event.x, event.y);
    DragMove(panel_pos.x - e_designer_view.offsetX,
             panel_pos.y - e_designer_view.offsetY);
  }
}

function selection_border_onmouseup() {
  selection_border_onmousemove();
  EndDragging();
}

function size_onmousedown(width_dir, height_dir) {
  if (event.button == 1 && g_selected_element) {
    var panel_pos = designerUtils.elementCoordToAncestor(event.srcElement,
                                                         e_designer_panel,
                                                         event.x, event.y);
    g_size_width_dir = width_dir;
    g_size_height_dir = height_dir;
    g_drag_offset_x = panel_pos.x - e_designer_view.offsetX;
    g_drag_offset_y = panel_pos.y - e_designer_view.offsetY;
    g_dragging = true;
  }
}

function size_onmousemove() {
  if (g_dragging && !(event.button & 1))
    EndDragging();
  if (g_selected_element && g_dragging) {
    var panel_pos = designerUtils.elementCoordToAncestor(event.srcElement,
                                                         e_designer_panel,
                                                         event.x, event.y);
    var x = panel_pos.x - e_designer_view.offsetX;
    var y = panel_pos.y - e_designer_view.offsetY;
    var delta = ViewDeltaToElement(g_selected_element,
                                   x - g_drag_offset_x, y - g_drag_offset_y);
    var size_delta = ResizeSelectedElement(delta.x * g_size_width_dir,
                                           delta.y * g_size_height_dir);
    // Dragging the top, left or topleft sizing point changes both size and
    // position. 
    MoveSelectedElementInElementCoord(
        g_size_width_dir == -1 ? -size_delta.x : 0,
        g_size_height_dir == -1 ? -size_delta.y : 0);
    var real_delta = ElementDeltaToView(g_selected_element,
                                        size_delta.x * g_size_width_dir,
                                        size_delta.y * g_size_height_dir);
    g_drag_offset_x += real_delta.x;
    g_drag_offset_y += real_delta.y;
  }
}

function size_onmouseup() {
  size_onmousemove();
  EndDragging();
}

function pin_onmousedown() {
  if (event.button == 1) {
    g_drag_offset_x = event.x + e_pin.offsetX - e_designer_view.offsetX;
    g_drag_offset_y = event.y + e_pin.offsetY - e_designer_view.offsetY;
    g_dragging = true;
  }
}

function pin_onmousemove() {
  if (g_dragging && !(event.button & 1))
    EndDragging();
  if (g_selected_element && g_dragging) {
    var x = event.x + e_pin.offsetX - e_designer_view.offsetX;
    var y = event.y + e_pin.offsetY - e_designer_view.offsetY;
    var delta = ViewDeltaToElement(g_selected_element,
                                   x - g_drag_offset_x, y - g_drag_offset_y);
    var new_pin_x = designerUtils.getOffsetPinX(g_selected_element) + delta.x;
    var new_pin_y = designerUtils.getOffsetPinY(g_selected_element) + delta.y;
    g_selected_element.pinX = new_pin_x;
    g_selected_element.pinY = new_pin_y;
    e_selection.pinX = new_pin_x + kSelectionBorderWidth;
    e_selection.pinY = new_pin_y + kSelectionBorderWidth;
    g_properties_view.SyncPropertyFromElement("pinX");
    g_properties_view.SyncPropertyFromElement("pinY");
    g_changed = true;
    // Enable or disable this?
    // MoveSelectedElementInElementCoord(delta.x, delta.y);
    g_drag_offset_x = x;
    g_drag_offset_y = y;
  }
}

function pin_onmouseup() {
  pin_onmousemove();
  EndDragging();
}

function rotation_onmousedown() {
  if (event.button == 1 && g_selected_element) {
    var pos = ElementCoordToParent(e_selection,
                                   event.x + event.srcElement.offsetX,
                                   event.y + event.srcElement.offsetY);
    g_offset_angle = Math.atan2(e_pin.x - pos.x, pos.y - e_pin.y)
                         * 180 / Math.PI -
                     e_selection.rotation;
    g_dragging = true;
  }
}

function rotation_onmousemove() {
  if (g_dragging && !(event.button & 1))
    EndDragging();
  if (g_selected_element && g_dragging) {
    var pos = ElementCoordToParent(e_selection,
                                   event.x + event.srcElement.offsetX,
                                   event.y + event.srcElement.offsetY);
    var angle = Math.atan2(e_pin.x - pos.x, pos.y - e_pin.y) * 180 / Math.PI;
    angle -= g_offset_angle;
    angle %= 360;
    if (angle < 0) angle += 360;
    g_selected_element.rotation = ViewRotationToElement(g_selected_element,
                                                        angle);
    e_selection.rotation = angle;
    g_properties_view.SyncPropertyFromElement("rotation");
    g_changed = true;
  }
}

function rotation_onmouseup() {
  rotation_onmousemove();
  EndDragging();
}

function MoveSelectedElement(delta_x, delta_y) {
  var x_moved = 0;
  var y_moved = 0;
  if (!g_x_fixed) {
    var org_x = g_selected_element.offsetX;
    var new_x = Math.min(kMaxX, Math.max(kMinX, org_x + delta_x));
    g_selected_element.x = new_x;
    x_moved = new_x - org_x;
    g_properties_view.SyncPropertyFromElement("x");
    g_changed = true;
  }
  if (!g_y_fixed) {
    var org_y = g_selected_element.offsetY;
    var new_y = Math.min(kMaxY, Math.max(kMinY, org_y + delta_y));
    g_selected_element.y = new_y;
    y_moved = new_y - org_y;
    g_properties_view.SyncPropertyFromElement("y");
    g_changed = true;
  }

  var pos = ElementPosToView(g_selected_element);
  e_selection.x = e_pin.x = pos.x + e_designer_view.offsetX;
  e_selection.y = e_pin.y = pos.y + e_designer_view.offsetY;
  return { x: x_moved, y: y_moved };
}

function MoveSelectedElementInElementCoord(delta_x, delta_y) {
  var parent_delta = ElementDeltaToParent(g_selected_element, delta_x, delta_y);
  var moved = MoveSelectedElement(parent_delta.x, parent_delta.y);
  moved = ParentDeltaToElement(g_selected_element, moved.x, moved.y);
  return moved;
}

function ResizeSelectedElement(delta_x, delta_y) {
  var delta_width = 0;
  var delta_height = 0;
  if (!g_width_fixed) {
    var org_width = g_selected_element.offsetWidth;
    var new_width = Math.min(kMaxWidth,
                             Math.max(kMinWidth, org_width + delta_x));
    g_selected_element.width = new_width;
    e_selection.width = new_width + 2 * kSelectionBorderWidth;
    delta_width = new_width - org_width;
    g_properties_view.SyncPropertyFromElement("width");
    g_changed = true;
  }
  if (!g_height_fixed) {
    var org_height = g_selected_element.offsetHeight;
    var new_height = Math.min(kMaxHeight,
                              Math.max(kMinHeight, org_height + delta_y));
    g_selected_element.height = new_height;
    e_selection.height = new_height + 2 * kSelectionBorderWidth;
    delta_height = new_height - org_height;
    g_properties_view.SyncPropertyFromElement("height");
    g_changed = true;
  }
  return { x: delta_width, y: delta_height };
}

function EndDragging() {
  g_dragging = false;
  if (g_properties_view)
    g_properties_view.SyncFromElement();
  e_selection.focus();
  if (g_selected_element == e_designer_view) {
    view.setTimeout(function() {
      designer_onsize();
      view.setTimeout(UpdateSelectionPos, 0);
    }, 0);
  }
}

function DragMove(x, y) {
  if (g_selected_element != e_designer_view) {
    var delta = ViewDeltaToElement(g_selected_element.parentElement,
                                   x - g_drag_offset_x, y - g_drag_offset_y);
    MoveSelectedElement(delta.x, delta.y);
    g_drag_offset_x = x;
    g_drag_offset_y = y;
  }
}

function InitToolBar() {
  for (var i = 0; i < kTools.length; i++) {
    var tool_name = kTools[i];
    e_toolbar.appendElement("<div height='100%'>" +
        "<button width='100%' height='100%' stretchMiddle='true'" +
        " overImage='images/tool_button_over.png'" +
        " downImage='images/tool_button_down.png'" +
        " onclick='SelectTool(" + i + ")' " +
        " ondblclick='tool_button_ondblclick(" + i + ")'/>" +
        "<img x='50%' pinX='50%' src='images/tool_" + tool_name + ".png'/>" +
        "<label size='8' y='25' x='50%' pinX='50%'" +
        " onsize='tool_label_onsize(" + i + ")'>" +
        "&TOOL_" + tool_name.toUpperCase() + ";</label>" +
        "</div>");
  }
}

function tool_button_ondblclick(index) {
  SelectElement(NewElement(kTools[index]));
  g_changed = true;
  SelectTool(-1);
}

function FindElementByPos(element, x, y) {
  if (designerUtils.isPointIn(element, x, y)) {
    if (element.children &&
        (element.tagName != "combobox" || element.droplistVisible)) {
      var descendant = null;
      for (var i = element.children.count - 1; i >= 0; i--) {
        var child = element.children.item(i);
        var child_pos = ParentCoordToElement(child, x, y);
        var descendant = FindElementByPos(child, child_pos.x, child_pos.y);
        if (descendant)
          break;
      }
      return descendant ? descendant : element;
    } else {
      return element;
    }
  }
  return null;
}

function ElementCoordToParent(element, x, y) {
  return designerUtils.elementCoordToAncestor(element, element.parentElement,
                                              x, y);
}

function ParentCoordToElement(element, x, y) {
  return designerUtils.ancestorCoordToElement(element.parentElement, element,
                                              x, y);
}

function ElementDeltaToParent(element, delta_x, delta_y) {
  var pos0 = ElementCoordToParent(element, 0, 0);
  var pos = ElementCoordToParent(element, delta_x, delta_y);
  return { x: pos.x - pos0.x, y: pos.y - pos0.y };
}

function ParentDeltaToElement(element, delta_x, delta_y) {
  var pos0 = ParentCoordToElement(element, 0, 0);
  var pos = ParentCoordToElement(element, delta_x, delta_y);
  return { x: pos.x - pos0.x, y: pos.y - pos0.y };
}

// Here "View" means the e_designer_view. 
// element must be an element under the e_designer_view.
function ElementCoordToView(element, x, y) {
  return designerUtils.elementCoordToAncestor(element, e_designer_view, x, y);
}

function ElementPosToView(element) {
  return ElementCoordToView(element,
                            designerUtils.getOffsetPinX(element),
                            designerUtils.getOffsetPinY(element));
}

function ViewCoordToElement(element, x, y) {
  return designerUtils.ancestorCoordToElement(e_designer_view, element, x, y);
}

function ViewDeltaToElement(element, delta_x, delta_y) {
  var path = [ ];
  while (element != e_designer_view) {
    path.push(element);
    element = element.parentElement;
  }
  var delta = { x: delta_x, y: delta_y };
  for (var i = path.length - 1; i >= 0; i--)
   delta = ParentDeltaToElement(path[i], delta.x, delta.y);
  return delta;
}

function ElementDeltaToView(element, delta_x, delta_y) {
  var delta = { x: delta_x, y: delta_y };
  while (element != e_designer_view) {
    delta = ElementDeltaToParent(element, delta.x, delta.y);
    element = element.parentElement;
  }
  return delta;
}

function ElementRotationToView(element) {
  var rotation = 0;
  while (element != e_designer_view) {
    rotation += element.rotation;
    element = element.parentElement;
  }
  rotation %= 360;
  if (rotation < 0) rotation += 360;
  return rotation;
}

function ViewRotationToElement(element, rotation) {
  element = element.parentElement;
  while (element != e_designer_view) {
    rotation -= element.rotation;
    element = element.parentElement;
  }
  rotation %= 360;
  if (rotation < 0) rotation += 360;
  return rotation;
}

function UpdateSelectionPos() {
  if (g_selected_element) {
    var pos = ElementPosToView(g_selected_element);
    e_selection.x = e_pin.x = e_designer_view.offsetX + pos.x;
    e_selection.y = e_pin.y = e_designer_view.offsetY + pos.y;
    e_selection.width = g_selected_element.offsetWidth +
                        2 * kSelectionBorderWidth;
    e_selection.height = g_selected_element.offsetHeight +
                         2 * kSelectionBorderWidth;
    e_selection.pinX = designerUtils.getOffsetPinX(g_selected_element) +
                       kSelectionBorderWidth;
    e_selection.pinY = designerUtils.getOffsetPinY(g_selected_element) +
                       kSelectionBorderWidth;
    e_selection.rotation = ElementRotationToView(g_selected_element);

    if (g_selected_element == e_designer_view) {
      g_x_fixed = g_y_fixed = true;
      g_width_fixed = g_height_fixed = false;
      e_rotation1.visible = false;
      e_rotation2.visible = false;
    } else {
      // Don't allow mouse resizing if x/y/width/height specified percentage.
      var is_item = g_selected_element.tagName == "item";
      g_x_fixed = typeof g_selected_element.x == "string" || is_item;
      g_y_fixed = typeof g_selected_element.y == "string" || is_item;
      g_width_fixed = typeof g_selected_element.width == "string" || is_item;
      g_height_fixed = typeof g_selected_element.height == "string" ||
                       is_item || g_selected_element.tagName == "combobox";
      e_rotation1.visible = true;
      e_rotation2.visible = true;
    }
    e_size_top_left.visible = !g_x_fixed && !g_y_fixed &&
                              !g_width_fixed & !g_height_fixed;
    e_size_top.visible = !g_y_fixed && !g_height_fixed;
    e_size_top_right.visible = !g_y_fixed && !g_height_fixed && !g_width_fixed;
    e_size_left.visible = !g_x_fixed && !g_width_fixed;
    e_size_right.visible = !g_width_fixed;
    e_size_bottom_left.visible = !g_x_fixed && !g_width_fixed &&
                                 !g_height_fixed;
    e_size_bottom.visible = !g_height_fixed;
    e_size_bottom_right.visible = !g_width_fixed && !g_height_fixed;
    e_selection.visible = true;
    e_selection.focus();
  }
}

function SelectElement(element_info) {
  if (element_info) {
    var element = element_info.element;
    g_selected_element = element;
    // Will be set to true in UpdateSelectionPos.
    e_selection.visible = false;
    if (g_selected_element != e_designer_view)
      e_pin.visible = true;
    g_properties_view = new Properties(
        e_properties_list, g_properties_meta[element_info.type], element_info,
        function() { // The on_property_change callback.
          // Setting one property may cause changes of other properties, so
          // sync all properties.
          g_properties_view.SyncFromElement();
          g_changed = true;
          UpdateSelectionPos();
        });
    g_events_view = new Properties(
        e_events_list, g_events_meta[element_info.type], element_info,
        function() { // The on_property_change callback.
          // TODO:
          g_changed = true;
        });
    ElementsViewSelectNode(element_info);
    view.setTimeout(UpdateSelectionPos, 0);
  } else {
    e_selection.visible = false;
    e_pin.visible = false;
    g_selected_element = null;
    g_properties_view = null;
    g_events_view = null;
    e_properties_list.removeAllElements();
    e_events_list.removeAllElements();
    ElementsViewSelectNode(null);
  }
}

function SelectTool(index) {
  if (g_current_tool == index) {
    // Selecting a currently selected tool will cause the tool unselected.  
    index = -1;
  }
  if (g_current_tool != -1) {
    var button = e_toolbar.children.item(g_current_tool).children.item(0);
    button.image = "";
    button.overImage = "images/tool_button_over.png";
  }
  g_current_tool = index;
  if (index != -1) {
    var button = e_toolbar.children.item(index).children.item(0);
    button.image = "images/tool_button_active.png";
    button.overImage = "images/tool_button_active_over.png";
    SelectElement(null);
  }
}

function AddGlobalMenu(menu) {
  menu.AddItem(strings.MENU_NEW_GADGET, 0, NewGadget);
  menu.AddItem(strings.MENU_OPEN_GADGET, 0, OpenGadget);
  menu.AddItem(strings.MENU_CLOSE_GADGET,
               g_gadget_file ? 0 : gddMenuItemFlagGrayed, CloseGadget);
  menu.AddItem(strings.MENU_RUN_GADGET,
               g_gadget_file ? 0 : gddMenuItemFlagGrayed, RunGadget);
//  menu.AddItem("", 0, null);
  menu.AddItem(strings.MENU_SAVE_CURRENT_FILE,
               g_current_file ? 0 : gddMenuItemFlagGrayed, SaveCurrentFile);
  menu.AddItem(strings.MENU_EDIT_SOURCE,
               g_current_file ? 0 : gddMenuItemFlagGrayed, EditSource);
}

function RefreshFileList() {
  e_file_list.removeAllElements();
  var files = g_gadget_file_manager.getAllFiles();
  for (var i in files) {
    e_file_list.appendString(files[i]);
    if (files[i] == g_current_file)
      e_file_list.selectedIndex = i;
  }
  for (var i = 0; i < e_file_list.children.count; i++)
    e_file_list.children.item(i).children.item(0).size = 9;
}

function NewGadget() {
  // TODO:
}

function OpenGadget() {
  if (!CloseGadget())
    return;
  var file = framework.BrowseForFile(
      Strings.TYPE_ALL_GADGET + "|*.gg;gadget.gmanifest|" +
      Strings.TYPE_GG + "|*.gg|" +
      Strings.TYPE_GMANIFEST + "|gadget.gmanifest");
  if (file) {
    g_gadget_file = file;
    g_gadget_file_manager = designerUtils.initGadgetFileManager(file);
    if (g_gadget_file_manager) {
      e_toolbar.visible = true;
      e_tool_views.visible = true;
      e_designer.visible = true;
      OpenFile("main.xml");
      RefreshFileList();
    } else {
      gadget.debug.error("Failed to open gadget: " + file);
    }
  }
}

function CheckSave() {
  return !g_changed || view.confirm(strings.CONFIRM_DISCARD);
}

function CloseGadget() {
  if (!CheckSave())
    return false;

  designerUtils.removeGadget();
  e_toolbar.visible = false;
  e_tool_views.visible = false;
  e_designer.visible = false;
  ResetGlobals();
  g_gadget_file = null;
  g_gadget_file_manager = null;
  e_file_list.removeAllElements();
  g_extracted_files = { };
  g_extracted_files_time = { };
  return true;
}

function RunGadget() {
  designerUtils.runGadget(g_gadget_file);
}

function GetFileExtension(filename) {
  var extpos = filename.lastIndexOf(".");
  return extpos > 0 ? filename.substring(extpos + 1) : "";
}

function SystemOpenFile(filename) {
  var full_path;
  if (g_gadget_file_manager.isDirectlyAccessible(filename)) {
    full_path = g_gadget_file_manager.getFullPath(filename);
  } else {
    full_path = g_gadget_file_manager.extract(filename);
    if (full_path) {
      g_extracted_files[filename] = full_path;
      g_extracted_files_time[filename] =
          g_global_file_manager.getLastModifiedTime(full_path).getTime();
    }
  }

  if (full_path) {
    var extension = GetFileExtension(filename);
    var editor = g_system_editors[extension];
    if (editor)
      designerUtils.systemOpenFileWith(editor, full_path);
    else
      framework.openUrl(full_path);
  } else {
    gadget.debug.error("Failed to get full path or extract file: " + filename);
  }
}

function EditSource() {
  if (CheckSave())
    SystemOpenFile(g_current_file);
}

function SelectFile(filename) {
  var items = e_file_list.children;
  for (var i = 0; i < items.count; i++) {
    if (items.item(i).children(0).innerText == filename) {
      e_file_list.selectedIndex = i;
      break;
    }
  }
}

// Replace entities (except the predefined ones) to a XML string representing
// itself, e.g. replace "&MENU_FILE;" to "&amp;MENU_FILE;".
// This is important to let the original entity strings entered and
// displayed in the designer in place of the actual localized strings. 
// Works well except some corner cases, but it's enough for a designer.
function ReplaceEntitiesToStrings(s) {
  // We don't allow ':' and non-ASCII alpha chars in string names.
  return s.replace(
      /&(?!(amp|lt|gt|apos|quot);)(?=[a-zA-Z_][a-zA-Z0-9_\-.]*;)/gm, "&amp;");
}

function ReplaceStringsToEntities(s) {
  return s.replace(/&amp;(?=[a-zA-Z_][a-zA-Z0-9_\-.]*;)/gm, "&");
}

function SaveCurrentFile() {
  var content = ReplaceStringsToEntities(ViewToXML());
  if (g_gadget_file_manager.write(g_current_file, content, true)) {
    if (!g_gadget_file_manager.isDirectlyAccessible(g_current_file)) {
      var extracted = g_extracted_files[g_current_file];
      if (extracted) {
        // Re-extract the file to let the system editor know the change. 
        var full_path = g_gadget_file_manager.extract(g_current_file);
        if (full_path != extracted)
          g_global_file_manager.write(extracted, content, true);
        g_extracted_files_time[g_current_file] =
            g_global_file_manager.getLastModifiedTime(extracted).getTime();
      }
    }
    g_current_file_time =
        g_gadget_file_manager.getLastModifiedTime(g_current_file).getTime();
    g_changed = false;
  }
}

function OpenFile(filename) {
  var extension = GetFileExtension(filename);
  if (extension == "xml") {
    var content = g_gadget_file_manager.read(filename);
    if (content) {
      content = ReplaceEntitiesToStrings(content);
      var doc = new DOMDocument();
      doc.loadXML(content);
      if (GadgetStrEquals(doc.documentElement.tagName, "view")) {
        if (!CheckSave())
          return;
        ResetGlobals();
        ParseViewXMLDocument(doc);
        g_current_file = filename;
        g_current_file_time =
            g_gadget_file_manager.getLastModifiedTime(filename).getTime();
        SelectFile(filename);
        return;
      } else {
        gadget.debug.warning("XML file is not a view file: " + filename);
      }
    } else {
      gadget.debug.warning("Failed to load file or file is empty: " + filename);
    }
  }
  SystemOpenFile(filename);
  // Still select the current opened file.
  SelectFile(g_current_file);
}

function CheckExternalFileChange() {
  // Copy changed extracted files back into the zip.
  for (var i in g_extracted_files) {
    var extracted = g_extracted_files[i];
    if (g_global_file_manager.getLastModifiedTime(extracted).getTime() >
        g_extracted_files_time[i]) {
      var content = g_global_file_manager.read(extracted);
      g_gadget_file_manager.write(i, content, true);
      gadget.debug.warning("Updating file: " + i);
    }
  }

  if (g_current_file) {
    var time =
        g_gadget_file_manager.getLastModifiedTime(g_current_file).getTime();
    if (time > g_current_file_time) {
      if (view.confirm(strings.CONFIRM_CHANGED)) {
        g_changed = false;
        OpenFile(g_current_file);
      }
    }
  }
}

function AddElementContextMenu() {
  if (!g_selected_element || g_selected_element == e_designer_view)
    return true;

  var menu = event.menu;
  var can_up_level = false;
  var can_down_level = false;
  var can_move_back = false;
  var can_move_front = false;
  var parent = g_selected_element.parentElement;
  var index = GetIndexInParent(g_selected_element);
  can_up_level = parent != e_designer_view;
  can_move_back = index > 0;
  can_move_front = index < parent.children.count - 1;
  // The element can be moved down a level if the next element can have
  // children.
  can_down_level = can_move_front && parent.children.item(index + 1).children;

  menu.AddItem(strings.MENU_RENAME, 0, RenameElement);
  menu.AddItem(strings.MENU_MOVE_BACK,
               can_move_back ? 0 : gddMenuItemFlagGrayed,
               MoveElementBack);
  menu.AddItem(strings.MENU_MOVE_FRONT,
               can_move_front ? 0 : gddMenuItemFlagGrayed,
               MoveElementFront);
  menu.AddItem(strings.MENU_MOVE_BOTTOM,
               can_move_back ? 0 : gddMenuItemFlagGrayed,
               MoveElementToBottom);
  menu.AddItem(strings.MENU_MOVE_TOP,
               can_move_front ? 0 : gddMenuItemFlagGrayed,
               MoveElementToTop);
  menu.AddItem(strings.MENU_MOVE_UP_LEVEL,
               can_up_level ? 0 : gddMenuItemFlagGrayed,
               MoveElementUpLevel);
  menu.AddItem(strings.MENU_MOVE_DOWN_LEVEL,
               can_down_level ? 0 : gddMenuItemFlagGrayed,
               MoveElementDownLevel);
  if (g_selected_element.tagName == "listbox" ||
      g_selected_element.tagName == "combobox") {
    menu.AddItem("", 0, null);
    menu.AddItem(strings.MENU_APPEND_STRING, 0, PromptAndAppendString);
  }
  event.returnValue = false;
}

function RenameElement() {
  g_properties_view.SelectProperty("name");
}

function MoveElementBack() {
  g_changed = true;
  SelectElement(MoveElement(g_selected_element,
                            g_selected_element.parentElement,
                            GetPreviousSibling(g_selected_element)));
}

function MoveElementFront() {
  g_changed = true;
  var parent = g_selected_element.parentElement;
  var index = GetIndexInParent(g_selected_element);
  SelectElement(MoveElement(g_selected_element, parent,
                            parent.children.item(index + 2)));
}

function MoveElementToBottom() {
  g_changed = true;
  var parent = g_selected_element.parentElement;
  SelectElement(MoveElement(g_selected_element, parent,
                            parent.children.item(0)));
}

function MoveElementToTop() {
  g_changed = true;
  SelectElement(MoveElement(g_selected_element,
                            g_selected_element.parentElement, null));
}

function MoveElementUpLevel() {
  g_changed = true;
  var parent = g_selected_element.parentElement;
  if (parent.tagName == "item")
    parent = parent.parentElement;
  SelectElement(MoveElement(g_selected_element, parent.parentElement, parent));
}

function MoveElementDownLevel() {
  var next = GetNextSibling(g_selected_element);
  if (next && next.children) {
    g_changed = true;
    SelectElement(MoveElement(g_selected_element, next,
                              next.children.item(0)));
  }
}

function PromptAndAppendString() {
  var s = view.prompt(strings.PROMPT_APPEND_STRING, "");
  if (s) {
    g_changed = true;
    SelectElement(AppendString(g_selected_element, s));
  }
}
