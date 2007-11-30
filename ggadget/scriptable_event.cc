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
#include "basic_element.h"
#include "event.h"
#include "scriptable_array.h"

namespace ggadget {

class ScriptableEvent::Impl {
 public:
  Impl::Impl(const Event *event, BasicElement *src_element,
             Event *output_event)
       : event_(event),
         return_value_(EVENT_RESULT_UNHANDLED),
         src_element_(src_element),
         output_event_(output_event) {
  }

  ScriptableArray *ScriptGetDragFiles() {
    ASSERT(event_->IsDragEvent());
    const DragEvent *drag_event = static_cast<const DragEvent *>(event_);
    return ScriptableArray::Create(drag_event->GetDragFiles(), false);
  }

  bool ScriptGetReturnValue() {
    return return_value_ != EVENT_RESULT_CANCELED;
  }

  void ScriptSetReturnValue(bool value) {
    return_value_ = value ? EVENT_RESULT_HANDLED : EVENT_RESULT_CANCELED;
  }

  const Event *event_;
  EventResult return_value_;
  BasicElement *src_element_;
  Event *output_event_;
};

ScriptableEvent::ScriptableEvent(const Event *event,
                                 BasicElement *src_element,
                                 Event *output_event)
    : impl_(new Impl(event, src_element, output_event)) {
  RegisterProperty("returnValue",
                   NewSlot(impl_, &Impl::ScriptGetReturnValue),
                   NewSlot(impl_, &Impl::ScriptSetReturnValue));
  RegisterProperty("srcElement", NewSlot(this, &ScriptableEvent::GetSrcElement),
                   NULL);
  RegisterProperty("type", NewSlot(this, &ScriptableEvent::GetName), NULL);

  if (event->IsMouseEvent()) {
    const MouseEvent *mouse_event = static_cast<const MouseEvent *>(event);
    const PositionEvent *position_event =
        static_cast<const PositionEvent *>(event);
    RegisterProperty("x", NewSlot(position_event, &PositionEvent::GetX), NULL);
    RegisterProperty("y", NewSlot(position_event, &PositionEvent::GetX), NULL);
    RegisterProperty("button",
                     NewSlot(mouse_event, &MouseEvent::GetButton), NULL);
    RegisterProperty("wheelDelta",
                     NewSlot(mouse_event, &MouseEvent::GetWheelDelta), NULL);
  } else if (event->IsKeyboardEvent()) {
    const KeyboardEvent *key_event = static_cast<const KeyboardEvent *>(event);
    RegisterProperty("keyCode",
                     NewSlot(key_event, &KeyboardEvent::GetKeyCode), NULL);
  } else if (event->IsDragEvent()) {
    const PositionEvent *position_event =
        static_cast<const PositionEvent *>(event);
    RegisterProperty("x", NewSlot(position_event, &PositionEvent::GetX), NULL);
    RegisterProperty("y", NewSlot(position_event, &PositionEvent::GetX), NULL);
    RegisterProperty("dragFiles", NewSlot(impl_, &Impl::ScriptGetDragFiles),
                     NULL);
  } else {
    Event::Type type = event->GetType();
    switch (type) {
      case Event::EVENT_SIZING: {
        ASSERT(output_event && output_event->GetType() == Event::EVENT_SIZING);
        const SizingEvent *sizing_event =
            static_cast<const SizingEvent *>(event);
        SizingEvent *output_sizing_event =
            static_cast<SizingEvent *>(output_event);
        RegisterProperty("width",
                         NewSlot(sizing_event, &SizingEvent::GetWidth),
                         NewSlot(output_sizing_event, &SizingEvent::SetWidth));
        RegisterProperty("height",
                         NewSlot(sizing_event, &SizingEvent::GetHeight),
                         NewSlot(output_sizing_event, &SizingEvent::SetHeight));
        break;
      }
      case Event::EVENT_OPTION_CHANGED: {
        const OptionChangedEvent *option_changed_event =
            static_cast<const OptionChangedEvent *>(event);
        RegisterProperty("propertyName",
                         NewSlot(option_changed_event,
                                 &OptionChangedEvent::GetPropertyName),
                         NULL);
        break;
      }
      case Event::EVENT_TIMER: {
        const TimerEvent *timer_event = static_cast<const TimerEvent *>(event);
        RegisterProperty("cookie", NewSlot(timer_event, &TimerEvent::GetToken),
                         NULL);
        RegisterProperty("value", NewSlot(timer_event, &TimerEvent::GetValue),
                         NULL);
        break;
      }
      default:
        break;
    }
  }
}

ScriptableEvent::~ScriptableEvent() {
  delete impl_;
  impl_ = NULL;
}

const char *ScriptableEvent::GetName() const {
  switch (impl_->event_->GetType()) {
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

    case Event::EVENT_SIZING: return kOnSizingEvent;
    case Event::EVENT_OPTION_CHANGED: return kOnOptionChangedEvent;
    case Event::EVENT_TIMER: return "";  // Windows version does the same.
    default: ASSERT(false); return "";
  }
}

const Event *ScriptableEvent::GetEvent() const {
  return impl_->event_;
}

const Event *ScriptableEvent::GetOutputEvent() const {
  return impl_->output_event_;
}
Event *ScriptableEvent::GetOutputEvent() {
  return impl_->output_event_;
}

BasicElement *ScriptableEvent::GetSrcElement() {
  return impl_->src_element_;
}
const BasicElement *ScriptableEvent::GetSrcElement() const {
  return impl_->src_element_;
}

EventResult ScriptableEvent::GetReturnValue() const {
  return impl_->return_value_;
}

void ScriptableEvent::SetReturnValue(EventResult return_value) {
  impl_->return_value_ = return_value;
}

} // namespace ggadget
