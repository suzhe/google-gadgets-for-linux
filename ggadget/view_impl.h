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

#ifndef GGADGET_VIEW_IMPL_H__
#define GGADGET_VIEW_IMPL_H__

#include "signal.h"
#include "static_scriptable.h"

namespace ggadget {

namespace internal {

/**
 * Fires when the user chooses the Cancel button in an options view.
 */
static const char *const kOnCancelEvent = "oncancel";
/**
 * Fires when the left mouse button is clicked.
 */
static const char *const kOnClickEvent = "onclick";
/**
 * Fires when the view is about to be closed.
 */
const char *const kOnCloseEvent = "onclose";
/**
 * Fires when the left mouse button is double-clicked.
 */
const char *const kOnDblClickEvent = "ondblclick";
/**
 * Fires when the gadget is moved into the Sidebar.
 */
const char *const kOnDockEvent = "ondock";
/**
 * Fires when a key is pressed down.
 */
const char *const kOnKeyDownEvent = "onkeydown";
/**
 * Fires when a key is pressed and released.
 */
const char *const kOnKeyPressEvent = "onkeypress";
/**
 * Fires when a key is released.
 */
const char *const kOnKeyReleaseEvent = "onkeyrelease";
/**
 * Fires when the gadget is minimized.
 */
const char *const kOnMinimizeEvent = "onminimize";
/**
 * Fires when the left mouse button is pressed down.
 */
const char *const kOnMouseDownEvent = "onmousedown";
/**
 * Fires when the mouse cursor leaves the view.
 */
const char *const kOnMouseOutEvent = "onmouseout";
/**
 * Fires when the mouse cursor enters the view.
 */
const char *const kOnMouseOverEvent = "onmouseover";
/**
 * Fires when the left mouse button is released.
 */
const char *const kOnMouseUpEvent = "onmouseup";
/**
 * Fires when the user chooses the OK button in an options view.
 */
const char *const kOnOkEvent = "onok";
/**
 * Fires when the view is first opened.
 */
const char *const kOnOpenEvent = "onopen";
/**
 * Fires when a property in the @c options object is added, changed,
 * or removed.  @c event.propertyName specifies which item was changed.
 */
const char *const kOnOptionChangedEvent = "onoptionchanged";
/**
 * Fires when the gadget's expanded view closes.
 */
const char *const kOnPopInEvent = "onpopin";
/**
 * Fires when the gadget's expanded view opens.
 */
const char *const kOnPopOutEvent = "onpopout";
/**
 * Fires when the gadget is restored from the minimized state.
 */
const char *const kOnRestoreEvent = "onrestore";
/**
 * Fires after the view has changed to a new size either as a result of
 * script code modifying the size (e.g. setting @c view.width or
 * @c view.height, or calling @c view.resizeBy or @c view.resizeTo) or
 * after @c onresizing was called and a new size was specified.  This event
 * cannot be canceled.
 */
const char *const kOnSizeEvent = "onsize";
/**
 * Fires when the user is resizing the gadget.  Only fires if @c resizable
 * is set to @c true.  @c event.width and @c event.height contain the new
 * width and height requested by the user.  The event code can cancel the
 * event (<code>event.returnValue = false</code>) which causes the gadget to
 * keep its current size.  The event code can modify @c event.width and
 * @c event.height to change to override the user's selection.
 */
const char *const kOnSizingEvent = "onsizing";
/**
 * Fires when the gadget is moved out of the Sidebar.
 */
const char *const kOnUndockEvent = "onundock";

class ViewImpl {
 public:
  ViewImpl();
  ~ViewImpl();

  DELEGATE_SCRIPTABLE_REGISTER(static_scriptable_);

  EventSignal oncancle_event_;
  EventSignal onclick_event_;
  EventSignal onclose_event_;
  EventSignal ondblclick_event_;
  EventSignal ondock_event_;
  EventSignal onkeydown_event_;
  EventSignal onkeypress_event_;
  EventSignal onkeyrelease_event_;
  EventSignal onminimize_event_;
  EventSignal onmousedown_event_;
  EventSignal onmouseout_event_;
  EventSignal onmouseover_event_;
  EventSignal onmouseup_event_;
  EventSignal onok_event_;
  EventSignal onopen_event_;
  EventSignal onoptionchanged_event_;
  EventSignal onpopin_event_;
  EventSignal onpopout_event_;
  EventSignal onrestore_event_;
  EventSignal onsize_event_;
  EventSignal onsizing_event_;
  EventSignal onundock_event_;

 private:
  StaticScriptable static_scriptable_;

};

} // namespace internal

} // namespace ggadget

#endif // GGADGET_VIEW_IMPL_H__
