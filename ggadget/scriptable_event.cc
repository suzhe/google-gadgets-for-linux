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

#include "scriptable_event.h"
#include "element_interface.h"
#include "event.h"
#include "scriptable_array.h"

namespace ggadget {

class DragFilesHelper {
 public:
  DragFilesHelper(const char **drag_files) : drag_files_(drag_files) { }
  ScriptableArray *operator()() const {
    return ScriptableArray::Create(drag_files_, false);
  }
  bool operator ==(const DragFilesHelper &another) const {
    return another.drag_files_ == drag_files_;
  }
 private:
  const char **drag_files_;
};

ScriptableEvent::ScriptableEvent(Event *event,
                                 ElementInterface *src_element,
                                 int cookie,
                                 int value)
    : event_(event),
      return_value_(true),
      src_element_(src_element),
      cookie_(cookie),
      value_(value) {
  RegisterSimpleProperty("returnValue", &return_value_); 
  RegisterReadonlySimpleProperty("srcElement", &src_element_);
  RegisterReadonlySimpleProperty("cookie", &cookie_);
  RegisterReadonlySimpleProperty("value", &value_);
  RegisterProperty("type", NewSlot(this, &ScriptableEvent::GetName), NULL);

  if (event->IsMouseEvent()) {
    MouseEvent *mouse_event = static_cast<MouseEvent *>(event);
    PositionEvent *position_event = static_cast<PositionEvent *>(event);
    RegisterProperty("x", NewSlot(position_event, &PositionEvent::GetX), NULL);
    RegisterProperty("y", NewSlot(position_event, &PositionEvent::GetX), NULL);
    RegisterProperty("button",
                     NewSlot(mouse_event, &MouseEvent::GetButton), NULL);
    RegisterProperty("wheelDelta",
                     NewSlot(mouse_event, &MouseEvent::GetWheelDelta), NULL);
  } else if (event->IsKeyboardEvent()) {
    KeyboardEvent *key_event = static_cast<KeyboardEvent *>(event);
    RegisterProperty("keyCode",
                     NewSlot(key_event, &KeyboardEvent::GetKeyCode), NULL);
  } else if (event->IsDragEvent()) {
    DragEvent *drag_event = static_cast<DragEvent *>(event);
    PositionEvent *position_event = static_cast<PositionEvent *>(event);
    RegisterProperty("x", NewSlot(position_event, &PositionEvent::GetX), NULL);
    RegisterProperty("y", NewSlot(position_event, &PositionEvent::GetX), NULL);
    RegisterProperty("dragFiles",
                     NewFunctorSlot<ScriptableArray *>(
                         DragFilesHelper(drag_event->GetDragFiles())),
                     NULL);
  } else {
    Event::Type type = event->GetType();
    switch (type) {
      case Event::EVENT_SIZING: {
        SizingEvent *sizing_event = static_cast<SizingEvent *>(event);
        RegisterProperty("width",
                         NewSlot(sizing_event, &SizingEvent::GetWidth),
                         NewSlot(sizing_event, &SizingEvent::SetWidth));
        RegisterProperty("height",
                         NewSlot(sizing_event, &SizingEvent::GetHeight),
                         NewSlot(sizing_event, &SizingEvent::SetHeight));
        break;
      }
      case Event::EVENT_OPTION_CHANGED: {
        OptionChangedEvent *option_changed_event =
            static_cast<OptionChangedEvent *>(event);
        RegisterProperty("propertyName",
                         NewSlot(option_changed_event,
                                 &OptionChangedEvent::GetPropertyName),
                         NULL);
        break;
      }
      default:
        break;
    }
  }
}

ScriptableEvent::~ScriptableEvent() {
}

const char *ScriptableEvent::GetName() const {
  switch (event_->GetType()) {
    case Event::EVENT_CANCEL: return kOnCancelEvent;
    case Event::EVENT_CLOSE: return kOnCloseEvent;
    case Event::EVENT_DOCK: return kOnDockEvent;
    case Event::EVENT_MINIMIZE: return kOnMinimizeEvent;
    case Event::EVENT_OK: return kOnOkEvent;
    case Event::EVENT_OPEN: return kOnOpenEvent;
    case Event::EVENT_POPIN: return kOnPopInEvent;
    case Event::EVENT_POPOUT: return kOnPopOutEvent;
    case Event::EVENT_RESTORE: return kOnRestoreEvent;
    case Event::EVENT_SIZE: return kOnSizeEvent;
    case Event::EVENT_UNDOCK: return kOnUndockEvent;
    case Event::EVENT_FOCUS_IN: return kOnFocusInEvent;
    case Event::EVENT_FOCUS_OUT: return kOnFocusOutEvent;

    case Event::EVENT_MOUSE_DOWN: return kOnMouseDownEvent;
    case Event::EVENT_MOUSE_UP: return kOnMouseUpEvent;
    case Event::EVENT_MOUSE_CLICK: return kOnClickEvent;
    case Event::EVENT_MOUSE_DBLCLICK: return kOnDblClickEvent;
    case Event::EVENT_MOUSE_RCLICK: return kOnRClickEvent;
    case Event::EVENT_MOUSE_RDBLCLICK: return kOnRDblClickEvent;
    case Event::EVENT_MOUSE_MOVE: return kOnMouseMoveEvent;
    case Event::EVENT_MOUSE_OUT: return kOnMouseOutEvent;
    case Event::EVENT_MOUSE_OVER: return kOnMouseOverEvent;
    case Event::EVENT_MOUSE_WHEEL: return kOnMouseWheelEvent;

    case Event::EVENT_KEY_DOWN: return kOnKeyDownEvent;
    case Event::EVENT_KEY_UP: return kOnKeyUpEvent;
    case Event::EVENT_KEY_PRESS: return kOnKeyPressEvent;

    case Event::EVENT_DRAG_DROP: return kOnDragDropEvent;
    case Event::EVENT_DRAG_OUT: return kOnDragOutEvent;
    case Event::EVENT_DRAG_OVER: return kOnDragOverEvent;

    case Event::EVENT_TIMER_TICK: return "";  // Windows version does the same.
    case Event::EVENT_SIZING: return kOnSizingEvent;
    case Event::EVENT_OPTION_CHANGED: return kOnOptionChangedEvent;
    default: ASSERT(false); return "";
  }
}

} // namespace ggadget
