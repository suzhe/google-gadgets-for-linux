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

#include <vector>
#include "view.h"
#include "element_factory_interface.h"
#include "element_interface.h"
#include "elements.h"
#include "event.h"
#include "file_manager_interface.h"
#include "gadget_host_interface.h"
#include "graphics_interface.h"
#include "image.h"
#include "math_utils.h"
#include "script_context_interface.h"
#include "scriptable_binary_data.h"
#include "scriptable_delegator.h"
#include "scriptable_event.h"
#include "slot.h"
#include "texture.h"
#include "view_host_interface.h"
#include "xml_utils.h"

namespace ggadget {

static const char kFTPPrefix[] = "ftp://";
static const char kHTTPPrefix[] = "http://";
static const char kHTTPSPrefix[] = "https://";

class View::Impl {
 public:
  Impl(ViewHostInterface *host,
       ElementFactoryInterface *element_factory,
       int debug_mode,
       View *owner)
    : owner_(owner),
      host_(host),
      gadget_host_(host->GetGadgetHost()),
      script_context_(host->GetScriptContext()),
      file_manager_(NULL),
      element_factory_(element_factory),
      children_(element_factory, NULL, owner),
      debug_mode_(debug_mode),
      width_(0), height_(0),
      // TODO: Make sure the default value.
      resizable_(ViewInterface::RESIZABLE_TRUE),
      show_caption_always_(false),
      focused_element_(NULL),
      mouseover_element_(NULL),
      grabmouse_element_(NULL),
      dragover_element_(NULL),
      dragover_result_(false),
      non_strict_delegator_(new ScriptableDelegator(owner, false)),
      posting_event_element_(NULL),
      post_event_token_(0) {
    if (gadget_host_)
      file_manager_ = gadget_host_->GetFileManager();
  }

  ~Impl() {
    delete non_strict_delegator_;
    non_strict_delegator_ = NULL;

    TimerMap::iterator it = timer_map_.begin();
    while (it != timer_map_.end()) {
      TimerMap::iterator next = it;
      ++next;
      RemoveTimer(it->first);
      it = next;
    }
  }

  bool InitFromFile(const char *filename) {
    if (SetupViewFromFile(owner_, filename)) {
      onopen_event_();
      return true;
    } else {
      return false;
    }
  }

  void MapChildPositionEvent(PositionEvent *org_event, ElementInterface *child,
                             PositionEvent *new_event) {
    ASSERT(child);
    std::vector<ElementInterface *> elements;
    for (ElementInterface *e = child; e != NULL; e = e->GetParentElement())
      elements.push_back(e);

    double x, y;
    ElementInterface *top = *(elements.end() - 1);
    ParentCoordToChildCoord(org_event->GetX(), org_event->GetY(),
                            top->GetPixelX(), top->GetPixelY(),
                            top->GetPixelPinX(), top->GetPixelPinY(),
                            DegreesToRadians(top->GetRotation()),
                            &x, &y);

    for (std::vector<ElementInterface *>::reverse_iterator it =
             elements.rbegin();
         // Note: Don't iterator to the last element.
         it < elements.rend() - 1; ++it) {
      // Make copies to prevent them from being overriden.
      double x1 = x, y1 = y;
      (*it)->SelfCoordToChildCoord(*(it + 1), x1, y1, &x, &y);
    }
    new_event->SetX(x);
    new_event->SetY(y);
  }

  bool SendMouseEventToChildren(MouseEvent *event) {
    Event::Type type = event->GetType();
    if (type == Event::EVENT_MOUSE_OVER)
      // View's EVENT_MOUSE_OVER only applicable to itself.
      // Children's EVENT_MOUSE_OVER is triggered by other mouse events.
      return true;

    bool result = true;
    // If some element is grabbing mouse, send all EVENT_MOUSE_MOVE and
    // EVENT_MOUSE_UP events to it directly, until an EVENT_MOUSE_UP received.
    if (grabmouse_element_ && grabmouse_element_->IsEnabled() &&
        grabmouse_element_->IsVisible() &&
        (type == Event::EVENT_MOUSE_MOVE || type == Event::EVENT_MOUSE_UP)) {
      MouseEvent new_event(*event);
      MapChildPositionEvent(event, grabmouse_element_, &new_event);
      ElementInterface *fired_element;
      result = grabmouse_element_->OnMouseEvent(&new_event, true, 
                                                &fired_element);

      // Release the grabbing.
      if (type == Event::EVENT_MOUSE_UP)
        grabmouse_element_ = NULL;
      return result;
    }

    if (type == Event::EVENT_MOUSE_OUT) {
      // Clear the mouseover state.
      if (mouseover_element_) {
        MouseEvent new_event(*event);
        MapChildPositionEvent(event, mouseover_element_, &new_event);
        ElementInterface *temp;
        result = mouseover_element_->OnMouseEvent(&new_event, true, &temp);
        mouseover_element_ = NULL;
      }
      return result;
    }

    // Dispatch the event to children normally.
    ElementInterface *fired_element = NULL;
    result = children_.OnMouseEvent(event, &fired_element);
    if (fired_element && type == Event::EVENT_MOUSE_DOWN) {
      // Start grabbing.
      grabmouse_element_ = fired_element;
      SetFocus(fired_element);
      // In the focusin handler, the element may be removed and fired_element
      // points to invalid element.  However, grabmouse_element_ will still
      // be valid or has been set to NULL.
      fired_element = grabmouse_element_;
    }

    if (fired_element != mouseover_element_) {
      ElementInterface *old_mouseover_element = mouseover_element_;
      // Store it early to prevent crash if fired_element is removed in
      // the mouseout handler.
      mouseover_element_ = fired_element;

      if (old_mouseover_element) {
        MouseEvent mouseout_event(Event::EVENT_MOUSE_OUT,
                                  event->GetX(), event->GetY(),
                                  event->GetButton(),
                                  event->GetWheelDelta());
        MapChildPositionEvent(event, old_mouseover_element, &mouseout_event);
        ElementInterface *temp;
        old_mouseover_element->OnMouseEvent(&mouseout_event, true, &temp);
      }

      if (mouseover_element_) {
        // The enabled and visible states may change during event handling.
        if (!mouseover_element_->IsEnabled() ||
            !mouseover_element_->IsVisible())
          mouseover_element_ = NULL;
        else {
          MouseEvent mouseover_event(Event::EVENT_MOUSE_OVER,
                                     event->GetX(), event->GetY(),
                                     event->GetButton(),
                                     event->GetWheelDelta());
          MapChildPositionEvent(event, mouseover_element_, &mouseover_event);
          ElementInterface *temp;
          mouseover_element_->OnMouseEvent(&mouseover_event, true, &temp);
        }
      }
    }

    return result;
  }

