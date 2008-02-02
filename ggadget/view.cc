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
#include "basic_element.h"
#include "contentarea_element.h"
#include "content_item.h"
#include "details_view.h"
#include "element_factory.h"
#include "elements.h"
#include "event.h"
#include "file_manager_interface.h"
#include "main_loop_interface.h"
#include "gadget_consts.h"
#include "gadget_host_interface.h"
#include "gadget_interface.h"
#include "graphics_interface.h"
#include "image_interface.h"
#include "logger.h"
#include "math_utils.h"
#include "script_context_interface.h"
#include "scriptable_binary_data.h"
#include "scriptable_event.h"
#include "scriptable_image.h"
#include "slot.h"
#include "texture.h"
#include "view_host_interface.h"
#include "xml_dom_interface.h"
#include "xml_http_request.h"
#include "xml_parser_interface.h"
#include "xml_utils.h"

namespace ggadget {

static const char *kResizableNames[] = { "false", "true", "zoom" };

class View::Impl {
 public:
  class GlobalObject : public ScriptableHelperNativeOwnedDefault {
   public:
    DEFINE_CLASS_ID(0x23840d38ed164ab2, ScriptableInterface);
    virtual bool IsStrict() const { return false; }
  };

  // Old "utils" global object, for backward compatibility.
  class Utils : public ScriptableHelperNativeOwnedDefault {
   public:
    DEFINE_CLASS_ID(0x7b7e1dd0b91f4153, ScriptableInterface);
    Utils(Impl *impl) : impl_(impl) {
      RegisterMethod("loadImage", NewSlot(this, &Utils::LoadImage));
      RegisterMethod("setTimeout", NewSlot(impl, &Impl::SetTimeout));
      RegisterMethod("clearTimeout", NewSlot(impl, &Impl::RemoveTimer));
      RegisterMethod("setInterval", NewSlot(impl, &Impl::SetInterval));
      RegisterMethod("clearInterval", NewSlot(impl, &Impl::RemoveTimer));
      RegisterMethod("alert", NewSlot(impl->owner_, &View::Alert));
      RegisterMethod("confirm", NewSlot(impl->owner_, &View::Confirm));
      RegisterMethod("prompt", NewSlot(impl->owner_, &View::Prompt));
    }
    // Just return the original image_src directly, to let the receiver
    // actually load it.
    ScriptableImage *LoadImage(const Variant &image_src) {
      ImageInterface *image = impl_->owner_->LoadImage(image_src, false);
      return image ? new ScriptableImage(image) : NULL;
    }

    Impl *impl_;
  };

  /**
   * Callback object for timer watches.
   * if duration > 0 then it's a animation timer.
   * else if duration == 0 then it's a timeout timer.
   * else if duration < 0 then it's a interval timer.
   */
  class TimerWatchCallback : public WatchCallbackInterface {
   public:
    TimerWatchCallback(Impl *impl, Slot *slot, int start, int end,
                       int duration, uint64_t start_time,
                       bool is_event)
      : impl_(impl), slot_(slot), start_(start), end_(end),
        duration_(duration), start_time_(start_time), last_value_(start),
        watch_id_(0), is_event_(is_event), destroy_connection_(NULL) {
      destroy_connection_ = impl_->on_destroy_signal_.Connect(
          NewSlot(this, &TimerWatchCallback::OnDestroy));
    }

    void SetWatchId(int watch_id) { watch_id_ = watch_id; }

    virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
      bool fire = true;
      bool ret = true;
      int value = 0;
      uint64_t current_time = main_loop->GetCurrentTime();

      // Animation timer
      if (duration_ > 0) {
        double progress =
            static_cast<double>(current_time - start_time_) / duration_;
        progress = std::min(1.0, std::max(0.0, progress));
        value = start_ + static_cast<int>(round(progress * (end_ - start_)));
        fire = (value != last_value_);
        ret = (progress < 1.0);
        last_value_ = value;
      } else if (duration_ == 0) {
        ret = false;
      }

      if (fire && (duration_ == 0 ||
                   current_time - last_finished_time_ > kMinInterval)) {
        if (is_event_) {
          TimerEvent event(watch_id, value);
          ScriptableEvent scriptable_event(&event, impl_->owner_, NULL);
          impl_->FireEventSlot(&scriptable_event, slot_);
        } else {
          slot_->Call(0, NULL);
        }
      }

      last_finished_time_ = main_loop->GetCurrentTime();
      return ret;
    }

    virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
      destroy_connection_->Disconnect();
      delete slot_;
      delete this;
    }

    void OnDestroy() {
      impl_->RemoveTimer(watch_id_);
    }

