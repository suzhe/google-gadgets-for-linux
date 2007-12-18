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
#include "element_factory_interface.h"
#include "elements.h"
#include "event.h"
#include "file_manager_interface.h"
#include "gadget_host_interface.h"
#include "gadget_interface.h"
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

static const char *kResizableNames[] = { "false", "true", "zoom" };

class View::Impl {
 public:
  class GlobalObject : public ScriptableHelper<ScriptableInterface> {
   public:
    DEFINE_CLASS_ID(0x23840d38ed164ab2, ScriptableInterface);
    virtual bool IsStrict() const { return false; }
  };

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
      resizable_(ViewHostInterface::RESIZABLE_TRUE),
      show_caption_always_(false),
      focused_element_(NULL),
      mouseover_element_(NULL),
      grabmouse_element_(NULL),
      dragover_element_(NULL),
      dragover_result_(EVENT_RESULT_UNHANDLED),
      tooltip_element_(NULL),
      content_area_element_(NULL),
      posting_event_element_(NULL),
      post_event_token_(0),
      draw_queued_(false) {
    if (gadget_host_)
      file_manager_ = gadget_host_->GetFileManager();
  }

  ~Impl() {
    ASSERT(event_stack_.empty());
    ASSERT(death_detected_elements_.empty());

    TimerMap::iterator it = timer_map_.begin();
    while (it != timer_map_.end()) {
      TimerMap::iterator next = it;
      ++next;
      RemoveTimer(it->first);
      it = next;
    }
  }

  template <typename T>
  void RegisterProperties(T *obj) {
    obj->RegisterProperty("caption", NewSlot(owner_, &View::GetCaption),
                          NewSlot(owner_, &View::SetCaption));
    obj->RegisterConstant("children", &children_);
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
                        NewSlot(this, &Impl::CancelAnimation));
    obj->RegisterMethod("setTimeout", NewSlot(this, &Impl::SetTimeout));
    obj->RegisterMethod("clearTimeout", NewSlot(this, &Impl::ClearTimeout));
    obj->RegisterMethod("setInterval", NewSlot(this, &Impl::SetInterval));
    obj->RegisterMethod("clearInterval", NewSlot(this, &Impl::ClearInterval));

    obj->RegisterMethod("alert", NewSlot(owner_, &View::Alert));
    obj->RegisterMethod("confirm", NewSlot(owner_, &View::Confirm));
    obj->RegisterMethod("prompt", NewSlot(owner_, &View::Prompt));