  bool OnMouseEvent(MouseEvent *event) {
    // Send event to view first.
    ScriptableEvent scriptable_event(event, NULL, 0, 0);
    if (event->GetType() != Event::EVENT_MOUSE_MOVE)
      DLOG("%s(view): %g %g %d %d", scriptable_event.GetName(),
           event->GetX(), event->GetY(),
           event->GetButton(), event->GetWheelDelta());
    switch (event->GetType()) {
      case Event::EVENT_MOUSE_MOVE:
        // Put the high volume events near top.
        // View itself doesn't have onmousemove handler.
        break;
      case Event::EVENT_MOUSE_DOWN:
        FireEvent(&scriptable_event, onmousedown_event_);
        break;
      case Event::EVENT_MOUSE_UP:
        FireEvent(&scriptable_event, onmouseup_event_);
        break;
      case Event::EVENT_MOUSE_CLICK:
        FireEvent(&scriptable_event, onclick_event_);
        break;
      case Event::EVENT_MOUSE_DBLCLICK:
        FireEvent(&scriptable_event, ondblclick_event_);
        break;
      case Event::EVENT_MOUSE_RCLICK:
        FireEvent(&scriptable_event, onrclick_event_);
        break;
      case Event::EVENT_MOUSE_RDBLCLICK:
        FireEvent(&scriptable_event, onrdblclick_event_);
        break;
      case Event::EVENT_MOUSE_OUT:
        FireEvent(&scriptable_event, onmouseout_event_);
        break;
      case Event::EVENT_MOUSE_OVER:
        FireEvent(&scriptable_event, onmouseover_event_);
        break;
      case Event::EVENT_MOUSE_WHEEL:
          // View doesn't have mouse wheel event according to the API document.
          break;
      default:
        ASSERT(false);
    }

    if (scriptable_event.GetReturnValue())
      return SendMouseEventToChildren(event);
    return true;
  }

  bool OnDragEvent(DragEvent *event) {
    Event::Type type = event->GetType();
    if (type == Event::EVENT_DRAG_OUT || type == Event::EVENT_DRAG_DROP) {
      bool result = false;
      // Send the event and clear the dragover state.
      if (dragover_element_) {
        // If the element rejects the drop, send a EVENT_DRAG_OUT
        // on EVENT_DRAG_DROP.
        if (!dragover_result_)
          type = Event::EVENT_DRAG_OUT;
        DragEvent new_event(type, event->GetX(), event->GetY(),
                            event->GetDragFiles());
        MapChildPositionEvent(event, dragover_element_, &new_event);
        ElementInterface *temp;
        result = dragover_element_->OnDragEvent(&new_event, true, &temp);
        dragover_element_ = NULL;
      }
      return result;
    }

    ASSERT(type == Event::EVENT_DRAG_MOTION);
    // Dispatch the event to children normally.
    ElementInterface *fired_element = NULL;
    children_.OnDragEvent(event, &fired_element);
    if (fired_element != dragover_element_) {
      dragover_result_ = false;
      ElementInterface *old_dragover_element = dragover_element_;
      // Store it early to prevent crash if fired_element is removed in
      // the dragout handler.
      dragover_element_ = fired_element;

      if (old_dragover_element) {
        DragEvent dragout_event(Event::EVENT_DRAG_OUT,
                                event->GetX(), event->GetY(),
                                event->GetDragFiles());
        MapChildPositionEvent(event, old_dragover_element, &dragout_event);
        ElementInterface *temp;
        old_dragover_element->OnDragEvent(&dragout_event, true, &temp);
      }

      if (dragover_element_) {
        // The visible state may change during event handling.
        if (!dragover_element_->IsVisible())
          dragover_element_ = NULL;
        else {
          DragEvent dragover_event(Event::EVENT_DRAG_OVER,
                                   event->GetX(), event->GetY(),
                                   event->GetDragFiles());
          MapChildPositionEvent(event, dragover_element_, &dragover_event);
          ElementInterface *temp;
          dragover_result_ = dragover_element_->OnDragEvent(&dragover_event,
                                                            true, &temp);
        }
      }
    }

    // Because gadget elements has no handler for EVENT_DRAG_MOTION, the
    // last return result of EVENT_DRAG_OVER should be used as the return result
    // of EVENT_DRAG_MOTION.
    return dragover_result_;
  }

