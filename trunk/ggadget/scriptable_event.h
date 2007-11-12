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

#ifndef GGADGET_SCRIPTABLE_EVENT_H__
#define GGADGET_SCRIPTABLE_EVENT_H__

#include <ggadget/common.h>
#include <ggadget/scriptable_helper.h>
#include <ggadget/scriptable_interface.h>

namespace ggadget {

class Event;
class ElementInterface;

/** Event names */
const char *const kOnCancelEvent        = "oncancel";
const char *const kOnClickEvent         = "onclick";
const char *const kOnCloseEvent         = "onclose";
const char *const kOnDblClickEvent      = "ondblclick";
const char *const kOnRClickEvent        = "onrclick";
const char *const kOnRDblClickEvent     = "onrdblclick";
const char *const kOnDragDropEvent      = "ondragdrop";
const char *const kOnDragOutEvent       = "ondragout";
const char *const kOnDragOverEvent      = "ondragover";
const char *const kOnFocusInEvent       = "onfocusin";
const char *const kOnFocusOutEvent      = "onfocusout";
const char *const kOnDockEvent          = "ondock";
const char *const kOnKeyDownEvent       = "onkeydown";
const char *const kOnKeyPressEvent      = "onkeypress";
const char *const kOnKeyUpEvent         = "onkeyup";
const char *const kOnMinimizeEvent      = "onminimize";
const char *const kOnMouseDownEvent     = "onmousedown";
const char *const kOnMouseMoveEvent     = "onmousemove";
const char *const kOnMouseOutEvent      = "onmouseout";
const char *const kOnMouseOverEvent     = "onmouseover";
const char *const kOnMouseUpEvent       = "onmouseup";
const char *const kOnMouseWheelEvent    = "onmousewheel";
const char *const kOnOkEvent            = "onok";
const char *const kOnOpenEvent          = "onopen";
const char *const kOnOptionChangedEvent = "onoptionchanged";
const char *const kOnPopInEvent         = "onpopin";
const char *const kOnPopOutEvent        = "onpopout";
const char *const kOnRestoreEvent       = "onrestore";
const char *const kOnSizeEvent          = "onsize";
const char *const kOnSizingEvent        = "onsizing";
const char *const kOnUndockEvent        = "onundock";

/**
 * Scriptable decorator for @c Event.
 */
class ScriptableEvent : public ScriptableHelper<ScriptableInterface> {
 public:
  DEFINE_CLASS_ID(0x6732238aacb4468a, ScriptableInterface)

  ScriptableEvent(Event *event, ElementInterface *src_element,
                  int cookie, int value);
  virtual ~ScriptableEvent();

  const char *GetName() const;

  Event *GetEvent() { return event_; }
  const Event *GetEvent() const { return event_; }

  ElementInterface *GetSrcElement() { return src_element_; }
  const ElementInterface *GetSrcElement() const { return src_element_; }

  bool GetReturnValue() const { return return_value_; }
  int GetCookie() const { return cookie_; }
  int GetValue() const { return value_; }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableEvent);

  Event *event_;
  bool return_value_;
  ElementInterface *src_element_;
  int cookie_;
  int value_;
};

} // namespace ggadget

#endif // GGADGET_SCRIPTABLE_EVENT_H__
