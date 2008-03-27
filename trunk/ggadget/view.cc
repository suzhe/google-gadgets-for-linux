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

#include <sys/time.h>
#include <time.h>
#include <list>
#include <set>
#include <vector>

#include "view.h"
#include "basic_element.h"
#include "clip_region.h"
#include "contentarea_element.h"
#include "content_item.h"
#include "details_view_data.h"
#include "element_factory.h"
#include "elements.h"
#include "event.h"
#include "file_manager_interface.h"
#include "file_manager_factory.h"
#include "main_loop_interface.h"
#include "gadget_consts.h"
#include "host_interface.h"
#include "gadget.h"
#include "graphics_interface.h"
#include "image_interface.h"
#include "logger.h"
#include "math_utils.h"
#include "options_interface.h"
#include "script_context_interface.h"
#include "scriptable_binary_data.h"
#include "scriptable_event.h"
#include "scriptable_image.h"
#include "scriptable_interface.h"
#include "scriptable_helper.h"
#include "slot.h"
#include "texture.h"
#include "view_host_interface.h"
#include "xml_dom_interface.h"
#include "xml_http_request_interface.h"
#include "xml_parser_interface.h"
#include "xml_utils.h"

namespace ggadget {

static const char *kResizableNames[] = { "false", "true", "zoom" };

class View::Impl {
 public:
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
        duration_(duration), start_time_(start_time), last_finished_time_(0),
        last_value_(start), watch_id_(0),
        is_event_(is_event), destroy_connection_(NULL) {
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
          ScriptableEvent scriptable_event(&event, NULL, NULL);
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

  Impl(View *owner,
       ViewHostInterface *host,
       Gadget *gadget,
       ElementFactory *element_factory,
       ScriptContextInterface *script_context)
    : clip_region_(0.9),
      owner_(owner),
      gadget_(gadget),
      element_factory_(element_factory),
      main_loop_(GetGlobalMainLoop()),
      host_(host),
      script_context_(script_context),
      onoptionchanged_connection_(NULL),
      canvas_cache_(NULL),
      enable_cache_(true),
      children_(element_factory, NULL, owner),
      dragover_result_(EVENT_RESULT_UNHANDLED),
      width_(0),
      height_(0),
      // TODO: Make sure the default value.
      resizable_(ViewInterface::RESIZABLE_ZOOM),
      show_caption_always_(false),
      post_event_token_(0),
      draw_queued_(false),
      events_enabled_(true),
      need_redraw_(true),
      draw_count_(0),
      hittest_(ViewInterface::HT_CLIENT) {
    ASSERT(host_);
    ASSERT(element_factory_);
    ASSERT(main_loop_);

    if (gadget_) {
      onoptionchanged_connection_ =
          gadget_->GetOptions()->ConnectOnOptionChanged(
              NewSlot(this, &Impl::OnOptionChanged));
    }
  }

  ~Impl() {
    ASSERT(event_stack_.empty());

    host_->SetView(NULL);
    host_ = NULL;

    on_destroy_signal_.Emit(0, NULL);

    if (onoptionchanged_connection_) {
      onoptionchanged_connection_->Disconnect();
      onoptionchanged_connection_ = NULL;
    }

    if (canvas_cache_) {
      canvas_cache_->Destroy();
      canvas_cache_ = NULL;
    }
  }

  void RegisterProperties(RegisterableInterface *obj) {
    obj->RegisterProperty("caption",
                          NewSlot(owner_, &View::GetCaption),
                          NewSlot(owner_, &View::SetCaption));
    // Note: "event" property will be overrided in ScriptableView,
    // because ScriptableView will set itself to ScriptableEvent as SrcElement.
    obj->RegisterProperty("event", NewSlot(this, &Impl::GetEvent), NULL);
    obj->RegisterProperty("width",
                          NewSlot(owner_, &View::GetWidth),
                          NewSlot(owner_, &View::SetWidth));
    obj->RegisterProperty("height",
                          NewSlot(owner_, &View::GetHeight),
                          NewSlot(owner_, &View::SetHeight));
    obj->RegisterStringEnumProperty("resizable",
                          NewSlot(owner_, &View::GetResizable),
                          NewSlot(owner_, &View::SetResizable),
                          kResizableNames,
                          arraysize(kResizableNames));
    obj->RegisterProperty("showCaptionAlways",
                          NewSlot(owner_, &View::GetShowCaptionAlways),
                          NewSlot(owner_, &View::SetShowCaptionAlways));

    obj->RegisterVariantConstant("children", Variant(&children_));
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
    obj->RegisterMethod("cancelAnimation", NewSlot(this, &Impl::RemoveTimer));
    obj->RegisterMethod("setTimeout", NewSlot(this, &Impl::SetTimeout));
    obj->RegisterMethod("clearTimeout", NewSlot(this, &Impl::RemoveTimer));
    obj->RegisterMethod("setInterval", NewSlot(this, &Impl::SetInterval));
    obj->RegisterMethod("clearInterval", NewSlot(this, &Impl::RemoveTimer));

    obj->RegisterMethod("alert", NewSlot(owner_, &View::Alert));
    obj->RegisterMethod("confirm", NewSlot(owner_, &View::Confirm));
    obj->RegisterMethod("prompt", NewSlot(owner_, &View::Prompt));

    obj->RegisterMethod("resizeBy", NewSlot(this, &Impl::ResizeBy));
    obj->RegisterMethod("resizeTo", NewSlot(this, &Impl::SetSize));

    // Extended APIs.
    obj->RegisterProperty("focusedElement",
                          NewSlot(owner_, &View::GetFocusedElement), NULL);
    obj->RegisterProperty("mouseOverElement",
                          NewSlot(owner_, &View::GetMouseOverElement), NULL);
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
    obj->RegisterSignal(kOnMouseMoveEvent, &onmousemove_event_);
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

  void MapChildPositionEvent(const PositionEvent &org_event,
                             BasicElement *child,
                             PositionEvent *new_event) {
    ASSERT(child);
    double x, y;
    child->ViewCoordToSelfCoord(org_event.GetX(), org_event.GetY(), &x, &y);
    new_event->SetX(x);
    new_event->SetY(y);
  }

  void MapChildMouseEvent(const MouseEvent &org_event,
                          BasicElement *child,
                          MouseEvent *new_event) {
    MapChildPositionEvent(org_event, child, new_event);
    BasicElement::FlipMode flip = child->GetFlip();
    if (flip & BasicElement::FLIP_HORIZONTAL)
      new_event->SetWheelDeltaX(-org_event.GetWheelDeltaX());
    if (flip & BasicElement::FLIP_VERTICAL)
      new_event->SetWheelDeltaY(-org_event.GetWheelDeltaY());
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
      if (grabmouse_element_.Get()->IsReallyEnabled() &&
          (event.GetButton() & MouseEvent::BUTTON_LEFT) &&
          (type == Event::EVENT_MOUSE_MOVE || type == Event::EVENT_MOUSE_UP ||
           type == Event::EVENT_MOUSE_CLICK)) {
        MouseEvent new_event(event);
        MapChildMouseEvent(event, grabmouse_element_.Get(), &new_event);
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
        MapChildMouseEvent(event, mouseover_element_.Get(), &new_event);
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
      MapChildMouseEvent(event, popup_element_.Get(), &new_event);
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
      // Focus is handled in BasicElement.
    }

    if (fired_element_holder.Get() != mouseover_element_.Get()) {
      BasicElement *old_mouseover_element = mouseover_element_.Get();
      // Store it early to prevent crash if fired_element is removed in
      // the mouseout handler.
      mouseover_element_.Reset(fired_element_holder.Get());

      if (old_mouseover_element) {
        MouseEvent mouseout_event(Event::EVENT_MOUSE_OUT,
                                  event.GetX(), event.GetY(),
                                  event.GetWheelDeltaX(),
                                  event.GetWheelDeltaY(),
                                  event.GetButton(),
                                  event.GetModifier());
        MapChildMouseEvent(event, old_mouseover_element, &mouseout_event);
        old_mouseover_element->OnMouseEvent(mouseout_event, true,
                                            &temp, &temp1);
      }

      if (mouseover_element_.Get()) {
        // The enabled and visible states may change during event handling.
        if (!mouseover_element_.Get()->IsReallyEnabled()) {
          mouseover_element_.Reset(NULL);
        } else {
          MouseEvent mouseover_event(Event::EVENT_MOUSE_OVER,
                                     event.GetX(), event.GetY(),
                                     event.GetWheelDeltaX(),
                                     event.GetWheelDeltaY(),
                                     event.GetButton(),
                                     event.GetModifier());
          MapChildMouseEvent(event, mouseover_element_.Get(), &mouseover_event);
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
      // Gets the hit test value of the element currently pointed by mouse.
      // It'll be used as the hit test value of the View.
      hittest_ = in_element->GetHitTest();
      //DLOG("In element: %s type %s, hitTest:%d", in_element->GetName().c_str(),
      //     in_element->GetTagName().c_str(), hittest_);
    } else {
      host_->SetCursor(-1);
      tooltip_element_.Reset(NULL);
      hittest_ = ViewInterface::HT_CLIENT;
    }

    return result;
  }

  EventResult OnMouseEvent(const MouseEvent &event) {
    // Send event to view first.
    ScriptableEvent scriptable_event(&event, NULL, NULL);
    Event::Type type = event.GetType();
    if (type != Event::EVENT_MOUSE_MOVE)
      DLOG("%s(view): x:%g y:%g dx:%d dy:%d b:%d m:%d", scriptable_event.GetName(),
           event.GetX(), event.GetY(),
           event.GetWheelDeltaX(), event.GetWheelDeltaY(),
           event.GetButton(), event.GetModifier());
    switch (type) {
      case Event::EVENT_MOUSE_MOVE:
        // Put the high volume events near top.
        FireEvent(&scriptable_event, onmousemove_event_);
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
    if (result != EVENT_RESULT_CANCELED) {
      if (type == Event::EVENT_MOUSE_OVER) {
        // Translate the mouse over event to a mouse move event, to make sure
        // that the correct mouseover element will be set.
        MouseEvent new_event(Event::EVENT_MOUSE_MOVE,
                             event.GetX(), event.GetY(), 0, 0,
                             MouseEvent::BUTTON_NONE, MouseEvent::MOD_NONE);
        result = SendMouseEventToChildren(new_event);
      } else {
        result = SendMouseEventToChildren(event);
      }
    }

    // Child handled or cancelled the event, just return.
    if (result != EVENT_RESULT_UNHANDLED)
      return result;

    if(event.GetType() == Event::EVENT_MOUSE_DOWN &&
       event.GetButton() == MouseEvent::BUTTON_RIGHT) {
      // Handle ShowContextMenu event.
      if (host_->ShowContextMenu(MouseEvent::BUTTON_RIGHT))
        result = EVENT_RESULT_HANDLED;
    }

    return result;
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

    if (focused_element_.Get()) {
      if (!focused_element_.Get()->IsReallyEnabled()) {
        focused_element_.Get()->OnOtherEvent(
            SimpleEvent(Event::EVENT_FOCUS_OUT));
        focused_element_.Reset(NULL);
      } else {
        result = focused_element_.Get()->OnKeyEvent(event);
      }
    }
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
        if (!dragover_element_.Get()->IsReallyVisible()) {
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

  EventResult OnOtherEvent(const Event &event) {
    ScriptableEvent scriptable_event(&event, NULL, NULL);
    DLOG("%s(view)", scriptable_event.GetName());
    switch (event.GetType()) {
      case Event::EVENT_FOCUS_IN:
        // For now we don't automatically set focus to some element.
        break;
      case Event::EVENT_FOCUS_OUT:
        SetFocus(NULL);
        break;
      case Event::EVENT_CANCEL:
        FireEvent(&scriptable_event, oncancel_event_);
        break;
      case Event::EVENT_CLOSE:
        FireEvent(&scriptable_event, onclose_event_);
        break;
      case Event::EVENT_DOCK:
        FireEvent(&scriptable_event, ondock_event_);
        break;
      case Event::EVENT_MINIMIZE:
        FireEvent(&scriptable_event, onminimize_event_);
        break;
      case Event::EVENT_OK:
        FireEvent(&scriptable_event, onok_event_);
        break;
      case Event::EVENT_OPEN:
        FireEvent(&scriptable_event, onopen_event_);
        break;
      case Event::EVENT_POPIN:
        FireEvent(&scriptable_event, onpopin_event_);
        break;
      case Event::EVENT_POPOUT:
        FireEvent(&scriptable_event, onpopout_event_);
        break;
      case Event::EVENT_RESTORE:
        FireEvent(&scriptable_event, onrestore_event_);
        break;
      case Event::EVENT_SIZING:
        FireEvent(&scriptable_event, onsizing_event_);
        break;
      case Event::EVENT_UNDOCK:
        FireEvent(&scriptable_event, onundock_event_);
        break;
      default:
        ASSERT(false);
    }
    return scriptable_event.GetReturnValue();
  }

  void SetSize(double width, double height) {
    if (width != width_ || height != height_) {
      // Invalidate the canvas cache.
      if (enable_cache_ && canvas_cache_) {
        canvas_cache_->Destroy();
        canvas_cache_ = NULL;
      }

      width_ = width;
      height_ = height;

      if (host_)
        host_->QueueResize();

      SimpleEvent event(Event::EVENT_SIZE);
      ScriptableEvent scriptable_event(&event, NULL, NULL);
      FireEvent(&scriptable_event, onsize_event_);
    }
  }

  void ResizeBy(double width, double height) {
    SetSize(width_ + width, height_ + height);
  }

  double GetWidth() {
    return width_;
  }

  double GetHeight() {
    return height_;
  }

  void MarkRedraw() {
    need_redraw_ = true;
    children_.MarkRedraw();
  }

  void Draw(CanvasInterface *canvas) {
#ifdef _DEBUG
    draw_count_ = 0;
    uint64_t start = main_loop_->GetCurrentTime();
#endif
    // no draw queued, so the draw request is initiated from host.
    // And because the canvas cache_ is valid, just need to paint the canvas
    // cache to the dest canvas.
    if (!draw_queued_ && canvas_cache_ && !need_redraw_) {
      DLOG("Draw from canvas cache.");
      canvas->DrawCanvas(0, 0, canvas_cache_);
      return;
    }

    // Any QueueDraw() called during Layout() will be ignored, because
    // draw_queued_ is true.
    draw_queued_ = true;
    children_.Layout();
    draw_queued_ = false;

#ifdef _DEBUG
    clip_region_.PrintLog();
#endif

    if (enable_cache_ && !canvas_cache_) {
      canvas_cache_ = host_->GetGraphics()->NewCanvas(width_, height_);
      need_redraw_ = true;
    }

    // Let posted events be processed after Layout() and before actual Draw().
    // This can prevent some flickers, for example, onsize of labels.
    FirePostedEvents();

    if (need_redraw_)
      clip_region_.Clear();
    else
      clip_region_.Integerize();

    CanvasInterface *target = enable_cache_ ? canvas_cache_ : canvas;

    target->PushState();
    target->IntersectGeneralClipRegion(clip_region_);
    target->ClearRect(0, 0, width_, height_);

    // No need to draw children if there is a popup element and it's fully
    // opaque and the clip region is inside its extents.
    // If the popup element has non-horizontal/vertical orientation,
    // then the region of the popup element may not cover the whole clip
    // region. In this case it's not safe to skip drawing other children.
    bool skip_children = false;
    if (popup_element_.Get() && popup_element_.Get()->IsFullyOpaque() &&
        clip_region_.IsInside(popup_element_.Get()->GetExtentsInView())) {
      double rotation = 0;
      BasicElement *e = popup_element_.Get();
      for (; e != NULL; e = e->GetParentElement()) {
        rotation += e->GetRotation();
      }
      if (fmod(rotation, 90) == 0) {
        skip_children = true;
      } else {
        DLOG("The popup element has non-horizontal/vertical orientation.");
      }
    }

    if (!skip_children)
      children_.Draw(target);

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
          target->TranslateCoordinates(
              element->GetPixelX() - element->GetPixelPinX(),
              element->GetPixelY() - element->GetPixelPinY());
        } else {
          target->TranslateCoordinates(element->GetPixelX(),
                                              element->GetPixelY());
          target->RotateCoordinates(
              DegreesToRadians(element->GetRotation()));
          target->TranslateCoordinates(-element->GetPixelPinX(),
                                              -element->GetPixelPinY());
        }
      }

      popup_element_.Get()->Draw(target);
    }
    target->PopState();
    if (enable_cache_)
      canvas->DrawCanvas(0, 0, canvas_cache_);
    clip_region_.Clear();
    need_redraw_ = false;

#ifdef _DEBUG
    uint64_t end = main_loop_->GetCurrentTime();
    if (end > 0 && start > 0)
      DLOG("Draw count: %d, time: %ju", draw_count_, end - start);
#endif
  }

  bool OnAddContextMenuItems(MenuInterface *menu) {
    bool result = true;
    if (mouseover_element_.Get()) {
      if (mouseover_element_.Get()->IsReallyEnabled())
        result = mouseover_element_.Get()->OnAddContextMenuItems(menu);
      else
        mouseover_element_.Reset(NULL);
    }

    // If the view is main and the mouse over element doesn't have special menu
    // items, then add gadget's menu items.
    if (result && gadget_ &&
        host_->GetType() == ViewHostInterface::VIEW_HOST_MAIN)
      gadget_->OnAddCustomMenuItems(menu);

    return result;
  }

  bool OnSizing(double *width, double *height) {
    ASSERT(width);
    ASSERT(height);

    SizingEvent event(*width, *height);
    ScriptableEvent scriptable_event(&event, NULL, &event);

    FireEvent(&scriptable_event, onsizing_event_);
    bool result = (scriptable_event.GetReturnValue() != EVENT_RESULT_CANCELED);

    DLOG("onsizing view %s, request: %lf x %lf, actual: %lf x %lf",
         (result ? "accepted" : "cancelled"),
         *width, *height, event.GetWidth(), event.GetHeight());

    if (result) {
      *width = event.GetWidth();
      *height = event.GetHeight();
    }

    return result;
  }

  void FireEventSlot(ScriptableEvent *event, const Slot *slot) {
    ASSERT(event);
    ASSERT(slot);
    event->SetReturnValue(EVENT_RESULT_HANDLED);
    event_stack_.push_back(event);
    slot->Call(0, NULL);
    event_stack_.pop_back();
  }

  void FireEvent(ScriptableEvent *event, const EventSignal &event_signal) {
    if (events_enabled_ && event_signal.HasActiveConnections()) {
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

  ScriptableEvent *GetEvent() {
    return event_stack_.empty() ? NULL : *(event_stack_.end() - 1);
  }

  BasicElement *GetElementByName(const char *name) {
    ElementsMap::iterator it = all_elements_.find(name);
    return it == all_elements_.end() ? NULL : it->second;
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
    owner_->AddElementToClipRegion(element);
    if (element == tooltip_element_.Get() && host_)
      host_->SetTooltip(NULL);

    std::string name = element->GetName();
    if (!name.empty()) {
      ElementsMap::iterator it = all_elements_.find(name);
      if (it != all_elements_.end() && it->second == element)
        all_elements_.erase(it);
    }
  }

  void SetFocus(BasicElement *element) {
    if (element != focused_element_.Get() &&
        (!element || element->IsReallyEnabled())) {
      ElementHolder element_holder(element);
      // Remove the current focus first.
      if (!focused_element_.Get() ||
          focused_element_.Get()->OnOtherEvent(SimpleEvent(
              Event::EVENT_FOCUS_OUT)) != EVENT_RESULT_CANCELED) {
        ElementHolder old_focused_element(focused_element_.Get());
        focused_element_.Reset(element_holder.Get());
        if (focused_element_.Get()) {
          // The enabled or visible state may changed, so check again.
          if (!focused_element_.Get()->IsReallyEnabled() ||
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

  void SetPopupElement(BasicElement *element) {
    if (popup_element_.Get()) {
      popup_element_.Get()->OnPopupOff();
    }
    popup_element_.Reset(element);
    if (element) {
      element->QueueDraw();
    }
  }

  int BeginAnimation(Slot *slot, int start_value, int end_value,
                     unsigned int duration) {
    if (!slot) {
      LOG("Invalid slot for animation.");
      return 0;
    }

    uint64_t current_time = main_loop_->GetCurrentTime();
    TimerWatchCallback *watch =
        new TimerWatchCallback(this, slot, start_value, end_value,
                               duration, current_time, true);
    int id = main_loop_->AddTimeoutWatch(kAnimationInterval, watch);
    watch->SetWatchId(id);
    return id;
  }

  int SetTimeout(Slot *slot, unsigned int duration) {
    if (!slot) {
      LOG("Invalid slot for timeout.");
      return 0;
    }

    TimerWatchCallback *watch =
        new TimerWatchCallback(this, slot, 0, 0, 0, 0, true);
    int id = main_loop_->AddTimeoutWatch(duration, watch);
    watch->SetWatchId(id);
    return id;
  }

  int SetInterval(Slot *slot, unsigned int duration) {
    if (!slot) {
      LOG("Invalid slot for interval.");
      return 0;
    }

    TimerWatchCallback *watch =
        new TimerWatchCallback(this, slot, 0, 0, -1, 0, true);
    int id = main_loop_->AddTimeoutWatch(duration, watch);
    watch->SetWatchId(id);
    return id;
  }

  void RemoveTimer(int token) {
    if (token > 0)
      main_loop_->RemoveWatch(token);
  }

  ImageInterface *LoadImage(const Variant &src, bool is_mask) {
    if (!gadget_ || !host_) return NULL;

    Variant::Type type = src.type();
    if (type == Variant::TYPE_STRING) {
      const char *filename = VariantValue<const char*>()(src);
      if (filename && *filename) {
        std::string data;
        if (gadget_->GetFileManager()->ReadFile(filename, &data)) {
          return host_->GetGraphics()->NewImage(filename, data, is_mask);
        }
      }
    } else if (type == Variant::TYPE_SCRIPTABLE) {
      const ScriptableBinaryData *binary =
          VariantValue<const ScriptableBinaryData *>()(src);
      if (binary)
        return host_->GetGraphics()->NewImage("", binary->data(), is_mask);
    } else {
      LOG("Unsupported type of image src.");
      DLOG("src=%s", src.Print().c_str());
    }
    return NULL;
  }

  ImageInterface *LoadImageFromGlobal(const char *name, bool is_mask) {
    if (name && *name && host_) {
      std::string data;
      if (GetGlobalFileManager()->ReadFile(name, &data)) {
        return host_->GetGraphics()->NewImage(name, data, is_mask);
      }
    }
    return NULL;
  }

  Texture *LoadTexture(const Variant &src) {
    Color color;
    double opacity;
    if (src.type() == Variant::TYPE_STRING) {
      const char *name = VariantValue<const char *>()(src);
      if (name && name[0] == '#' && Color::FromString(name, &color, &opacity))
        return new Texture(color, opacity);
    }

    ImageInterface *image = LoadImage(src, false);
    return image ? new Texture(image) : NULL;
  }

  void OnOptionChanged(const char *name) {
    OptionChangedEvent event(name);
    ScriptableEvent scriptable_event(&event, NULL, NULL);
    FireEvent(&scriptable_event, onoptionchanged_event_);
  }

  bool OpenURL(const char *url) {
    if (!gadget_) return false;

    // Important: verify that URL is valid first.
    // Otherwise could be a security problem.
    std::string newurl = EncodeURL(url);
    if (IsValidURL(url)) {
      return gadget_->GetHost()->OpenURL(newurl.c_str());
    }

    DLOG("Malformed URL: %s", newurl.c_str());
    return false;
  }

 public: // member variables
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
  EventSignal onmousemove_event_;
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

  // Note: though other things are case-insenstive, this map is case-sensitive,
  // to keep compatible with the Windows version.
  typedef std::map<std::string, BasicElement *> ElementsMap;
  // Put all_elements_ here to make it the last member to be destructed,
  // because destruction of children_ needs it.
  ElementsMap all_elements_;

  ClipRegion clip_region_;

  View *owner_;
  Gadget *gadget_;
  ElementFactory *element_factory_;
  MainLoopInterface *main_loop_;
  ViewHostInterface *host_;
  ScriptContextInterface *script_context_;
  Connection *onoptionchanged_connection_;
  CanvasInterface *canvas_cache_;
  bool enable_cache_;

  Elements children_;

  ElementHolder focused_element_;
  ElementHolder mouseover_element_;
  ElementHolder grabmouse_element_;
  ElementHolder dragover_element_;
  ElementHolder tooltip_element_;
  ElementHolder popup_element_;
  ScriptableHolder<ContentAreaElement> content_area_element_;

  typedef std::vector<std::pair<ScriptableEvent *, const EventSignal *> >
      PostedEvents;
  PostedEvents posted_events_;

  std::vector<ScriptableEvent *> event_stack_;

  EventResult dragover_result_;
  double width_;
  double height_;
  ResizableMode resizable_;
  std::string caption_;
  bool show_caption_always_;

  int post_event_token_;
  bool draw_queued_;
  bool events_enabled_;
  bool need_redraw_;
  int draw_count_;

  ViewInterface::HitTest hittest_;

  Signal0<void> on_destroy_signal_;

  static const unsigned int kAnimationInterval = 20;
  static const unsigned int kMinInterval = 5;
};

View::View(ViewHostInterface *host,
           Gadget *gadget,
           ElementFactory *element_factory,
           ScriptContextInterface *script_context)
    : impl_(new Impl(this, host, gadget, element_factory, script_context)) {
  // Make sure that the view is initialized when attaching to the ViewHost.
  impl_->host_->SetView(this);
}

View::~View() {
  delete impl_;
  impl_ = NULL;
}

Gadget *View::GetGadget() const {
  return impl_->gadget_;
}

ScriptContextInterface *View::GetScriptContext() const {
  return impl_->script_context_;
}

FileManagerInterface *View::GetFileManager() const {
  return impl_->gadget_->GetFileManager();
}

const GraphicsInterface *View::GetGraphics() const {
  return impl_->host_ ? impl_->host_->GetGraphics() : NULL;
}

void View::RegisterProperties(RegisterableInterface *obj) const {
  impl_->RegisterProperties(obj);
}

void View::SetWidth(double width) {
  impl_->SetSize(width, impl_->height_);
}

void View::SetHeight(double height) {
  impl_->SetSize(impl_->width_, height);
}

void View::SetSize(double width, double height) {
  impl_->SetSize(width, height);
}

double View::GetWidth() const {
  return impl_->width_;
}

double View::GetHeight() const {
  return impl_->height_;
}

void View::SetResizable(ViewInterface::ResizableMode resizable) {
  impl_->resizable_ = resizable;
  if (impl_->host_)
    impl_->host_->SetResizable(resizable);
}

ViewInterface::ResizableMode View::GetResizable() const {
  return impl_->resizable_;
}

void View::SetCaption(const char *caption) {
  impl_->caption_ = caption ? caption : "";
  if (impl_->host_)
    impl_->host_->SetCaption(caption);
}

std::string View::GetCaption() const {
  return impl_->caption_;
}

void View::SetShowCaptionAlways(bool show_always) {
  impl_->show_caption_always_ = show_always;
  if (impl_->host_)
    impl_->host_->SetShowCaptionAlways(show_always);
}

bool View::GetShowCaptionAlways() const {
  return impl_->show_caption_always_;
}

void View::MarkRedraw() {
  impl_->MarkRedraw();
}

void View::Draw(CanvasInterface *canvas) {
  impl_->Draw(canvas);
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

EventResult View::OnOtherEvent(const Event &event) {
  return impl_->OnOtherEvent(event);
}

ViewInterface::HitTest View::GetHitTest() const {
  return impl_->hittest_;
}

bool View::OnAddContextMenuItems(MenuInterface *menu) {
  return impl_->OnAddContextMenuItems(menu);
}

bool View::OnSizing(double *width, double *height) {
  return impl_->OnSizing(width, height);
}

void View::FireEvent(ScriptableEvent *event, const EventSignal &event_signal) {
  impl_->FireEvent(event, event_signal);
}

void View::PostEvent(ScriptableEvent *event, const EventSignal &event_signal) {
  impl_->PostEvent(event, event_signal);
}

ScriptableEvent *View::GetEvent() const {
  return impl_->GetEvent();
}

void View::EnableEvents(bool enable_events) {
  impl_->events_enabled_ = enable_events;
}

ElementFactory *View::GetElementFactory() const {
  return impl_->element_factory_;
}

Elements *View::GetChildren() const {
  return &impl_->children_;
}

BasicElement *View::GetElementByName(const char *name) const {
  return impl_->GetElementByName(name);
}

bool View::OnElementAdd(BasicElement *element) {
  return impl_->OnElementAdd(element);
}

void View::OnElementRemove(BasicElement *element) {
  impl_->OnElementRemove(element);
}

void View::SetFocus(BasicElement *element) {
  impl_->SetFocus(element);
}

void View::SetPopupElement(BasicElement *element) {
  impl_->SetPopupElement(element);
}

BasicElement *View::GetPopupElement() const {
  return impl_->popup_element_.Get();
}

BasicElement *View::GetFocusedElement() const {
  return impl_->focused_element_.Get();
}

BasicElement *View::GetMouseOverElement() const {
  return impl_->mouseover_element_.Get();
}

ContentAreaElement *View::GetContentAreaElement() const {
  return impl_->content_area_element_.Get();
}

bool View::IsElementInClipRegion(const BasicElement *element) const {
  return impl_->clip_region_.IsEmpty() ||
         impl_->clip_region_.Overlaps(element->GetExtentsInView());
}

void View::AddElementToClipRegion(BasicElement *element) {
  impl_->clip_region_.AddRectangle(element->GetExtentsInView());
}

void View::IncreaseDrawCount() {
  impl_->draw_count_++;
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

ImageInterface *View::LoadImage(const Variant &src, bool is_mask) const {
  return impl_->LoadImage(src, is_mask);
}

ImageInterface *
View::LoadImageFromGlobal(const char *name, bool is_mask) const {
  return impl_->LoadImageFromGlobal(name, is_mask);
}

Texture *View::LoadTexture(const Variant &src) const {
  return impl_->LoadTexture(src);
}

void *View::GetNativeWidget() const {
  return impl_->host_ ? impl_->host_->GetNativeWidget() : NULL;
}

void View::ViewCoordToNativeWidgetCoord(
    double x, double y, double *widget_x, double *widget_y) const {
  if (impl_->host_)
    impl_->host_->ViewCoordToNativeWidgetCoord(x, y, widget_x, widget_y);
}

void View::QueueDraw() {
  if (!impl_->draw_queued_ && impl_->host_) {
    impl_->draw_queued_ = true;
    impl_->host_->QueueDraw();
  }
}

ViewInterface::DebugMode View::GetDebugMode() const {
  return impl_->host_ ? impl_->host_->GetDebugMode() :
         ViewInterface::DEBUG_DISABLED;
}

bool View::OpenURL(const char *url) const {
  return impl_->OpenURL(url);
}

void View::Alert(const char *message) const {
  if (impl_->host_)
    impl_->host_->Alert(message);
}

bool View::Confirm(const char *message) const {
  return impl_->host_ ? impl_->host_->Confirm(message) : false;
}

std::string
View::Prompt(const char *message, const char *default_result) const {
  return impl_->host_ ? impl_->host_->Prompt(message, default_result) : "";
}

uint64_t View::GetCurrentTime() const {
  return impl_->main_loop_->GetCurrentTime();
}

void View::SetTooltip(const char *tooltip) {
  if (impl_->host_)
    impl_->host_->SetTooltip(tooltip);
}

bool View::ShowView(bool modal, int flags, Slot1<void, int> *feedback_handler) {
  return impl_->host_ ? impl_->host_->ShowView(modal, flags, feedback_handler) :
         false;
}

void View::CloseView() {
  if (impl_->host_)
    impl_->host_->CloseView();
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
Connection *View::ConnectOnMouseMoveEvent(Slot0<void> *handler) {
  return impl_->onmousemove_event_.Connect(handler);
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