  bool OnKeyEvent(KeyboardEvent *event) {
    ScriptableEvent scriptable_event(event, NULL, 0, 0);
    // TODO: dispatch to children.
    DLOG("%s(view): %d", scriptable_event.GetName(), event->GetKeyCode());
    switch (event->GetType()) {
      case Event::EVENT_KEY_DOWN:
        FireEvent(&scriptable_event, onkeydown_event_);
        break;
      case Event::EVENT_KEY_UP:
        FireEvent(&scriptable_event, onkeyup_event_);
        break;
      case Event::EVENT_KEY_PRESS:
        FireEvent(&scriptable_event, onkeypress_event_);
        break;
      default:
        ASSERT(false);
    }

    if (!scriptable_event.GetReturnValue())
      return false;

    if (focused_element_) {
      if (!focused_element_->IsEnabled() || !focused_element_->IsVisible())
        focused_element_ = NULL;
      else
        return focused_element_->OnKeyEvent(event);
    }
    return true;
  }

  bool OnOtherEvent(Event *event) {
    ScriptableEvent scriptable_event(event, NULL, 0, 0);
    DLOG("%s(view)", scriptable_event.GetName());
    switch (event->GetType()) {
      case Event::EVENT_FOCUS_IN:
        // For now we don't automatically set focus to some element.
        break;
      case Event::EVENT_FOCUS_OUT:
        SetFocus(NULL);
        break;
      case Event::EVENT_OK:
        FireEvent(&scriptable_event, onok_event_);
        break;
      case Event::EVENT_CANCEL:
        FireEvent(&scriptable_event, oncancel_event_);
        break;
      default:
        ASSERT(false);
    }
    // TODO: Returns false if a cancelable event is canceled.
    return true;
  }

  void OnElementAdd(ElementInterface *element) {
    ASSERT(element);
    const char *name = element->GetName();
    if (name && *name &&
        // Don't overwrite the existing element with the same name.
        all_elements_.find(name) == all_elements_.end())
      all_elements_[name] = element;
  }

  void OnElementRemove(ElementInterface *element) {
    ASSERT(element);
    if (element == focused_element_)
      // Don't send EVENT_FOCUS_OUT because the element is being removed.
      focused_element_ = NULL;
    if (element == mouseover_element_)
      mouseover_element_ = NULL;
    if (element == grabmouse_element_)
      grabmouse_element_ = NULL;
    if (element == dragover_element_)
      dragover_element_ = NULL;

    for (PostedEvents::iterator it = posted_events_.begin();
         it != posted_events_.end(); ++it) {
      if (it->first && it->first->GetSrcElement() == element) {
        // The source element of the posted event has been removed,
        // clean up the event.
        delete it->first->GetEvent();
        delete it->first;
        it->first = NULL;
      }
    }
    if (posting_event_element_ == element)
      posting_event_element_ = NULL;

    const char *name = element->GetName();
    if (name && *name) {
      ElementsMap::iterator it = all_elements_.find(name);
      if (it != all_elements_.end() && it->second == element)
        all_elements_.erase(it);
    }
  }

  void FireEvent(ScriptableEvent *event, const EventSignal &event_signal) {
    // Note: there is another place also do the similar thing: OnTimerEvent().
    event_stack_.push_back(event);
    event_signal();
    event_stack_.pop_back();
  }

  bool FirePostedEvents(int token) {
    ASSERT(token == post_event_token_);
    post_event_token_ = 0;

    // Make a copy of posted_events_, because it may change during the
    // following loop.
    PostedEvents posted_events_copy;
    std::swap(posted_events_, posted_events_copy);
    for (PostedEvents::iterator it = posted_events_copy.begin();
         it != posted_events_copy.end(); ++it) {
      if (it->first) { // The event is still valid.
        posting_event_element_ = it->first->GetSrcElement();
        FireEvent(it->first, *it->second);
        if (posting_event_element_) {
          // The element is still there, clean the event up.
          delete it->first->GetEvent();
          delete it->first;
          it->first = NULL;
        }
      }
    }
    posted_events_copy.clear();
    posting_event_element_ = NULL;
    return false;
  }

