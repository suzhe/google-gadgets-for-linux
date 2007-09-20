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

#include <map>
#include <vector>

#include "common.h"
#include "elements.h"
#include "signal.h"

namespace ggadget {

namespace internal {

// TODO: the following events should be supported by view.
#if 0
/**
 * Fires when the user chooses the Cancel button in an options view.
 */
const char *const kOnCancelEvent = "oncancel";
/**
 * Fires when the view is about to be closed.
 */
const char *const kOnCloseEvent = "onclose";
/**
 * Fires when the gadget is moved into the Sidebar.
 */
const char *const kOnDockEvent = "ondock";
/**
 * Fires when the gadget is minimized.
 */
const char *const kOnMinimizeEvent = "onminimize";
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
#endif

class ViewImpl {
 public:
  ViewImpl(ElementFactoryInterface *factory, ViewInterface *owner);
  ~ViewImpl();

  bool AttachHost(HostInterface *host);

  void OnMouseEvent(MouseEvent *event);
  void OnKeyEvent(KeyboardEvent *event);
  void OnTimerEvent(TimerEvent *event);
  void OnOtherEvent(Event *event);

  void OnElementAdd(ElementInterface *element);
  void OnElementRemove(ElementInterface *element);

  void FireEvent(Event *event, const EventSignal &event_signal);
  Event *GetEvent() const;

  bool SetWidth(int width);
  bool SetHeight(int height);
  bool SetSize(int width, int height);
  bool ResizeBy(int width, int height);

  const CanvasInterface *Draw(bool *changed);

  void SetResizable(ViewInterface::ResizableMode resizable);
  void SetCaption(const char *caption);
  void SetShowCaptionAlways(bool show_always);

  ElementInterface *GetElementByName(const char *name);

  enum TimerType { TIMER_ANIMATION, TIMER_TIMEOUT, TIMER_INTERVAL };
  int NewTimer(TimerType type, Slot *slot,
               int start_value, int end_value, unsigned int duration);
  void RemoveTimer(int token);
  int BeginAnimation(Slot *slot, int start_value, int end_value,
                     unsigned int duration);
  void CancelAnimation(int token);
  int SetTimeout(Slot *slot, unsigned int duration);
  void ClearTimeout(int token);
  int SetInterval(Slot *slot, unsigned int duration);
  void ClearInterval(int token);

  EventSignal oncancle_event_;
  EventSignal onclick_event_;
  EventSignal onclose_event_;
  EventSignal ondblclick_event_;
  EventSignal ondock_event_;
  EventSignal onkeydown_event_;
  EventSignal onkeypress_event_;
  EventSignal onkeyup_event_;
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

  Elements children_;
  int width_, height_;
  HostInterface *host_;
  CanvasInterface *canvas_;
  ViewInterface::ResizableMode resizable_;
  std::string caption_;
  bool show_caption_always_;

  std::vector<Event *> event_stack_;
  // NOTE: Here don't use GadgetStringComparator, because even in GDWin's
  // case-insensitive environment, element names are still case sensitive.
  typedef std::map<std::string, ElementInterface *> ElementsMap;
  ElementsMap all_elements_;

  static const unsigned int kAnimationInterval = 10;
  struct TimerInfo {
    int token;
    TimerType type;
    Slot *slot;
    int start_value;
    int last_value;
    int spread;
    unsigned int duration;
    uint64_t start_time;
    void *host_timer;
  };
  typedef std::map<int, TimerInfo> TimerMap;
  TimerMap timer_map_;
  int current_timer_token_;
};

} // namespace internal

} // namespace ggadget

#endif // GGADGET_VIEW_IMPL_H__