    obj->RegisterMethod("resizeBy", NewSlot(this, &Impl::ResizeBy));
    obj->RegisterMethod("resizeTo", NewSlot(owner_, &View::SetSize));

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
      onopen_event_();
      return true;
    } else {
      return false;
    }
  }

  void MapChildPositionEvent(const PositionEvent &org_event,
                             BasicElement *child,
                             PositionEvent *new_event) {
    ASSERT(child);
    std::vector<BasicElement *> elements;
    for (ElementInterface *e = child; e != NULL; e = e->GetParentElement())
      elements.push_back(down_cast<BasicElement *>(e));

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

    BasicElement *fired_element = NULL;
    BasicElement *in_element = NULL;
    ScopedDeathDetector death_detector(owner_, &fired_element);
    ScopedDeathDetector death_detector1(owner_, &in_element);

    EventResult result = EVENT_RESULT_UNHANDLED;
    // If some element is grabbing mouse, send all EVENT_MOUSE_MOVE,
    // EVENT_MOUSE_UP and EVENT_MOUSE_CLICK events to it directly, until
    // an EVENT_MOUSE_CLICK received, or any mouse event received without
    // left button down.
    if (grabmouse_element_) {
      if (grabmouse_element_->IsEnabled() &&
          grabmouse_element_->IsVisible() &&
          (event.GetButton() & MouseEvent::BUTTON_LEFT) &&
          (type == Event::EVENT_MOUSE_MOVE || type == Event::EVENT_MOUSE_UP ||
           type == Event::EVENT_MOUSE_CLICK)) {
        MouseEvent new_event(event);
        MapChildPositionEvent(event, grabmouse_element_, &new_event);
        result = grabmouse_element_->OnMouseEvent(new_event, true,
                                                  &fired_element, &in_element);
        // Release the grabbing on EVENT_MOUSE_CLICK not EVENT_MOUSE_UP,
        // otherwise the click event may be sent to wrong element.
        if (type == Event::EVENT_MOUSE_CLICK)
          grabmouse_element_ = NULL;
        return result;
      } else {
        // Release the grabbing on any mouse event without left button down.
        grabmouse_element_ = NULL;
      }
    }

    if (type == Event::EVENT_MOUSE_OUT) {
      // The mouse has been moved out of the view, clear the mouseover state.
      if (mouseover_element_) {
        MouseEvent new_event(event);
        MapChildPositionEvent(event, mouseover_element_, &new_event);
        result = mouseover_element_->OnMouseEvent(new_event, true,
                                                  &fired_element, &in_element);
        mouseover_element_ = NULL;
      }
      return result;
    }

    // Dispatch the event to children normally.
    result = children_.OnMouseEvent(event, &fired_element, &in_element);

    if (fired_element && type == Event::EVENT_MOUSE_DOWN &&
        (event.GetButton() & MouseEvent::BUTTON_LEFT)) {
      // Start grabbing.
      grabmouse_element_ = fired_element;
      SetFocus(fired_element);
      // In the focusin handler, the element may be removed and fired_element
      // points to invalid element.  However, grabmouse_element_ will still
      // be valid or has been set to NULL.
      fired_element = grabmouse_element_;
    }

    if (fired_element != mouseover_element_) {
      BasicElement *old_mouseover_element = mouseover_element_;
      // Store it early to prevent crash if fired_element is removed in
      // the mouseout handler.
      mouseover_element_ = fired_element;

      if (old_mouseover_element) {
        MouseEvent mouseout_event(Event::EVENT_MOUSE_OUT,
                                  event.GetX(), event.GetY(),
                                  event.GetButton(), event.GetWheelDelta(),
                                  event.GetModifier());
        MapChildPositionEvent(event, old_mouseover_element, &mouseout_event);
        old_mouseover_element->OnMouseEvent(mouseout_event, true,
                                            &fired_element, &in_element);
      }

      if (mouseover_element_) {
        // The enabled and visible states may change during event handling.
        if (!mouseover_element_->IsEnabled() ||
            !mouseover_element_->IsVisible()) {
          mouseover_element_ = NULL;
        } else {
          MouseEvent mouseover_event(Event::EVENT_MOUSE_OVER,
                                     event.GetX(), event.GetY(),
                                     event.GetButton(), event.GetWheelDelta(),
                                     event.GetModifier());
          MapChildPositionEvent(event, mouseover_element_, &mouseover_event);
          mouseover_element_->OnMouseEvent(mouseover_event, true,
                                           &fired_element, &in_element);
        }
      }
    }

    if (in_element) {
      host_->SetCursor(in_element->GetCursor());
      if (type == Event::EVENT_MOUSE_MOVE && in_element != tooltip_element_) {
        tooltip_element_ = in_element;
        host_->SetTooltip(tooltip_element_->GetTooltip().c_str());
      }
    } else {
      tooltip_element_ = NULL;
    }

    return result;
  }

  EventResult OnMouseEvent(const MouseEvent &event) {
    // Send event to view first.
    ScriptableEvent scriptable_event(&event, NULL, NULL);
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
      if (dragover_element_) {
        // If the element rejects the drop, send a EVENT_DRAG_OUT
        // on EVENT_DRAG_DROP.
        if (dragover_result_ != EVENT_RESULT_HANDLED)
          type = Event::EVENT_DRAG_OUT;
        DragEvent new_event(type, event.GetX(), event.GetY(),
                            event.GetDragFiles());
        MapChildPositionEvent(event, dragover_element_, &new_event);
        BasicElement *temp;
        result = dragover_element_->OnDragEvent(new_event, true, &temp);
        dragover_element_ = NULL;
        dragover_result_ = EVENT_RESULT_UNHANDLED;
      }
      return result;
    }

    ASSERT(type == Event::EVENT_DRAG_MOTION);
    // Dispatch the event to children normally.
    BasicElement *fired_element = NULL;
    children_.OnDragEvent(event, &fired_element);
    if (fired_element != dragover_element_) {
      dragover_result_ = EVENT_RESULT_UNHANDLED;
      BasicElement *old_dragover_element = dragover_element_;
      // Store it early to prevent crash if fired_element is removed in
      // the dragout handler.
      dragover_element_ = fired_element;

      if (old_dragover_element) {
        DragEvent dragout_event(Event::EVENT_DRAG_OUT,
                                event.GetX(), event.GetY(),
                                event.GetDragFiles());
        MapChildPositionEvent(event, old_dragover_element, &dragout_event);
        BasicElement *temp;
        old_dragover_element->OnDragEvent(dragout_event, true, &temp);
      }

      if (dragover_element_) {
        // The visible state may change during event handling.
        if (!dragover_element_->IsVisible()) {
          dragover_element_ = NULL;
        } else {
          DragEvent dragover_event(Event::EVENT_DRAG_OVER,
                                   event.GetX(), event.GetY(),
                                   event.GetDragFiles());
          MapChildPositionEvent(event, dragover_element_, &dragover_event);
          BasicElement *temp;
          dragover_result_ = dragover_element_->OnDragEvent(dragover_event,
                                                            true, &temp);
        }
      }
    }

    // Because gadget elements has no handler for EVENT_DRAG_MOTION, the
    // last return result of EVENT_DRAG_OVER should be used as the return result
    // of EVENT_DRAG_MOTION.
    return dragover_result_;
  }

  EventResult OnKeyEvent(const KeyboardEvent &event) {
    ScriptableEvent scriptable_event(&event, NULL, NULL);
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

    if (focused_element_) {
      if (!focused_element_->IsEnabled() || !focused_element_->IsVisible())
        focused_element_ = NULL;
      else
        result = focused_element_->OnKeyEvent(event);
    }
    return result;
  }

  EventResult OnOtherEvent(const Event &event, Event *output_event) {
    ScriptableEvent scriptable_event(&event, NULL, output_event);
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
      if (content_area_element_) {
        LOG("Only one contentarea element is allowed in a view");
        return false;
      }
      content_area_element_ = down_cast<ContentAreaElement *>(element);
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
    if (element == focused_element_)
      // Don't send EVENT_FOCUS_OUT because the element is being removed.
      focused_element_ = NULL;
    if (element == mouseover_element_)
      mouseover_element_ = NULL;
    if (element == grabmouse_element_)
      grabmouse_element_ = NULL;
    if (element == dragover_element_)
      dragover_element_ = NULL;
    if (element == tooltip_element_) {
      host_->SetTooltip(NULL);
      tooltip_element_ = NULL;
    }
    if (element == content_area_element_)
      content_area_element_ = NULL;

    for (std::vector<BasicElement **>::iterator it =
             death_detected_elements_.begin();
         it != death_detected_elements_.end(); ++it) {
      if (**it == element)
        *it = NULL;
    }

    for (std::vector<ScriptableEvent *>::iterator it = event_stack_.begin();
         it != event_stack_.end(); ++it) {
      if ((*it)->GetSrcElement() == element)
        (*it)->SetSrcElement(NULL);
    }

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

    std::string name = element->GetName();
    if (!name.empty()) {
      ElementsMap::iterator it = all_elements_.find(name);
      if (it != all_elements_.end() && it->second == element)
        all_elements_.erase(it);
    }
  }

  void FireEvent(ScriptableEvent *event, const EventSignal &event_signal) {
    ASSERT(event);
    // Note: TimerCallback() also does the similar thing.
    if (event_signal.HasActiveConnections()) {
      event->SetReturnValue(EVENT_RESULT_HANDLED);
      event_stack_.push_back(event);
      event_signal();
      event_stack_.pop_back();
    }
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
    return event_stack_.empty() ? NULL : *(event_stack_.end() - 1);
  }

  void SetFocus(BasicElement *element) {
    if (element != focused_element_ &&
        (!element || (element->IsEnabled() && element->IsVisible()))) {
      ScopedDeathDetector death_detector(owner_, &element);
      // Remove the current focus first.
      if (!focused_element_ ||
          focused_element_->OnOtherEvent(SimpleEvent(Event::EVENT_FOCUS_OUT)) !=
              EVENT_RESULT_CANCELED) {
        BasicElement *old_focused_element = focused_element_;
        ScopedDeathDetector death_detector(owner_, &old_focused_element);
        focused_element_ = element;
        if (focused_element_) {
          // The enabled or visible state may changed, so check again. 
          if (!focused_element_->IsEnabled() ||
              !focused_element_->IsVisible() ||
              focused_element_->OnOtherEvent(SimpleEvent(Event::EVENT_FOCUS_IN))
                  == EVENT_RESULT_CANCELED) {
            // If the EVENT_FOCUS_IN is canceled, set focus back to the old
            // focused element.
            focused_element_ = old_focused_element;
            if (focused_element_ &&
                focused_element_->OnOtherEvent(SimpleEvent(
                    Event::EVENT_FOCUS_IN)) == EVENT_RESULT_CANCELED) {
              focused_element_ = NULL;
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
      // TODO check if allowed first
      width_ = width;
      height_ = height;
      host_->QueueDraw();

      SimpleEvent event(Event::EVENT_SIZE);
      ScriptableEvent scriptable_event(&event, NULL, NULL);
      FireEvent(&scriptable_event, onsize_event_);
    }
    return true;
  }

  bool ResizeBy(int width, int height) {
    return SetSize(width_ + width, height_ + height);
  }

  const CanvasInterface *Draw(bool *changed) {
    // Any QueueDraw() called during Layout() will be ignored, because
    // draw_queued_ is true.
    draw_queued_ = true;
    children_.Layout();
    draw_queued_ = false;
    return children_.Draw(changed);
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
            kMinInterval) {
      // To avoid some frequent interval/animation timers or slow handlers
      // eat up all CPU resources, here automaticlly ignores some timer events.
      DLOG("Automatically ignored a timer event");
      return true;
    }

    TimerEvent event(token, 0);
    ScriptableEvent scriptable_event(&event, NULL, NULL);
    event_stack_.push_back(&scriptable_event);
    switch (info->type) {
      case TIMER_TIMEOUT:
        info->slot->Call(0, NULL);
        wants_more = false;
        RemoveTimer(token);
        break;
      case TIMER_INTERVAL:
        info->slot->Call(0, NULL);
        break;
      case TIMER_ANIMATION_FIRST:
        event.SetValue(info->start_value);
        info->slot->Call(0, NULL);
        wants_more = false;
        RemoveTimer(token);
        break;
      case TIMER_ANIMATION: {
        double progress = 1.0;
        if (info->duration > 0) {
          uint64_t event_time = gadget_host_->GetCurrentTime();
          progress = static_cast<double>(event_time - info->start_time) /
                     info->duration;
          progress = std::min(1.0, std::max(0.0, progress));
        }

        int value = info->start_value +
                    static_cast<int>(round(progress * info->spread));
        if (value != info->last_value) {
          event.SetValue(value);
          info->last_value = value;
          info->slot->Call(0, NULL);
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

    event_stack_.pop_back();
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
    ScriptableEvent scriptable_event(&event, NULL, NULL);
    FireEvent(&scriptable_event, onoptionchanged_event_);
  }

  class DeathDetectedSlot : public Slot {
   public:
    DeathDetectedSlot(Impl *impl, BasicElement *element, Slot *slot)
        : impl_(impl), element_(element), slot_(slot) {
      ASSERT(impl && element && slot);
      // Insert the detector at the beginning of the vector, because the other
      // end of the vector is used in stack manner for ScopedDeathDetector.
      impl->death_detected_elements_.insert(
          impl->death_detected_elements_.begin(), &element_);
    }
    ~DeathDetectedSlot() {
      std::vector<BasicElement **>::iterator it = std::find(
          impl_->death_detected_elements_.begin(),
          impl_->death_detected_elements_.end(), &element_);
      ASSERT(it != impl_->death_detected_elements_.end());
      impl_->death_detected_elements_.erase(it);
      delete slot_;
      slot_ = NULL;
    }
    virtual Variant Call(int argc, Variant argv[]) const {
      if (element_) {
        // If it is still live.
        return slot_->Call(argc, argv);
      }
      return Variant(slot_->GetReturnType());
    }
    virtual bool operator==(const Slot &another) const { return false; }

    Impl *impl_;
    BasicElement *element_;
    Slot *slot_;
  };

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
  ElementFactoryInterface *element_factory_;
  Elements children_;
  int debug_mode_;
  int width_, height_;
  ViewHostInterface::ResizableMode resizable_;
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
  BasicElement *focused_element_;
  BasicElement *mouseover_element_;
  BasicElement *grabmouse_element_;
  BasicElement *dragover_element_;
  EventResult dragover_result_;
  BasicElement *tooltip_element_;
  ContentAreaElement *content_area_element_;

  // Local pointers to elements should be pushed into this vector before any
  // event handler be called, and the pointer will be set to NULL if the
  // element has been removed during the event handler.
  std::vector<BasicElement **> death_detected_elements_;

  GlobalObject global_object_;

  typedef std::vector<std::pair<ScriptableEvent *, const EventSignal *> >
      PostedEvents;
  PostedEvents posted_events_;
  BasicElement *posting_event_element_;
  int post_event_token_;
  bool draw_queued_;
};

View::View(ViewHostInterface *host,
           ScriptableInterface *prototype,
           ElementFactoryInterface *element_factory,
           int debug_mode)
    : impl_(new Impl(host, element_factory, debug_mode, this)) {
  impl_->RegisterProperties(this);
  impl_->RegisterProperties(&impl_->global_object_);
  impl_->global_object_.RegisterConstant("view", this);
  impl_->global_object_.SetDynamicPropertyHandler(
      NewSlot(impl_, &Impl::GetElementByNameVariant), NULL);
  if (prototype)
    impl_->global_object_.SetPrototype(prototype);

  if (impl_->script_context_)
    impl_->script_context_->SetGlobalObject(&impl_->global_object_);
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

ViewHostInterface::ResizableMode View::GetResizable() const {
  return impl_->resizable_;
}

void View::SetResizable(ViewHostInterface::ResizableMode resizable) {
  impl_->resizable_ = resizable;
  impl_->host_->SetResizable(resizable);
}

ElementFactoryInterface *View::GetElementFactory() const {
  return impl_->element_factory_;
}

const ElementsInterface *View::GetChildren() const {
  return &impl_->children_;
}

ElementsInterface *View::GetChildren() {
  return &impl_->children_;
}

ElementInterface *View::GetElementByName(const char *name) {
  return impl_->GetElementByName(name);
}

const ElementInterface *View::GetElementByName(const char *name) const {
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

void View::PushDeathDetectedElement(BasicElement **element_ptr) {
  impl_->death_detected_elements_.push_back(element_ptr);
}

void View::PopDeathDetectedElement() {
  impl_->death_detected_elements_.pop_back();
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
        return new Image(GetGraphics(), binary->data().c_str(), binary->size(),
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
        return new Texture(GetGraphics(), binary->data().c_str(),
                           binary->size());
      // else falls through!
    }
    default:
      LOG("Unsupported type of texture src.");
      DLOG("src=%s", src.ToString().c_str());
      return NULL;
  }
}

void View::SetFocus(BasicElement *element) {
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
  return impl_->gadget_host_->GetCurrentTime();
}

ContentAreaElement *View::GetContentAreaElement() {
  return impl_->content_area_element_;
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
  if (impl_->mouseover_element_)
    return impl_->mouseover_element_->OnAddContextMenuItems(menu);
  return true;
}

Slot *View::NewDeathDetectedSlot(BasicElement *element, Slot *slot) {
  return new Impl::DeathDetectedSlot(impl_, element, slot);
}

EditInterface *View::NewEdit(size_t w, size_t h) {
  return impl_->host_->NewEdit(w, h);
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