  void PostEvent(ScriptableEvent *event, const EventSignal &event_signal) {
    ASSERT(event);
    ASSERT(event->GetEvent());
    // Merge multiple size events into one for each element.
    if (event->GetEvent()->GetType() == Event::EVENT_SIZE) {
      for (PostedEvents::const_iterator it = posted_events_.begin();
           it != posted_events_.end(); ++it) {
        if (it->first && // The event is still valid.
            it->first->GetEvent()->GetType() == Event::EVENT_SIZE &&
            it->first->GetSrcElement() == event->GetSrcElement()) {
          delete event->GetEvent();
          delete event;
          // The size event already posted for this element.
          return;
        }
      }
    }
    posted_events_.push_back(std::make_pair(event, &event_signal));

    if (!post_event_token_) {
      post_event_token_ = gadget_host_->RegisterTimer(
          0, NewSlot(this, &Impl::FirePostedEvents));
    }
  }

  ScriptableEvent *GetEvent() const {
    return event_stack_.empty() ? NULL : event_stack_[event_stack_.size() - 1];
  }

  void SetFocus(ElementInterface *element) {
    if (element != focused_element_) {
      ElementInterface *old_focused_element = focused_element_;
      // Set it early to prevent the local "element" variable from being stale
      // if the element is removed in the event handler.
      focused_element_ = element;
      // Remove the current focus first.
      if (old_focused_element) {
        Event event(Event::EVENT_FOCUS_OUT);
        old_focused_element->OnOtherEvent(&event);
      }

      if (focused_element_) {
        if (!focused_element_->IsEnabled() || !focused_element_->IsVisible())
          focused_element_ = NULL;
        else {
          Event event(Event::EVENT_FOCUS_IN);
          focused_element_->OnOtherEvent(&event);
        }
      }
    }
  }

  bool SetWidth(int width) {
    return SetSize(width, height_);
  }

  bool SetHeight(int height) {
    return SetSize(width_, height);
  }

  bool SetSize(int width, int height) {
    if (width != width_ || height != height_) {
      // TODO check if allowed first
      if (width != width_) {
        width_ = width;
        children_.OnParentWidthChange(width);
      }
      if (height != height_) {
        height_ = height;
        children_.OnParentHeightChange(height);
      }
      host_->QueueDraw();

      Event event(Event::EVENT_SIZE);
      ScriptableEvent scriptable_event(&event, NULL, 0, 0);
      FireEvent(&scriptable_event, onsize_event_);
    }
    return true;
  }

  bool ResizeBy(int width, int height) {
    return SetSize(width_ + width, height_ + height);
  }

  const CanvasInterface *Draw(bool *changed) {
    return children_.Draw(changed);
  }

  ElementInterface *GetElementByName(const char *name) {
    ElementsMap::iterator it = all_elements_.find(name);
    return it == all_elements_.end() ? NULL : it->second;
  }

  // For script.
  Variant GetElementByNameVariant(const char *name) {
    ElementInterface *result = GetElementByName(name);
    return result ? Variant(result) : Variant();
  }

  enum TimerType {
    TIMER_ANIMATION,
    // Used to indicate the first immediate timer event of an animation.
    TIMER_ANIMATION_FIRST,
    TIMER_TIMEOUT,
    TIMER_INTERVAL
  };
  int NewTimer(TimerType type, Slot *slot,
               int start_value, int end_value, unsigned int duration) {
    ASSERT(slot);

    TimerInfo *info = new TimerInfo();
    info->type = type;
    info->slot = slot;
    info->start_value = start_value;
    info->last_value = start_value;
    info->spread = end_value - start_value;
    info->duration = duration;
    info->start_time = gadget_host_->GetCurrentTime();
    info->token = gadget_host_->RegisterTimer(
        type == TIMER_ANIMATION ? kAnimationInterval : duration,
        NewSlot(this, &Impl::TimerCallback));
    timer_map_[info->token] = info;
    return info->token;
  }

  void RemoveTimer(int token) {
    if (token == 0)
      return;

    TimerMap::iterator it = timer_map_.find(token);
    if (it == timer_map_.end()) {
      LOG("Invalid timer token to remove: %d", token);
      return;
    }

    gadget_host_->RemoveTimer(it->second->token);
    // TIMER_ANIMATION_FIRST and TIMER_ANIMATION share the slot.
    if (it->second->type != TIMER_ANIMATION_FIRST)
      delete it->second->slot;
    delete it->second;
    timer_map_.erase(it);
  }