   private:
    Impl *impl_;
    Slot *slot_;
    int start_;
    int end_;
    int duration_;
    uint64_t start_time_;
    uint64_t last_finished_time_;
    int last_value_;
    int watch_id_;
    bool is_event_;
    Connection *destroy_connection_;
  };

  Impl(ElementFactory *element_factory,
       int debug_mode,
       View *owner)
    : owner_(owner),
      host_(NULL),
      gadget_host_(NULL),
      script_context_(NULL),
      file_manager_(NULL),
      element_factory_(element_factory),
      main_loop_(GetGlobalMainLoop()),
      children_(element_factory, NULL, owner),
      debug_mode_(debug_mode),
      width_(0), height_(0),
      // TODO: Make sure the default value.
      resizable_(ViewInterface::RESIZABLE_ZOOM),
      show_caption_always_(false),
      dragover_result_(EVENT_RESULT_UNHANDLED),
      post_event_token_(0),
      mark_redraw_token_(0),
      draw_queued_(false),
      utils_(this) {
  }

  ~Impl() {
    ASSERT(event_stack_.empty());
    ASSERT(death_detected_elements_.empty());
    SimpleEvent event(Event::EVENT_CLOSE);
    ScriptableEvent scriptable_event(&event, owner_, NULL);
    FireEvent(&scriptable_event, onclose_event_);
    on_destroy_signal_.Emit(0, NULL);
  }

  template <typename T>
  void RegisterProperties(T *obj) {
    obj->RegisterProperty("caption", NewSlot(owner_, &View::GetCaption),
                          NewSlot(owner_, &View::SetCaption));
    obj->RegisterProperty("event", NewSlot(this, &Impl::GetEvent), NULL);
    obj->RegisterProperty("height",
                          NewSlot(owner_, &View::GetHeight),
                          NewSlot(owner_, &View::SetHeight));
    obj->RegisterProperty("width",
                          NewSlot(owner_, &View::GetWidth),
                          NewSlot(owner_, &View::SetWidth));
    obj->RegisterStringEnumProperty("resizable",
                                    NewSlot(owner_, &View::GetResizable),
                                    NewSlot(owner_, &View::SetResizable),
                                    kResizableNames,
                                    arraysize(kResizableNames));
    obj->RegisterProperty("showCaptionAlways",
                          NewSlot(owner_, &View::GetShowCaptionAlways),
                          NewSlot(owner_, &View::SetShowCaptionAlways));

    obj->RegisterConstant("children", &children_);
    obj->RegisterMethod("appendElement",
                        NewSlot(&children_, &Elements::AppendElementFromXML));
    obj->RegisterMethod("insertElement",
                        NewSlot(&children_, &Elements::InsertElementFromXML));
    obj->RegisterMethod("removeElement",
                        NewSlot(&children_, &Elements::RemoveElement));
    obj->RegisterMethod("removeAllElements",
                        NewSlot(&children_, &Elements::RemoveAllElements));

    // Here register ViewImpl::BeginAnimation because the Slot1<void, int> *
    // parameter in View::BeginAnimation can't be automatically reflected.
    obj->RegisterMethod("beginAnimation", NewSlot(this, &Impl::BeginAnimation));
    obj->RegisterMethod("cancelAnimation",
                        NewSlot(this, &Impl::RemoveTimer));
    obj->RegisterMethod("setTimeout", NewSlot(this, &Impl::SetTimeout));
    obj->RegisterMethod("clearTimeout", NewSlot(this, &Impl::RemoveTimer));
    obj->RegisterMethod("setInterval", NewSlot(this, &Impl::SetInterval));
    obj->RegisterMethod("clearInterval", NewSlot(this, &Impl::RemoveTimer));

    obj->RegisterMethod("alert", NewSlot(owner_, &View::Alert));
    obj->RegisterMethod("confirm", NewSlot(owner_, &View::Confirm));
    obj->RegisterMethod("prompt", NewSlot(owner_, &View::Prompt));

    obj->RegisterMethod("resizeBy", NewSlot(this, &Impl::ResizeBy));
    obj->RegisterMethod("resizeTo", NewSlot(owner_, &View::SetSize));

    // Linux extension, not in official API. 
    obj->RegisterMethod("openURL", NewSlot(owner_, &View::OpenURL));

    obj->RegisterSignal(kOnCancelEvent, &oncancel_event_);
    obj->RegisterSignal(kOnClickEvent, &onclick_event_);
    obj->RegisterSignal(kOnCloseEvent, &onclose_event_);
    obj->RegisterSignal(kOnDblClickEvent, &ondblclick_event_);
    obj->RegisterSignal(kOnRClickEvent, &onrclick_event_);
    obj->RegisterSignal(kOnRDblClickEvent, &onrdblclick_event_);
    obj->RegisterSignal(kOnDockEvent, &ondock_event_);
    obj->RegisterSignal(kOnKeyDownEvent, &onkeydown_event_);
    obj->RegisterSignal(kOnKeyPressEvent, &onkeypress_event_);
    obj->RegisterSignal(kOnKeyUpEvent, &onkeyup_event_);
    obj->RegisterSignal(kOnMinimizeEvent, &onminimize_event_);
    obj->RegisterSignal(kOnMouseDownEvent, &onmousedown_event_);
    obj->RegisterSignal(kOnMouseOutEvent, &onmouseout_event_);
    obj->RegisterSignal(kOnMouseOverEvent, &onmouseover_event_);
    obj->RegisterSignal(kOnMouseUpEvent, &onmouseup_event_);
    obj->RegisterSignal(kOnOkEvent, &onok_event_);
    obj->RegisterSignal(kOnOpenEvent, &onopen_event_);
    obj->RegisterSignal(kOnOptionChangedEvent, &onoptionchanged_event_);
    obj->RegisterSignal(kOnPopInEvent, &onpopin_event_);
    obj->RegisterSignal(kOnPopOutEvent, &onpopout_event_);
    obj->RegisterSignal(kOnRestoreEvent, &onrestore_event_);
    obj->RegisterSignal(kOnSizeEvent, &onsize_event_);
    obj->RegisterSignal(kOnSizingEvent, &onsizing_event_);
    obj->RegisterSignal(kOnUndockEvent, &onundock_event_);
  }

  bool InitFromFile(const char *filename) {
    if (SetupViewFromFile(owner_, filename)) {
      SimpleEvent event(Event::EVENT_OPEN);
      ScriptableEvent scriptable_event(&event, owner_, NULL);
      FireEvent(&scriptable_event, onopen_event_);
      return scriptable_event.GetReturnValue() != EVENT_RESULT_CANCELED;
    } else {
      return false;
    }
  }

  void MapChildPositionEvent(const PositionEvent &org_event,
                             BasicElement *child,
                             PositionEvent *new_event) {
    ASSERT(child);
    std::vector<BasicElement *> elements;
    for (BasicElement *e = child; e != NULL; e = e->GetParentElement())
      elements.push_back(e);

    double x, y;
    BasicElement *top = *(elements.end() - 1);
    ParentCoordToChildCoord(org_event.GetX(), org_event.GetY(),
                            top->GetPixelX(), top->GetPixelY(),
                            top->GetPixelPinX(), top->GetPixelPinY(),
                            DegreesToRadians(top->GetRotation()),
                            &x, &y);

    for (std::vector<BasicElement *>::reverse_iterator it = elements.rbegin();
         // Note: Don't iterator to the last element.
         it < elements.rend() - 1; ++it) {
      // Make copies to prevent them from being overriden.
      double x1 = x, y1 = y;
      (*it)->SelfCoordToChildCoord(*(it + 1), x1, y1, &x, &y);
    }
    new_event->SetX(x);
    new_event->SetY(y);
  }

  EventResult SendMouseEventToChildren(const MouseEvent &event) {
    Event::Type type = event.GetType();
    if (type == Event::EVENT_MOUSE_OVER) {
      // View's EVENT_MOUSE_OVER only applicable to itself.
      // Children's EVENT_MOUSE_OVER is triggered by other mouse events.
      return EVENT_RESULT_UNHANDLED;
    }

    BasicElement *temp, *temp1; // Used to receive unused output parameters.
    EventResult result = EVENT_RESULT_UNHANDLED;
    // If some element is grabbing mouse, send all EVENT_MOUSE_MOVE,
    // EVENT_MOUSE_UP and EVENT_MOUSE_CLICK events to it directly, until
    // an EVENT_MOUSE_CLICK received, or any mouse event received without
    // left button down.
    if (grabmouse_element_.Get()) {
      if (grabmouse_element_.Get()->ReallyEnabled() &&
          (event.GetButton() & MouseEvent::BUTTON_LEFT) &&
          (type == Event::EVENT_MOUSE_MOVE || type == Event::EVENT_MOUSE_UP ||
           type == Event::EVENT_MOUSE_CLICK)) {
        MouseEvent new_event(event);
        MapChildPositionEvent(event, grabmouse_element_.Get(), &new_event);
        result = grabmouse_element_.Get()->OnMouseEvent(new_event, true,
                                                        &temp, &temp1);
        // Release the grabbing on EVENT_MOUSE_CLICK not EVENT_MOUSE_UP,
        // otherwise the click event may be sent to wrong element.
        if (type == Event::EVENT_MOUSE_CLICK) {
          grabmouse_element_.Reset(NULL);
        }
        return result;
      } else {
        // Release the grabbing on any mouse event without left button down.
        grabmouse_element_.Reset(NULL);
      }
    }

    if (type == Event::EVENT_MOUSE_OUT) {
      // The mouse has been moved out of the view, clear the mouseover state.
      if (mouseover_element_.Get()) {
        MouseEvent new_event(event);
        MapChildPositionEvent(event, mouseover_element_.Get(), &new_event);
        result = mouseover_element_.Get()->OnMouseEvent(new_event, true,
                                                        &temp, &temp1);
        mouseover_element_.Reset(NULL);
      }
      return result;
    }

    BasicElement *fired_element = NULL;
    BasicElement *in_element = NULL;
    ElementHolder fired_element_holder, in_element_holder;

    // Dispatch the event to children normally,
    // unless popup is active and event is inside popup element.
    bool outside_popup = true;
    if (popup_element_.Get()) {
      MouseEvent new_event(event);
      MapChildPositionEvent(event, popup_element_.Get(), &new_event);
      if (popup_element_.Get()->IsPointIn(new_event.GetX(), new_event.GetY())) {
        result = popup_element_.Get()->OnMouseEvent(new_event,
                                                    false, // NOT direct
                                                    &fired_element,
                                                    &in_element);
        outside_popup = false;
      }
    }
    if (outside_popup) {
      result = children_.OnMouseEvent(event, &fired_element, &in_element);
      // The following might hit if a grabbed element is
      // turned invisible or disabled while under grab.
      if (type == Event::EVENT_MOUSE_DOWN && result != EVENT_RESULT_CANCELED) {
        SetPopupElement(NULL);
      }
    }
    fired_element_holder.Reset(fired_element);
    in_element_holder.Reset(in_element);

    if (fired_element_holder.Get() && type == Event::EVENT_MOUSE_DOWN &&
        (event.GetButton() & MouseEvent::BUTTON_LEFT)) {
      // Start grabbing.
      grabmouse_element_.Reset(fired_element);
      SetFocus(fired_element);
    }

    if (fired_element_holder.Get() != mouseover_element_.Get()) {
      BasicElement *old_mouseover_element = mouseover_element_.Get();
      // Store it early to prevent crash if fired_element is removed in
      // the mouseout handler.
      mouseover_element_.Reset(fired_element_holder.Get());

      if (old_mouseover_element) {
        MouseEvent mouseout_event(Event::EVENT_MOUSE_OUT,
                                  event.GetX(), event.GetY(),
                                  event.GetButton(), event.GetWheelDelta(),
                                  event.GetModifier());
        MapChildPositionEvent(event, old_mouseover_element, &mouseout_event);
        old_mouseover_element->OnMouseEvent(mouseout_event, true,
                                            &temp, &temp1);
      }

      if (mouseover_element_.Get()) {
        // The enabled and visible states may change during event handling.
        if (!mouseover_element_.Get()->ReallyEnabled()) {
          mouseover_element_.Reset(NULL);
        } else {
          MouseEvent mouseover_event(Event::EVENT_MOUSE_OVER,
                                     event.GetX(), event.GetY(),
                                     event.GetButton(), event.GetWheelDelta(),
                                     event.GetModifier());
          MapChildPositionEvent(event, mouseover_element_.Get(),
                                &mouseover_event);
          mouseover_element_.Get()->OnMouseEvent(mouseover_event, true,
                                                 &temp, &temp1);
        }
      }
    }

    if (in_element_holder.Get()) {
      host_->SetCursor(in_element->GetCursor());
      if (type == Event::EVENT_MOUSE_MOVE &&
          in_element != tooltip_element_.Get()) {
        tooltip_element_.Reset(in_element);
        host_->SetTooltip(tooltip_element_.Get()->GetTooltip().c_str());
      }
    } else {
      tooltip_element_.Reset(NULL);
    }

    return result;
  }

  EventResult OnMouseEvent(const MouseEvent &event) {
    // Send event to view first.
    ScriptableEvent scriptable_event(&event, owner_, NULL);
    if (event.GetType() != Event::EVENT_MOUSE_MOVE)
      DLOG("%s(view): %g %g %d %d %d", scriptable_event.GetName(),
           event.GetX(), event.GetY(),
           event.GetButton(), event.GetWheelDelta(), event.GetModifier());
    switch (event.GetType()) {
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

    EventResult result = scriptable_event.GetReturnValue();
    if (result != EVENT_RESULT_CANCELED)
      result = SendMouseEventToChildren(event);
    return result;
  }

  EventResult OnDragEvent(const DragEvent &event) {
    Event::Type type = event.GetType();
    if (type == Event::EVENT_DRAG_OUT || type == Event::EVENT_DRAG_DROP) {
      EventResult result = EVENT_RESULT_UNHANDLED;
      // Send the event and clear the dragover state.
      if (dragover_element_.Get()) {
        // If the element rejects the drop, send a EVENT_DRAG_OUT
        // on EVENT_DRAG_DROP.
        if (dragover_result_ != EVENT_RESULT_HANDLED)
          type = Event::EVENT_DRAG_OUT;
        DragEvent new_event(type, event.GetX(), event.GetY(),
                            event.GetDragFiles());
        MapChildPositionEvent(event, dragover_element_.Get(), &new_event);
        BasicElement *temp;
        result = dragover_element_.Get()->OnDragEvent(new_event, true, &temp);
        dragover_element_.Reset(NULL);
        dragover_result_ = EVENT_RESULT_UNHANDLED;
      }
      return result;
    }

    ASSERT(type == Event::EVENT_DRAG_MOTION);
    // Dispatch the event to children normally.
    BasicElement *fired_element = NULL;
    children_.OnDragEvent(event, &fired_element);
    if (fired_element != dragover_element_.Get()) {
      dragover_result_ = EVENT_RESULT_UNHANDLED;
      BasicElement *old_dragover_element = dragover_element_.Get();
      // Store it early to prevent crash if fired_element is removed in
      // the dragout handler.
      dragover_element_.Reset(fired_element);

      if (old_dragover_element) {
        DragEvent dragout_event(Event::EVENT_DRAG_OUT,
                                event.GetX(), event.GetY(),
                                event.GetDragFiles());
        MapChildPositionEvent(event, old_dragover_element, &dragout_event);
        BasicElement *temp;
        old_dragover_element->OnDragEvent(dragout_event, true, &temp);
      }

      if (dragover_element_.Get()) {
        // The visible state may change during event handling.
        if (!dragover_element_.Get()->ReallyVisible()) {
          dragover_element_.Reset(NULL);
        } else {
          DragEvent dragover_event(Event::EVENT_DRAG_OVER,
                                   event.GetX(), event.GetY(),
                                   event.GetDragFiles());
          MapChildPositionEvent(event, dragover_element_.Get(),
                                &dragover_event);
          BasicElement *temp;
          dragover_result_ = dragover_element_.Get()->OnDragEvent(
              dragover_event, true, &temp);
        }
      }
    }

    // Because gadget elements has no handler for EVENT_DRAG_MOTION, the
    // last return result of EVENT_DRAG_OVER should be used as the return result
    // of EVENT_DRAG_MOTION.
    return dragover_result_;
  }

  EventResult OnKeyEvent(const KeyboardEvent &event) {
    ScriptableEvent scriptable_event(&event, owner_, NULL);
    DLOG("%s(view): %d %d", scriptable_event.GetName(),
         event.GetKeyCode(), event.GetModifier());
    switch (event.GetType()) {
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

    EventResult result = scriptable_event.GetReturnValue();
    if (result == EVENT_RESULT_CANCELED)
      return result;

    if (focused_element_.Get()) {
      if (!focused_element_.Get()->ReallyEnabled()) {
        focused_element_.Get()->OnOtherEvent(
            SimpleEvent(Event::EVENT_FOCUS_OUT));
        focused_element_.Reset(NULL);
      } else {
        result = focused_element_.Get()->OnKeyEvent(event);
      }
    }
    return result;
  }

  EventResult OnOtherEvent(const Event &event, Event *output_event) {
    ScriptableEvent scriptable_event(&event, owner_, output_event);
    DLOG("%s(view)", scriptable_event.GetName());
    switch (event.GetType()) {
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
      case Event::EVENT_SIZING:
        FireEvent(&scriptable_event, onsizing_event_);
        break;
      default:
        ASSERT(false);
    }
    return scriptable_event.GetReturnValue();
  }

  bool OnElementAdd(BasicElement *element) {
    ASSERT(element);
    if (element->IsInstanceOf(ContentAreaElement::CLASS_ID)) {
      if (content_area_element_.Get()) {
        LOG("Only one contentarea element is allowed in a view");
        return false;
      }
      content_area_element_.Reset(down_cast<ContentAreaElement *>(element));
    }

    std::string name = element->GetName();
    if (!name.empty() &&
        // Don't overwrite the existing element with the same name.
        all_elements_.find(name) == all_elements_.end())
      all_elements_[name] = element;
    return true;
  }

  // All references to this element should be cleared here.
  void OnElementRemove(BasicElement *element) {
    ASSERT(element);
    if (element == tooltip_element_.Get())
      host_->SetTooltip(NULL);

    std::string name = element->GetName();
    if (!name.empty()) {
      ElementsMap::iterator it = all_elements_.find(name);
      if (it != all_elements_.end() && it->second == element)
        all_elements_.erase(it);
    }
  }

  void FireEventSlot(ScriptableEvent *event, const Slot *slot) {
    ASSERT(event && slot);
    event->SetReturnValue(EVENT_RESULT_HANDLED);
    event_stack_.push_back(event);
    slot->Call(0, NULL);
    event_stack_.pop_back();
  }

  void FireEvent(ScriptableEvent *event, const EventSignal &event_signal) {
    if (event_signal.HasActiveConnections()) {
      SignalSlot slot(&event_signal);
      FireEventSlot(event, &slot);
    }
  }

  bool FirePostedEvents() {
    post_event_token_ = 0;

    // Make a copy of posted_events_, because it may change during the
    // following loop.
    PostedEvents posted_events_copy;
    std::swap(posted_events_, posted_events_copy);
    for (PostedEvents::iterator it = posted_events_copy.begin();
         it != posted_events_copy.end(); ++it) {
      // Test if the event is still valid. If srcElement has been deleted,
      // it->first->GetSrcElement() should return NULL.
      if (it->first->GetSrcElement()) {
        FireEvent(it->first, *it->second);
        delete it->first->GetEvent();
        delete it->first;
      }
    }
    posted_events_copy.clear();
    return false;
  }

  void PostEvent(ScriptableEvent *event, const EventSignal &event_signal) {
    ASSERT(event);
    ASSERT(event->GetEvent());
    // Merge multiple size events into one for each element.
    if (event->GetEvent()->GetType() == Event::EVENT_SIZE) {
      for (PostedEvents::const_iterator it = posted_events_.begin();
           it != posted_events_.end(); ++it) {
        if (it->first->GetEvent()->GetType() == Event::EVENT_SIZE &&
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
      TimerWatchCallback *watch =
          new TimerWatchCallback(this, NewSlot(this, &Impl::FirePostedEvents),
                                 0, 0, 0, 0, false);
      post_event_token_ = main_loop_->AddTimeoutWatch(0, watch);
      watch->SetWatchId(post_event_token_);
    }
  }

  ScriptableEvent *GetEvent() const {
    return event_stack_.empty() ? NULL : *(event_stack_.end() - 1);
  }

  void SetFocus(BasicElement *element) {
    if (element != focused_element_.Get() &&
        (!element || element->ReallyEnabled())) {
      ElementHolder element_holder(element);
      // Remove the current focus first.
      if (!focused_element_.Get() ||
          focused_element_.Get()->OnOtherEvent(SimpleEvent(
              Event::EVENT_FOCUS_OUT)) != EVENT_RESULT_CANCELED) {
        ElementHolder old_focused_element(focused_element_.Get());
        focused_element_.Reset(element_holder.Get());
        if (focused_element_.Get()) {
          // The enabled or visible state may changed, so check again.
          if (!focused_element_.Get()->ReallyEnabled() ||
              focused_element_.Get()->OnOtherEvent(SimpleEvent(
                  Event::EVENT_FOCUS_IN)) == EVENT_RESULT_CANCELED) {
            // If the EVENT_FOCUS_IN is canceled, set focus back to the old
            // focused element.
            focused_element_.Reset(old_focused_element.Get());
            if (focused_element_.Get() &&
                focused_element_.Get()->OnOtherEvent(SimpleEvent(
                    Event::EVENT_FOCUS_IN)) == EVENT_RESULT_CANCELED) {
              focused_element_.Reset(NULL);
            }
          }
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
      width_ = width;
      height_ = height;
      host_->QueueDraw();

      SimpleEvent event(Event::EVENT_SIZE);
      ScriptableEvent scriptable_event(&event, owner_, NULL);
      FireEvent(&scriptable_event, onsize_event_);
    }
    return true;
  }

  void MarkRedraw() {
    children_.MarkRedraw();
  }

  bool ResizeBy(int width, int height) {
    return SetSize(width_ + width, height_ + height);
  }

  void Draw(CanvasInterface *canvas) {
    // Any QueueDraw() called during Layout() will be ignored, because
    // draw_queued_ is true.
    draw_queued_ = true;
    children_.Layout();
    draw_queued_ = false;

    canvas->PushState();
    children_.Draw(canvas);

    if (popup_element_.Get()) {
      popup_element_.Get()->ClearPositionChanged();
      std::vector<BasicElement *> elements;
      BasicElement *e = popup_element_.Get();
      for (; e != NULL; e = e->GetParentElement())
        elements.push_back(e);

      std::vector<BasicElement *>::reverse_iterator it = elements.rbegin();
      for (; it < elements.rend(); ++it) {
        BasicElement *element = *it;
        if (element->GetRotation() == .0) {
          canvas->TranslateCoordinates(
              element->GetPixelX() - element->GetPixelPinX(),
              element->GetPixelY() - element->GetPixelPinY());
        } else {
          canvas->TranslateCoordinates(element->GetPixelX(),
                                       element->GetPixelY());
          canvas->RotateCoordinates(
              DegreesToRadians(element->GetRotation()));
          canvas->TranslateCoordinates(-element->GetPixelPinX(),
                                       -element->GetPixelPinY());
        }
      }

      popup_element_.Get()->Draw(canvas);
    }
    canvas->PopState();
  }

  BasicElement *GetElementByName(const char *name) {
    ElementsMap::iterator it = all_elements_.find(name);
    return it == all_elements_.end() ? NULL : it->second;
  }

  // For script.
  Variant GetElementByNameVariant(const char *name) {
    BasicElement *result = GetElementByName(name);
    return result ? Variant(result) : Variant();
  }

  void RemoveTimer(int token) {
    main_loop_->RemoveWatch(token);
  }

  int BeginAnimation(Slot *slot, int start_value, int end_value,
                     unsigned int duration) {
    uint64_t current_time = main_loop_->GetCurrentTime();
    TimerWatchCallback *watch =
        new TimerWatchCallback(this, slot, start_value, end_value,
                               duration, current_time, true);
    int id = main_loop_->AddTimeoutWatch(kAnimationInterval, watch);
    watch->SetWatchId(id);
    return id;
  }

  int SetTimeout(Slot *slot, unsigned int duration) {
    TimerWatchCallback *watch =
        new TimerWatchCallback(this, slot, 0, 0, 0, 0, true);
    int id = main_loop_->AddTimeoutWatch(duration, watch);
    watch->SetWatchId(id);
    return id;
  }

  int SetInterval(Slot *slot, unsigned int duration) {
    TimerWatchCallback *watch =
        new TimerWatchCallback(this, slot, 0, 0, -1, 0, true);
    int id = main_loop_->AddTimeoutWatch(duration, watch);
    watch->SetWatchId(id);
    return id;
  }

  void SetPopupElement(BasicElement *element) {
    if (popup_element_.Get()) {
      popup_element_.Get()->OnPopupOff();
    }
    popup_element_.Reset(element);
    if (element) {
      element->QueueDraw();
    }
  }

  void OnOptionChanged(const char *name) {
    OptionChangedEvent event(name);
    ScriptableEvent scriptable_event(&event, owner_, NULL);
    FireEvent(&scriptable_event, onoptionchanged_event_);
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
  // Note: though other things are case-insenstive, this map is case-sensitive,
  // to keep compatible with the Windows version.
  typedef std::map<std::string, BasicElement *> ElementsMap;
  ElementsMap all_elements_;

  View *owner_;
  ViewHostInterface *host_;
  GadgetHostInterface *gadget_host_;
  ScriptContextInterface *script_context_;
  FileManagerInterface *file_manager_;
  ElementFactory *element_factory_;
  MainLoopInterface *main_loop_;

  Elements children_;
  int debug_mode_;
  int width_, height_;
  ResizableMode resizable_;
  std::string caption_;
  bool show_caption_always_;

  std::vector<ScriptableEvent *> event_stack_;

  static const unsigned int kAnimationInterval = 20;
  static const unsigned int kMinInterval = 5;

  ElementHolder focused_element_;
  ElementHolder mouseover_element_;
  ElementHolder grabmouse_element_;
  ElementHolder dragover_element_;
  EventResult dragover_result_;
  ElementHolder tooltip_element_;
  ScriptableHolder<ContentAreaElement> content_area_element_;

  // Local pointers to elements should be pushed into this vector before any
  // event handler be called, and the pointer will be set to NULL if the
  // element has been removed during the event handler.
  std::vector<BasicElement **> death_detected_elements_;

  typedef std::vector<std::pair<ScriptableEvent *, const EventSignal *> >
      PostedEvents;
  PostedEvents posted_events_;
  ElementHolder popup_element_;
  int post_event_token_;
  int mark_redraw_token_;
  bool draw_queued_;
  Signal0<void> on_destroy_signal_;

  Utils utils_;
  GlobalObject global_object_;
};

View::View(ScriptableInterface *inherits_from,
           ElementFactory *element_factory,
           int debug_mode)
    : impl_(new Impl(element_factory, debug_mode, this)) {
  impl_->RegisterProperties(this);
  impl_->RegisterProperties(&impl_->global_object_);
  impl_->global_object_.RegisterConstant("view", this);
  impl_->global_object_.RegisterConstant("utils", &impl_->utils_);
  impl_->global_object_.SetDynamicPropertyHandler(
      NewSlot(impl_, &Impl::GetElementByNameVariant), NULL);
  if (inherits_from)
    impl_->global_object_.SetInheritsFrom(inherits_from);
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

void View::AttachHost(ViewHostInterface *host) {
  impl_->host_ = host;
  impl_->gadget_host_ = host->GetGadgetHost();
  impl_->file_manager_ = impl_->gadget_host_->GetFileManager();
  impl_->script_context_ = host->GetScriptContext();

  // Register script context.
  if (impl_->script_context_) {
    impl_->script_context_->SetGlobalObject(&impl_->global_object_);

    // Register global classes into script context.
    impl_->script_context_->RegisterClass("DOMDocument",
        NewSlot(impl_->gadget_host_->GetXMLParser(),
                &XMLParserInterface::CreateDOMDocument));
    impl_->script_context_->RegisterClass(
        "XMLHttpRequest", NewSlot(host, &ViewHostInterface::NewXMLHttpRequest));
    impl_->script_context_->RegisterClass(
        "DetailsView", NewSlot(DetailsView::CreateInstance));
    impl_->script_context_->RegisterClass(
        "ContentItem", NewFunctorSlot<ContentItem *>(
             ContentItem::ContentItemCreator(this)));

    // Execute common.js to initialize global constants and compatibility
    // adapters.
    std::string common_js_contents;
    std::string common_js_path;
    if (impl_->file_manager_->GetFileContents(kCommonJS, &common_js_contents,
                                              &common_js_path)) {
      impl_->script_context_->Execute(common_js_contents.c_str(),
                                      common_js_path.c_str(), 1);
    } else {
      LOG("Failed to load %s.", kCommonJS);
    }
  }
}

int View::GetWidth() const {
  return impl_->width_;
}

int View::GetHeight() const {
  return impl_->height_;
}

void View::Draw(CanvasInterface *canvas) {
  impl_->Draw(canvas);
}

void *View::GetNativeWidget() {
  return impl_->host_->GetNativeWidget();
}

void View::ViewCoordToNativeWidgetCoord(double x, double y,
                                        double *widget_x, double *widget_y) {
  impl_->host_->ViewCoordToNativeWidgetCoord(x, y, widget_x, widget_y);
}

void View::QueueDraw() {
  if (!impl_->draw_queued_) {
    impl_->draw_queued_ = true;
    impl_->host_->QueueDraw();
  }
}

const GraphicsInterface *View::GetGraphics() const {
  return impl_->host_->GetGraphics();
}

EventResult View::OnMouseEvent(const MouseEvent &event) {
  return impl_->OnMouseEvent(event);
}

EventResult View::OnKeyEvent(const KeyboardEvent &event) {
  return impl_->OnKeyEvent(event);
}

EventResult View::OnDragEvent(const DragEvent &event) {
  return impl_->OnDragEvent(event);
}

EventResult View::OnOtherEvent(const Event &event, Event *output_event) {
  return impl_->OnOtherEvent(event, output_event);
}

bool View::OnElementAdd(BasicElement *element) {
  return impl_->OnElementAdd(element);
}

void View::OnElementRemove(BasicElement *element) {
  impl_->OnElementRemove(element);
}

void View::SetPopupElement(BasicElement *element) {
  impl_->SetPopupElement(element);
}

BasicElement *View::GetPopupElement() {
  return impl_->popup_element_.Get();
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

ViewInterface::ResizableMode View::GetResizable() const {
  return impl_->resizable_;
}

void View::SetResizable(ViewInterface::ResizableMode resizable) {
  impl_->resizable_ = resizable;
  impl_->host_->SetResizable(resizable);
}

ElementFactory *View::GetElementFactory() const {
  return impl_->element_factory_;
}

const Elements *View::GetChildren() const {
  return &impl_->children_;
}

Elements *View::GetChildren() {
  return &impl_->children_;
}

BasicElement *View::GetElementByName(const char *name) {
  return impl_->GetElementByName(name);
}

const BasicElement *View::GetElementByName(const char *name) const {
  return impl_->GetElementByName(name);
}

void View::SetCaption(const char *caption) {
  impl_->caption_ = caption ? caption : NULL;
  impl_->host_->SetCaption(caption);
}

std::string View::GetCaption() const {
  return impl_->caption_;
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
  impl_->RemoveTimer(token);
}

int View::SetTimeout(Slot0<void> *slot, unsigned int duration) {
  return impl_->SetTimeout(slot, duration);
}

void View::ClearTimeout(int token) {
  impl_->RemoveTimer(token);
}

int View::SetInterval(Slot0<void> *slot, unsigned int duration) {
  return impl_->SetInterval(slot, duration);
}

void View::ClearInterval(int token) {
  impl_->RemoveTimer(token);
}

int View::GetDebugMode() const {
  return impl_->debug_mode_;
}

ImageInterface *View::LoadImage(const Variant &src, bool is_mask) {
  ASSERT(impl_->file_manager_);
  Variant::Type type = src.type();
  if (type == Variant::TYPE_STRING) {
    const char *filename = VariantValue<const char*>()(src);
    if (filename && *filename) {
      std::string real_path;
      std::string data;
      if (impl_->file_manager_->GetFileContents(filename, &data, &real_path)) {
        ImageInterface *image = GetGraphics()->NewImage(data, is_mask);
        if (image)
          image->SetTag(filename);
        return image;
      }
    }
  } else if (type == Variant::TYPE_SCRIPTABLE) {
    const ScriptableBinaryData *binary =
        VariantValue<const ScriptableBinaryData *>()(src);
    if (binary)
      return GetGraphics()->NewImage(binary->data(), is_mask);
  } else {
    LOG("Unsupported type of image src.");
    DLOG("src=%s", src.Print().c_str());
  }
  return NULL;
}

ImageInterface *View::LoadImageFromGlobal(const char *name, bool is_mask) {
  return LoadImage(Variant(name), is_mask);
}

Texture *View::LoadTexture(const Variant &src) {
  ASSERT(impl_->file_manager_);
  Color color;
  double opacity;
  if (src.type() == Variant::TYPE_STRING &&
      Color::FromString(VariantValue<const char*>()(src), &color, &opacity)) {
    return new Texture(color, opacity);
  }

  return new Texture(LoadImage(src, false));
}

void View::SetFocus(BasicElement *element) {
  impl_->SetFocus(element);
}

bool View::OpenURL(const char *url) const {
  // Important: verify that URL is valid first.
  // Otherwise could be a security problem.
  std::string newurl = EncodeURL(url);
  if (0 == strncasecmp(newurl.c_str(), kHttpUrlPrefix,
                       arraysize(kHttpUrlPrefix) - 1) ||
      0 == strncasecmp(newurl.c_str(), kHttpsUrlPrefix,
                       arraysize(kHttpsUrlPrefix) - 1) ||
      0 == strncasecmp(newurl.c_str(), kFtpUrlPrefix,
                       arraysize(kFtpUrlPrefix) - 1)) {
    return impl_->gadget_host_->OpenURL(newurl.c_str());
  }

  DLOG("Malformed URL: %s", newurl.c_str());
  return false;
}

void View::OnOptionChanged(const char *name) {
  impl_->OnOptionChanged(name);
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

uint64_t View::GetCurrentTime() {
  return impl_->main_loop_->GetCurrentTime();
}

ContentAreaElement *View::GetContentAreaElement() {
  return impl_->content_area_element_.Get();
}

void View::SetTooltip(const char *tooltip) {
  impl_->host_->SetTooltip(tooltip);
}

bool View::ShowDetailsView(DetailsView *details_view,
                           const char *title, int flags,
                           Slot1<void, int> *feedback_handler) {
  return impl_->gadget_host_->GetGadget()->ShowDetailsView(details_view, title,
                                                           flags,
                                                           feedback_handler);
}

bool View::OnAddContextMenuItems(MenuInterface *menu) {
  if (impl_->mouseover_element_.Get()) {
    if (impl_->mouseover_element_.Get()->ReallyEnabled())
      return impl_->mouseover_element_.Get()->OnAddContextMenuItems(menu);
    else
      impl_->mouseover_element_.Reset(NULL);
  }
  return true;
}

void View::MarkRedraw() {
  impl_->MarkRedraw();
}

XMLParserInterface *View::GetXMLParser() const {
  return impl_->gadget_host_->GetXMLParser();
}

Connection *View::ConnectOnCancelEvent(Slot0<void> *handler) {
  return impl_->oncancel_event_.Connect(handler);
}
Connection *View::ConnectOnClickEvent(Slot0<void> *handler) {
  return impl_->onclick_event_.Connect(handler);
}
Connection *View::ConnectOnCloseEvent(Slot0<void> *handler) {
  return impl_->onclose_event_.Connect(handler);
}
Connection *View::ConnectOnDblClickEvent(Slot0<void> *handler) {
  return impl_->ondblclick_event_.Connect(handler);
}
Connection *View::ConnectOnRClickEvent(Slot0<void> *handler) {
  return impl_->onrclick_event_.Connect(handler);
}
Connection *View::ConnectOnRDblClickCancelEvent(Slot0<void> *handler) {
  return impl_->onrdblclick_event_.Connect(handler);
}
Connection *View::ConnectOnDockEvent(Slot0<void> *handler) {
  return impl_->ondock_event_.Connect(handler);
}
Connection *View::ConnectOnKeyDownEvent(Slot0<void> *handler) {
  return impl_->onkeydown_event_.Connect(handler);
}
Connection *View::ConnectOnPressEvent(Slot0<void> *handler) {
  return impl_->onkeypress_event_.Connect(handler);
}
Connection *View::ConnectOnKeyUpEvent(Slot0<void> *handler) {
  return impl_->onkeyup_event_.Connect(handler);
}
Connection *View::ConnectOnMinimizeEvent(Slot0<void> *handler) {
  return impl_->onminimize_event_.Connect(handler);
}
Connection *View::ConnectOnMouseDownEvent(Slot0<void> *handler) {
  return impl_->onmousedown_event_.Connect(handler);
}
Connection *View::ConnectOnMouseOverEvent(Slot0<void> *handler) {
  return impl_->onmouseover_event_.Connect(handler);
}
Connection *View::ConnectOnMouseOutEvent(Slot0<void> *handler) {
  return impl_->onmouseout_event_.Connect(handler);
}
Connection *View::ConnectOnMouseUpEvent(Slot0<void> *handler) {
  return impl_->onmouseup_event_.Connect(handler);
}
Connection *View::ConnectOnOkEvent(Slot0<void> *handler) {
  return impl_->onok_event_.Connect(handler);
}
Connection *View::ConnectOnOpenEvent(Slot0<void> *handler) {
  return impl_->onopen_event_.Connect(handler);
}
Connection *View::ConnectOnOptionChangedEvent(Slot0<void> *handler) {
  return impl_->onoptionchanged_event_.Connect(handler);
}
Connection *View::ConnectOnPopInEvent(Slot0<void> *handler) {
  return impl_->onpopin_event_.Connect(handler);
}
Connection *View::ConnectOnPopOutEvent(Slot0<void> *handler) {
  return impl_->onpopout_event_.Connect(handler);
}
Connection *View::ConnectOnRestoreEvent(Slot0<void> *handler) {
  return impl_->onrestore_event_.Connect(handler);
}
Connection *View::ConnectOnSizeEvent(Slot0<void> *handler) {
  return impl_->onsize_event_.Connect(handler);
}
Connection *View::ConnectOnSizingEvent(Slot0<void> *handler) {
  return impl_->onsizing_event_.Connect(handler);
}
Connection *View::ConnectOnUndockEvent(Slot0<void> *handler) {
  return impl_->onundock_event_.Connect(handler);
}

} // namespace ggadget
