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
#include <ggadget/event.h>
#include <ggadget/scriptable_helper.h>
#include <ggadget/scriptable_interface.h>

namespace ggadget {

class BasicElement;

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
const char *const kOnChangeEvent        = "onchange";
const char *const kOnTextChangeEvent    = "ontextchange";

/**
 * Scriptable decorator for @c Event.
 */
class ScriptableEvent : public ScriptableHelper<ScriptableInterface> {
 public:
  DEFINE_CLASS_ID(0x6732238aacb4468a, ScriptableInterface)

  /**
   * @param event it's not declared as a const reference because sometimes we
   *     need dynamically allocated event (e.g. @c View::PostEvent()).
   * @param src_element the element from which is event is fired.
   *     If the event is fired from the view, @a src_element should be @c NULL.
   * @param output_event only used for some events (such as
   *     @c Event::EVENT_SIZING) to store the output event.
   *     Can be @c NULL if the event has no output.
   */
  ScriptableEvent(const Event *event,
                  BasicElement *src_element,
                  Event *output_event);
  virtual ~ScriptableEvent();

  const char *GetName() const;
  const Event *GetEvent() const;

  const Event *GetOutputEvent() const;
  Event *GetOutputEvent();

  BasicElement *GetSrcElement();
  const BasicElement *GetSrcElement() const;
  void SetSrcElement(BasicElement *src_element);

  EventResult GetReturnValue() const;
  void SetReturnValue(EventResult return_value);

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableEvent);
};

} // namespace ggadget

#endif // GGADGET_SCRIPTABLE_EVENT_H__