  bool TimerCallback(int token) {
    bool wants_more = true;
    TimerMap::iterator it = timer_map_.find(token);
    if (it == timer_map_.end()) {
      DLOG("Timer has been removed but event still fired: %d", token);
      return false;
    }
    TimerInfo *info = it->second;

    if (info->type != TIMER_TIMEOUT &&
        gadget_host_->GetCurrentTime() - info->last_finished_time <
            kMinInterval * 1000) {
      // To avoid some frequent interval/animation timers or slow handlers
      // eat up all CPU resources, here automaticlly ignores some timer events.
      DLOG("Automatically ignored a timer event");
      return true;
    }

    Event event(Event::EVENT_TIMER_TICK);
    switch (info->type) {
      case TIMER_TIMEOUT: {
        ScriptableEvent scriptable_event(&event, NULL, token, 0);
        event_stack_.push_back(&scriptable_event);
        info->slot->Call(0, NULL);
        wants_more = false;
        RemoveTimer(token);
        event_stack_.pop_back();
        break;
      }
      case TIMER_INTERVAL: {
        ScriptableEvent scriptable_event(&event, NULL, token, 0);
        event_stack_.push_back(&scriptable_event);
        info->slot->Call(0, NULL);
        event_stack_.pop_back();
        break;
      }
      case TIMER_ANIMATION_FIRST: {
        ScriptableEvent scriptable_event(&event, NULL, token,
                                         info->start_value);
        event_stack_.push_back(&scriptable_event);
        info->slot->Call(0, NULL);
        wants_more = false;
        RemoveTimer(token);
        break;
      }
      case TIMER_ANIMATION: {
        double progress = 1.0;
        if (info->duration > 0) {
          uint64_t event_time = gadget_host_->GetCurrentTime();
          progress = static_cast<double>(event_time - info->start_time) /
                     1000.0 / info->duration;
          progress = std::min(1.0, std::max(0.0, progress));
        }

        int value = info->start_value +
                    static_cast<int>(round(progress * info->spread));
        if (value != info->last_value) {
          ScriptableEvent scriptable_event(&event, NULL, token, value);
          event_stack_.push_back(&scriptable_event);
          info->last_value = value;
          info->slot->Call(0, NULL);
          event_stack_.pop_back();
        }

        if (progress == 1.0) {
          wants_more = false;
          RemoveTimer(token);
        }
        break;
      }
      default:
        ASSERT(false);
        break;
    }

    // Record the time when this event is finished processing.
    // Must check if the token is still there to ensure info pointer is valid.
    if (timer_map_.find(token) != timer_map_.end())
      info->last_finished_time = gadget_host_->GetCurrentTime();
    return wants_more;
  }

  int BeginAnimation(Slot *slot, int start_value, int end_value,
                     unsigned int duration) {
    // Create an additional timer which fires immediately.
    NewTimer(TIMER_ANIMATION_FIRST, slot, start_value, end_value, 0);
    return NewTimer(TIMER_ANIMATION, slot, start_value, end_value, duration);
  }

  void CancelAnimation(int token) {
    RemoveTimer(token);
  }

  int SetTimeout(Slot *slot, unsigned int duration) {
    return NewTimer(TIMER_TIMEOUT, slot, 0, 0, duration);
  }

  void ClearTimeout(int token) {
    RemoveTimer(token);
  }

  int SetInterval(Slot *slot, unsigned int duration) {
    return NewTimer(TIMER_INTERVAL, slot, 0, 0, duration);
  }

  void ClearInterval(int token) {
    RemoveTimer(token);
  }

  void OnOptionChanged(const char *name) {
    OptionChangedEvent event(name);
    ScriptableEvent scriptable_event(&event, NULL, 0, 0);
    FireEvent(&scriptable_event, onoptionchanged_event_);
  }

  // The implementation uses if-else statements, which seems the best
  // trade-off among code size, data size, calling frequency and time.
  // This method is seldom called only by C++ code.
  Connection *ConnectEvent(const char *event_name, Slot0<void> *handler) {
    ASSERT(event_name);
    EventSignal *signal = NULL;
    if (GadgetStrCmp(event_name, kOnCancelEvent) == 0)
      signal = &oncancel_event_;
    else if (GadgetStrCmp(event_name, kOnClickEvent) == 0)
      signal = &onclick_event_;
    else if (GadgetStrCmp(event_name, kOnCloseEvent) == 0)
      signal = &onclose_event_;
    else if (GadgetStrCmp(event_name, kOnDblClickEvent) == 0)
      signal = &ondblclick_event_;
    else if (GadgetStrCmp(event_name, kOnRClickEvent) == 0)
      signal = &onrclick_event_;
    else if (GadgetStrCmp(event_name, kOnRDblClickEvent) == 0)
      signal = &onrdblclick_event_;  
    else if (GadgetStrCmp(event_name, kOnDockEvent) == 0)
      signal = &ondock_event_;
    else if (GadgetStrCmp(event_name, kOnKeyDownEvent) == 0)
      signal = &onkeydown_event_;
    else if (GadgetStrCmp(event_name, kOnKeyPressEvent) == 0)
      signal = &onkeypress_event_;
    else if (GadgetStrCmp(event_name, kOnKeyUpEvent) == 0)
      signal = &onkeyup_event_;
    else if (GadgetStrCmp(event_name, kOnMinimizeEvent) == 0)
      signal = &onminimize_event_;
    else if (GadgetStrCmp(event_name, kOnMouseDownEvent) == 0)
      signal = &onmousedown_event_;
    else if (GadgetStrCmp(event_name, kOnMouseOutEvent) == 0)
      signal = &onmouseout_event_;
    else if (GadgetStrCmp(event_name, kOnMouseOverEvent) == 0)
      signal = &onmouseover_event_;
    else if (GadgetStrCmp(event_name, kOnMouseUpEvent) == 0)
      signal = &onmouseup_event_;
    else if (GadgetStrCmp(event_name, kOnOkEvent) == 0)
      signal = &onok_event_;
    else if (GadgetStrCmp(event_name, kOnOpenEvent) == 0)
      signal = &onopen_event_;
    else if (GadgetStrCmp(event_name, kOnOptionChangedEvent) == 0)
      signal = &onoptionchanged_event_;
    else if (GadgetStrCmp(event_name, kOnPopInEvent) == 0)
      signal = &onpopin_event_;
    else if (GadgetStrCmp(event_name, kOnPopOutEvent) == 0)
      signal = &onpopout_event_;
    else if (GadgetStrCmp(event_name, kOnRestoreEvent) == 0)
      signal = &onrestore_event_;
    else if (GadgetStrCmp(event_name, kOnSizeEvent) == 0)
      signal = &onsize_event_;
    else if (GadgetStrCmp(event_name, kOnSizingEvent) == 0)
      signal = &onsizing_event_;
    else if (GadgetStrCmp(event_name, kOnUndockEvent) == 0)
      signal = &onundock_event_;

    ASSERT_M(signal, ("Unknown event name: %s", event_name));
    return signal->Connect(handler);
  }

  EventSignal oncancel_event_;
  EventSignal onclick_event_;
  EventSignal onclose_event_;
  EventSignal ondblclick_event_;
  EventSignal onrclick_event_;
  EventSignal onrdblclick_event_;
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

  // Put all_elements_ here to make it the alst member to be destructed,
  // because destruction of children_ needs it.
  typedef std::map<std::string, ElementInterface *, GadgetStringComparator>
      ElementsMap;
  ElementsMap all_elements_;

  View *owner_;
  ViewHostInterface *host_;
  GadgetHostInterface *gadget_host_;
  ScriptContextInterface *script_context_;
  FileManagerInterface *file_manager_;
  ElementFactoryInterface *element_factory_;
  Elements children_;
  int debug_mode_;
  int width_, height_;
  ViewInterface::ResizableMode resizable_;
  std::string caption_;
  bool show_caption_always_;

  std::vector<ScriptableEvent *> event_stack_;

  static const unsigned int kAnimationInterval = 20;
  static const unsigned int kMinInterval = 5;
  struct TimerInfo {
    Impl *view;
    int token;
    TimerType type;
    Slot *slot;
    int start_value;
    int last_value;
    int spread;
    unsigned int duration;
    uint64_t start_time;
    // The time when the last event was finished processing.
    uint64_t last_finished_time;
  };
  typedef std::map<int, TimerInfo *> TimerMap;
  TimerMap timer_map_;
  ElementInterface *focused_element_;
  ElementInterface *mouseover_element_;
  ElementInterface *grabmouse_element_;
  ElementInterface *dragover_element_;
  bool dragover_result_;

  ScriptableDelegator *non_strict_delegator_;

  typedef std::vector<std::pair<ScriptableEvent *, const EventSignal *> >
      PostedEvents;
  PostedEvents posted_events_;
  ElementInterface *posting_event_element_;
  int post_event_token_;
};

static const char *kResizableNames[] = { "false", "true", "zoom" };

View::View(ViewHostInterface *host,
           ScriptableInterface *prototype,
           ElementFactoryInterface *element_factory,
           int debug_mode)
    : impl_(new Impl(host, element_factory, debug_mode, this)) {
  RegisterProperty("caption", NewSlot(this, &View::GetCaption),
                   NewSlot(this, &View::SetCaption));
  RegisterConstant("children", GetChildren());
  RegisterProperty("event", NewSlot(impl_, &Impl::GetEvent), NULL);
  RegisterProperty("height", NewSlot(this, &View::GetHeight),
                   NewSlot(this, &View::SetHeight));
  RegisterProperty("width", NewSlot(this, &View::GetWidth),
                   NewSlot(this, &View::SetWidth));
  RegisterStringEnumProperty("resizable", NewSlot(this, &View::GetResizable),
                             NewSlot(this, &View::SetResizable),
                             kResizableNames, arraysize(kResizableNames));
  RegisterProperty("showCaptionAlways",
                   NewSlot(this, &View::GetShowCaptionAlways),
                   NewSlot(this, &View::SetShowCaptionAlways));
  // The global view object is itself.
  RegisterConstant("view", this);

  RegisterMethod("appendElement",
                 NewSlot(GetChildren(), &Elements::AppendElementFromXML));
  RegisterMethod("insertElement",
                 NewSlot(GetChildren(), &Elements::InsertElementFromXML));
  RegisterMethod("removeElement",
                 NewSlot(GetChildren(), &Elements::RemoveElement));
  RegisterMethod("removeAllElements",
                 NewSlot(GetChildren(), &Elements::RemoveAllElements));

  // Here register ViewImpl::BeginAnimation because the Slot1<void, int> *
  // parameter in View::BeginAnimation can't be automatically reflected.
  RegisterMethod("beginAnimation", NewSlot(impl_, &Impl::BeginAnimation));
  RegisterMethod("cancelAnimation", NewSlot(impl_, &Impl::CancelAnimation));
  RegisterMethod("setTimeout", NewSlot(impl_, &Impl::SetTimeout));
  RegisterMethod("clearTimeout", NewSlot(impl_, &Impl::ClearTimeout));
  RegisterMethod("setInterval", NewSlot(impl_, &Impl::SetInterval));
  RegisterMethod("clearInterval", NewSlot(impl_, &Impl::ClearInterval));

  RegisterMethod("alert", NewSlot(this, &View::Alert));
  RegisterMethod("confirm", NewSlot(this, &View::Confirm));
  RegisterMethod("prompt", NewSlot(this, &View::Prompt));

  RegisterMethod("resizeBy", NewSlot(impl_, &Impl::ResizeBy));
  RegisterMethod("resizeTo", NewSlot(this, &View::SetSize));

  RegisterSignal(kOnCancelEvent, &impl_->oncancel_event_);
  RegisterSignal(kOnClickEvent, &impl_->onclick_event_);
  RegisterSignal(kOnCloseEvent, &impl_->onclose_event_);
  RegisterSignal(kOnDblClickEvent, &impl_->ondblclick_event_);
  RegisterSignal(kOnRClickEvent, &impl_->onrclick_event_);
  RegisterSignal(kOnRDblClickEvent, &impl_->onrdblclick_event_);  
  RegisterSignal(kOnDockEvent, &impl_->ondock_event_);
  RegisterSignal(kOnKeyDownEvent, &impl_->onkeydown_event_);
  RegisterSignal(kOnKeyPressEvent, &impl_->onkeypress_event_);
  RegisterSignal(kOnKeyUpEvent, &impl_->onkeyup_event_);
  RegisterSignal(kOnMinimizeEvent, &impl_->onminimize_event_);
  RegisterSignal(kOnMouseDownEvent, &impl_->onmousedown_event_);
  RegisterSignal(kOnMouseOutEvent, &impl_->onmouseout_event_);
  RegisterSignal(kOnMouseOverEvent, &impl_->onmouseover_event_);
  RegisterSignal(kOnMouseUpEvent, &impl_->onmouseup_event_);
  RegisterSignal(kOnOkEvent, &impl_->onok_event_);
  RegisterSignal(kOnOpenEvent, &impl_->onopen_event_);
  RegisterSignal(kOnOptionChangedEvent, &impl_->onoptionchanged_event_);
  RegisterSignal(kOnPopInEvent, &impl_->onpopin_event_);
  RegisterSignal(kOnPopOutEvent, &impl_->onpopout_event_);
  RegisterSignal(kOnRestoreEvent, &impl_->onrestore_event_);
  RegisterSignal(kOnSizeEvent, &impl_->onsize_event_);
  RegisterSignal(kOnSizingEvent, &impl_->onsizing_event_);
  RegisterSignal(kOnUndockEvent, &impl_->onundock_event_);

  SetDynamicPropertyHandler(NewSlot(impl_, &Impl::GetElementByNameVariant),
                            NULL);

  if (prototype)
    SetPrototype(prototype);

  if (impl_->script_context_)
    impl_->script_context_->SetGlobalObject(impl_->non_strict_delegator_);
}

View::~View() {
  delete impl_;
  impl_ = NULL;
}

ScriptContextInterface *View::GetScriptContext() const {
  return impl_->script_context_;
}

FileManagerInterface *View::GetFileManager() const {
  return impl_->file_manager_;
}

bool View::InitFromFile(const char *filename) {
  return impl_->InitFromFile(filename);
}

int View::GetWidth() const {
  return impl_->width_;
}

int View::GetHeight() const {
  return impl_->height_;
}

const CanvasInterface *View::Draw(bool *changed) {
  return impl_->Draw(changed);
}

void View::QueueDraw() {
  impl_->host_->QueueDraw();
}

const GraphicsInterface *View::GetGraphics() const {
  return impl_->host_->GetGraphics();
}

bool View::OnMouseEvent(MouseEvent *event) {
  return impl_->OnMouseEvent(event);
}

bool View::OnKeyEvent(KeyboardEvent *event) {
  return impl_->OnKeyEvent(event);
}

bool View::OnDragEvent(DragEvent *event) {
  return impl_->OnDragEvent(event);
}

bool View::OnOtherEvent(Event *event) {
  return impl_->OnOtherEvent(event);
}

void View::OnElementAdd(ElementInterface *element) {
  impl_->OnElementAdd(element);
}

void View::OnElementRemove(ElementInterface *element) {
  impl_->OnElementRemove(element);
}

void View::FireEvent(ScriptableEvent *event, const EventSignal &event_signal) {
  impl_->FireEvent(event, event_signal);
}

void View::PostEvent(ScriptableEvent *event, const EventSignal &event_signal) {
  impl_->PostEvent(event, event_signal);
}

ScriptableEvent *View::GetEvent() {
  return impl_->GetEvent();
}

const ScriptableEvent *View::GetEvent() const {
  return impl_->GetEvent();
}

bool View::SetWidth(int width) {
  return impl_->SetWidth(width);
}

bool View::SetHeight(int height) {
  return impl_->SetHeight(height);
}

bool View::SetSize(int width, int height) {
  return impl_->SetSize(width, height);
}

void View::SetResizable(ViewInterface::ResizableMode resizable) {
  impl_->resizable_ = resizable;
  impl_->host_->SetResizable(resizable);
}

ElementFactoryInterface *View::GetElementFactory() const {
  return impl_->element_factory_;
}

const Elements *View::GetChildren() const {
  return &impl_->children_;
}

Elements *View::GetChildren() {
  return &impl_->children_;
}

ElementInterface *View::GetElementByName(const char *name) {
  return impl_->GetElementByName(name);
}

const ElementInterface *View::GetElementByName(const char *name) const {
  return impl_->GetElementByName(name);
}

ViewInterface::ResizableMode View::GetResizable() const {
  return impl_->resizable_;
}

void View::SetCaption(const char *caption) {
  impl_->caption_ = caption ? caption : NULL;
  impl_->host_->SetCaption(caption);
}

const char *View::GetCaption() const {
  return impl_->caption_.c_str();
}

void View::SetShowCaptionAlways(bool show_always) {
  impl_->show_caption_always_ = show_always;
  impl_->host_->SetShowCaptionAlways(show_always);
}

bool View::GetShowCaptionAlways() const {
  return impl_->show_caption_always_;
}

int View::BeginAnimation(Slot0<void> *slot,
                         int start_value,
                         int end_value,
                         unsigned int duration) {
  return impl_->BeginAnimation(slot, start_value, end_value, duration);
}

void View::CancelAnimation(int token) {
  impl_->CancelAnimation(token);
}

int View::SetTimeout(Slot0<void> *slot, unsigned int duration) {
  return impl_->SetTimeout(slot, duration);
}

void View::ClearTimeout(int token) {
  impl_->ClearTimeout(token);
}

int View::SetInterval(Slot0<void> *slot, unsigned int duration) {
  return impl_->SetInterval(slot, duration);
}

void View::ClearInterval(int token) {
  impl_->ClearInterval(token);
}

int View::GetDebugMode() const {
  return impl_->debug_mode_;
}

Image *View::LoadImage(const Variant &src, bool is_mask) {
  ASSERT(impl_->file_manager_);
  switch (src.type()) {
    case Variant::TYPE_STRING:
      return new Image(GetGraphics(), impl_->file_manager_,
                       VariantValue<const char *>()(src), is_mask);
    case Variant::TYPE_SCRIPTABLE:
    case Variant::TYPE_CONST_SCRIPTABLE: {
      const ScriptableBinaryData *binary =
          VariantValue<const ScriptableBinaryData *>()(src);
      if (binary)
        return new Image(GetGraphics(), binary->data(), binary->size(),
                         is_mask);
      // else falls through!
    }
    default:
      LOG("Unsupported type of image src.");
      DLOG("src=%s", src.ToString().c_str());
      return NULL;
  }
}

Image *View::LoadImageFromGlobal(const char *name, bool is_mask) {
  return new Image(GetGraphics(), impl_->gadget_host_->GetGlobalFileManager(),
                   name, is_mask);
}

Texture *View::LoadTexture(const Variant &src) {
  ASSERT(impl_->file_manager_);
  switch (src.type()) {
    case Variant::TYPE_STRING:
      return new Texture(GetGraphics(), impl_->file_manager_,
                         VariantValue<const char *>()(src));
    case Variant::TYPE_SCRIPTABLE:
    case Variant::TYPE_CONST_SCRIPTABLE: {
      const ScriptableBinaryData *binary =
          VariantValue<const ScriptableBinaryData *>()(src);
      if (binary)
        return new Texture(GetGraphics(), binary->data(), binary->size());
      // else falls through!
    }
    default:
      LOG("Unsupported type of texture src.");
      DLOG("src=%s", src.ToString().c_str());
      return NULL;
  }
}

void View::SetFocus(ElementInterface *element) {
  impl_->SetFocus(element);
}

bool View::OpenURL(const char *url) const {
  // Important: verify that URL is valid first. 
  // Otherwise could be a security problem.
  std::string newurl = EncodeURL(url);
  if (0 == strncmp(newurl.c_str(), kFTPPrefix, sizeof(kFTPPrefix) - 1) ||
      0 == strncmp(newurl.c_str(), kHTTPPrefix, sizeof(kHTTPPrefix) - 1) ||
      0 == strncmp(newurl.c_str(), kHTTPSPrefix, sizeof(kHTTPSPrefix) - 1)) {  
    return impl_->gadget_host_->OpenURL(newurl.c_str());
  }

  DLOG("Malformed URL: %s", newurl.c_str());
  return false;
}

void View::OnOptionChanged(const char *name) {
  impl_->OnOptionChanged(name);
}

Connection *View::ConnectEvent(const char *event_name,
                               Slot0<void> *handler) {
  return impl_->ConnectEvent(event_name, handler);
}

void View::Alert(const char *message) {
  impl_->host_->Alert(message);
}

bool View::Confirm(const char *message) {
  return impl_->host_->Confirm(message);
}

std::string View::Prompt(const char *message, const char *default_result) {
  return impl_->host_->Prompt(message, default_result);
}

} // namespace ggadget
