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

#include "view.h"
#include "view_impl.h"
#include "element_factory.h"
#include "element_interface.h"
#include "event.h"
#include "graphics_interface.h"
#include "host_interface.h"
#include "slot.h"

namespace ggadget {

namespace internal {

ViewImpl::ViewImpl(ElementFactoryInterface *element_factory,
                   ViewInterface *owner) 
    : children_(element_factory, NULL, owner),
      width_(200), height_(200),
      host_(NULL),
      canvas_(NULL),
      // TODO: Make sure the default value.
      resizable_(ViewInterface::RESIZABLE_TRUE),
      show_caption_always_(false),
      current_timer_token_(1) {
}

ViewImpl::~ViewImpl() {
  if (canvas_) {
    canvas_->Destroy();
    canvas_ = NULL;
  }
}

void ViewImpl::OnMouseEvent(MouseEvent *event) {
  switch (event->GetType()) {
    case Event::EVENT_MOUSE_MOVE: // put the high volume events near top
    // DLOG("mousemove");
    // View doesn't have mouse move event according to the API document.
    break;
   case Event::EVENT_MOUSE_DOWN:
    DLOG("mousedown");
    FireEvent(event, onmousedown_event_);
    break;
   case Event::EVENT_MOUSE_UP:
    DLOG("mouseup");
    FireEvent(event, onmouseup_event_);
    break;
   case Event::EVENT_MOUSE_CLICK:
    DLOG("click %g %g", event->GetX(), event->GetY());
    FireEvent(event, onclick_event_);
    break;
   case Event::EVENT_MOUSE_DBLCLICK:
    DLOG("dblclick %g %g", event->GetX(), event->GetY());
    FireEvent(event, ondblclick_event_);
    break;
   case Event::EVENT_MOUSE_OUT:
    DLOG("mouseout");
    FireEvent(event, onmouseout_event_);
    break;
   case Event::EVENT_MOUSE_OVER:
    DLOG("mouseover");
    FireEvent(event, onmouseover_event_);
    break;
   case Event::EVENT_MOUSE_WHEEL:
    DLOG("mousewheel");
    // View doesn't have mouse wheel event according to the API document.
    break;
   default:
    ASSERT(false);
  };
}

void ViewImpl::OnKeyEvent(KeyboardEvent *event) {
  switch (event->GetType()) {
   case Event::EVENT_KEY_DOWN:
    DLOG("keydown");
    FireEvent(event, onkeydown_event_);      
    break;
   case Event::EVENT_KEY_UP:
    DLOG("keyup");
    FireEvent(event, onkeyup_event_);
    break;
   case Event::EVENT_KEY_PRESS:
    DLOG("keypress");
    FireEvent(event, onkeypress_event_);
    break;
   default:
    ASSERT(false);
  };
}

void ViewImpl::OnTimerEvent(TimerEvent *event) {
  ASSERT(event->GetType() == Event::EVENT_TIMER_TICK);
  DLOG("timer tick");

  if (event->GetTarget()) {
    // TODO: Dispatch the event to the element target. 
  } else {
    // The target is this view.
    int token = reinterpret_cast<int>(event->GetData());
    TimerMap::iterator it = timer_map_.find(token);
    if (it == timer_map_.end()) {
      LOG("Timer has been removed but event still fired: %d", token);
      return;
    }

    TimerInfo &info = it->second;
    ASSERT(info.token == token);
    switch (info.type) {
      case TIMER_TIMEOUT:
        event->StopReceivingMore();
        RemoveTimer(token);
        info.slot->Call(0, NULL);
        break;
      case TIMER_INTERVAL:
        info.slot->Call(0, NULL);
        break;
      case TIMER_ANIMATION: {
        uint64_t event_time = event->GetTimeStamp();
        double progress = static_cast<double>(event_time - info.start_time) /
                          1000.0 / info.duration;
        progress = std::min(1.0, std::max(0.0, progress));
        int value = info.start_value + static_cast<int>(progress * info.spread);
        if (value != info.last_value) {
          info.last_value = value;
          Variant param(value);
          info.slot->Call(1, &param);
        }
        if (progress == 1.0) {
          event->StopReceivingMore();
          RemoveTimer(token);
        }
        break;
      }
      default:
        ASSERT(false);
        break;
    }
  }
}

void ViewImpl::OnOtherEvent(Event *event) {
  switch (event->GetType()) {
   case Event::EVENT_FOCUS_IN:
    DLOG("focusin");
    // View doesn't have focus in event according to the API document.
    break;
   case Event::EVENT_FOCUS_OUT:
    DLOG("focusout");
    // View doesn't have focus out event according to the API document.     
    break;
   default:
    ASSERT(false);
  };
}

bool ViewImpl::SetWidth(int width) {
  // TODO check if allowed first
  if (canvas_) {
    canvas_->Destroy();
    canvas_ = NULL;
  }
  width_ = width;  
  if (host_) {
    host_->QueueDraw();
  }

  // TODO: Is NULL a proper current event object for the script?
  FireEvent(NULL, onsize_event_);
  return true;
}

bool ViewImpl::SetHeight(int height) {
  // TODO check if allowed first
  if (canvas_) {
    canvas_->Destroy();
    canvas_ = NULL;
  }
  height_ = height;
  if (host_) {
    host_->QueueDraw();
  }

  // TODO: Is NULL a proper current event object for the script?
  FireEvent(NULL, onsize_event_);
  return true;
}

bool ViewImpl::SetSize(int width, int height) {
  // TODO check if allowed first
  if (canvas_) {
    canvas_->Destroy();
    canvas_ = NULL;
  }
  width_ = width;  
  height_ = height;
  if (host_) {
    host_->QueueDraw();
  } 
  
  // TODO: Is NULL a proper current event object for the script?
  FireEvent(NULL, onsize_event_);
  return true;
}

bool ViewImpl::ResizeBy(int width, int height) {
  return SetSize(width_ + width, height_ + height);
}

bool ViewImpl::AttachHost(HostInterface *host) {
  if (host_) {
    // Detach old host first
    if (!host_->DetachFromView()) {
      return false;
    }
  }
  
  host_ = host;
  
  // TODO If host != NULL, should re-register all timers
  
  children_.HostChanged();
  
  return true;
}
   
const CanvasInterface *ViewImpl::Draw(bool *changed) {
  // Always set changed to true for now.
  
  if (!canvas_) {
    ASSERT(host_); // host must be initialized before drawing
    const GraphicsInterface *gfx = host_->GetGraphics();
    canvas_ = gfx->NewCanvas(width_, height_);
    if (!canvas_) {
      DLOG("Error: unable to create canvas.");
      return NULL;
    }
  }
  else {
    // If not new canvas, we must remember to always clear canvas before 
    // drawing since we're not checking dirty flag yet.
    canvas_->ClearCanvas();
  }
  
  // TODO test demo
  canvas_->DrawLine(0, 0, 0, height_, 1, Color(0, 0, 0));
  canvas_->DrawLine(0, 0, width_, 0, 1, Color(0, 0, 0));
  canvas_->DrawLine(width_, height_, 0, height_, 1, Color(0, 0, 0));
  canvas_->DrawLine(width_, height_, width_, 0, 1, Color(0, 0, 0));
  canvas_->DrawFilledRect(10, 10, 10, 10, Color(1, 1, 1));
  canvas_->DrawFilledRect(10, 20, 10, 10, Color(0, 0, 0));
  
  canvas_->MultiplyOpacity(.5);
  canvas_->PushState();
  canvas_->DrawFilledRect(10., 10., 280., 130., Color(1., 0., 0.));
  canvas_->IntersectRectClipRegion(30., 30., 100., 100.);
  canvas_->IntersectRectClipRegion(70., 40., 100., 70.);
  canvas_->DrawFilledRect(20., 20., 260., 110., Color(0., 1., 0.));  
  canvas_->PopState();
  canvas_->DrawFilledRect(110., 40., 90., 70., Color(0., 0., 1.));  
  // end test 
  
  return canvas_;
}

void ViewImpl::OnElementAdd(ElementInterface *element) {
  ASSERT(element);
  const char *name = element->GetName();
  if (name && *name &&
      // Don't overwrite the existing element with the same name.
      all_elements_.find(name) == all_elements_.end())
    all_elements_[name] = element;
}

void ViewImpl::OnElementRemove(ElementInterface *element) {
  ASSERT(element);
  const char *name = element->GetName();
  if (name && *name) {
    ElementsMap::iterator it = all_elements_.find(name);
    if (it->second == element)
      all_elements_.erase(it);
  }
}

void ViewImpl::FireEvent(Event *event, const EventSignal &event_signal) {
  event_stack_.push_back(event);
  event_signal();
  event_stack_.pop_back();
}

Event *ViewImpl::GetEvent() const {
  return event_stack_.empty() ? NULL : event_stack_[event_stack_.size() - 1];
}

void ViewImpl::SetResizable(ViewInterface::ResizableMode resizable) {
  resizable_ = resizable;
  // TODO:
}

void ViewImpl::SetCaption(const char *caption) {
  caption_ = caption ? caption : "";
  // TODO: Redraw?
}
  
void ViewImpl::SetShowCaptionAlways(bool show_always) {
  show_caption_always_ = show_always;
  // TODO: Redraw?
}

ElementInterface *ViewImpl::GetElementByName(const char *name) {
  ElementsMap::iterator it = all_elements_.find(name);
  return it == all_elements_.end() ? NULL : it->second;
}

int ViewImpl::NewTimer(TimerType type, Slot *slot,
                       int start_value, int end_value, unsigned int duration) {
  ASSERT(slot);
  if (duration == 0 || !host_)
    return 0;

  // Find the next available timer token.
  // Ignore the error when all timer tokens are allocated.
  do {
    if (current_timer_token_ < INT_MAX) current_timer_token_++;
    else current_timer_token_ = 1;
  } while (timer_map_.find(current_timer_token_) != timer_map_.end());
  
  TimerInfo &info = timer_map_[current_timer_token_];
  info.token = current_timer_token_;
  info.type = type;
  info.slot = slot;
  info.start_value = start_value;
  info.last_value = end_value;
  info.spread = end_value - start_value;
  info.duration = duration;
  info.start_time = host_->GetCurrentTime();
  info.host_timer = host_->RegisterTimer(
      type == TIMER_ANIMATION ? kAnimationInterval : duration, 
      NULL,
      // Passing an integer is safer than passing a struct pointer.
      reinterpret_cast<void *>(current_timer_token_));
  return current_timer_token_;
}

void ViewImpl::RemoveTimer(int token) {
  if (token == 0)
    return;

  TimerMap::iterator it = timer_map_.find(token);
  if (it == timer_map_.end()) {
    LOG("Invalid timer token to remove: %d", token);
    return;
  }

  host_->RemoveTimer(it->second.host_timer);
  delete it->second.slot;
  timer_map_.erase(it);
}

int ViewImpl::BeginAnimation(Slot *slot, int start_value, int end_value,
                             unsigned int duration) {
  return NewTimer(TIMER_ANIMATION, slot, start_value, end_value, duration);
}

void ViewImpl::CancelAnimation(int token) {
  RemoveTimer(token);
}

int ViewImpl::SetTimeout(Slot *slot, unsigned int duration) {
  return NewTimer(TIMER_TIMEOUT, slot, 0, 0, duration);
}

void ViewImpl::ClearTimeout(int token) {
  RemoveTimer(token);
}

int ViewImpl::SetInterval(Slot *slot, unsigned int duration) {
  return NewTimer(TIMER_INTERVAL, slot, 0, 0, duration);
}

void ViewImpl::ClearInterval(int token) {
  RemoveTimer(token);
}

} // namespace internal

using internal::ViewImpl;

static const char *kResizableNames[] = { "false", "true", "zoom" };

View::View(ElementFactoryInterface *element_factory)
    : impl_(new ViewImpl(element_factory, this)) {
  RegisterProperty("caption", NewSlot(this, &View::GetCaption),
                   NewSlot(this, &View::SetCaption));
  RegisterConstant("children", GetChildren());
  RegisterProperty("event", NewSlot(impl_, &ViewImpl::GetEvent), NULL);
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

  RegisterMethod("appendElement",
                 NewSlot(GetChildren(), &Elements::AppendElementFromXML));
  RegisterMethod("insertElement",
                 NewSlot(GetChildren(), &Elements::InsertElementFromXML));
  RegisterMethod("removeElement",
                 NewSlot(GetChildren(), &Elements::RemoveElement));

  // Here register ViewImpl::BeginAnimation because the Slot1<void, int> *
  // parameter in View::BeginAnimation can't be automatically reflected.
  RegisterMethod("beginAnimation", NewSlot(impl_, &ViewImpl::BeginAnimation));
  RegisterMethod("cancelAnimation", NewSlot(this, &View::CancelAnimation));
  RegisterMethod("setTimeout", NewSlot(impl_, &ViewImpl::SetTimeout));
  RegisterMethod("clearTimeout", NewSlot(this, &View::ClearTimeout));
  RegisterMethod("setInterval", NewSlot(impl_, &ViewImpl::SetInterval));
  RegisterMethod("clearInterval", NewSlot(this, &View::ClearInterval));

  // TODO: RegisterMethod("alert", NewSlot(this, &ViewImpl::Alert));
  // TODO: RegisterMethod("confirm", NewSlot(this, &ViewImpl::Confirm));

  RegisterMethod("resizeBy", NewSlot(impl_, &ViewImpl::ResizeBy));
  RegisterMethod("resizeTo", NewSlot(this, &View::SetSize));

  // TODO: Move it to OptionsView?
  RegisterSignal("oncancel", &impl_->oncancle_event_);
  RegisterSignal("onclick", &impl_->onclick_event_);
  RegisterSignal("onclose", &impl_->onclose_event_);
  RegisterSignal("ondblclick", &impl_->ondblclick_event_);
  RegisterSignal("ondock", &impl_->ondock_event_);
  RegisterSignal("onkeydown", &impl_->onkeydown_event_);
  RegisterSignal("onkeypress", &impl_->onkeypress_event_);
  RegisterSignal("onkeyup", &impl_->onkeyup_event_);
  RegisterSignal("onminimize", &impl_->onminimize_event_);
  RegisterSignal("onmousedown", &impl_->onmousedown_event_);
  RegisterSignal("onmouseout", &impl_->onmouseout_event_);
  RegisterSignal("onmouseover", &impl_->onmouseover_event_);
  RegisterSignal("onmouseup", &impl_->onmouseup_event_);
  // TODO: Move it to OptionsView?
  RegisterSignal("onok", &impl_->onok_event_);
  RegisterSignal("onopen", &impl_->onopen_event_);
  // TODO: Move it to OptionsView?
  RegisterSignal("onoptionchanged", &impl_->onoptionchanged_event_);
  RegisterSignal("onpopin", &impl_->onpopin_event_);
  RegisterSignal("onpopout", &impl_->onpopout_event_);
  RegisterSignal("onrestore", &impl_->onrestore_event_);
  RegisterSignal("onsize", &impl_->onsize_event_);
  RegisterSignal("onsizing", &impl_->onsizing_event_);
  RegisterSignal("onundock", &impl_->onundock_event_);

  SetDynamicPropertyHandler(NewSlot(impl_, &ViewImpl::GetElementByName), NULL);
}

View::~View() {
  delete impl_;
  impl_ = NULL;
}

bool View::AttachHost(HostInterface *host) { return impl_->AttachHost(host); };
  
int View::GetWidth() const { return impl_->width_; };
int View::GetHeight() const { return impl_->height_; };
 
const CanvasInterface *View::Draw(bool *changed) {
  return impl_->Draw(changed);
}

void View::OnMouseEvent(MouseEvent *event) {
  impl_->OnMouseEvent(event);
}

void View::OnKeyEvent(KeyboardEvent *event) {
  impl_->OnKeyEvent(event);
}

void View::OnTimerEvent(TimerEvent *event) {
  impl_->OnTimerEvent(event);
}

void View::OnOtherEvent(Event *event) {
  impl_->OnOtherEvent(event);
}

void View::OnElementAdd(ElementInterface *element) {
  impl_->OnElementAdd(element);
}

void View::OnElementRemove(ElementInterface *element) {
  impl_->OnElementRemove(element);
}

void View::FireEvent(Event *event, const EventSignal &event_signal) {
  impl_->FireEvent(event, event_signal);
}

Event *View::GetEvent() {
  return impl_->GetEvent();
}

const Event *View::GetEvent() const {
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
  impl_->SetResizable(resizable);
}

const Elements *View::GetChildren() const { return &impl_->children_; }
Elements *View::GetChildren() { return &impl_->children_; }

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
  impl_->SetCaption(caption);
}

const char *View::GetCaption() const {
  return impl_->caption_.c_str();
}

void View::SetShowCaptionAlways(bool show_always) {
  impl_->SetShowCaptionAlways(show_always);
}

bool View::GetShowCaptionAlways() const {
  return impl_->show_caption_always_;
}

int View::BeginAnimation(Slot1<void, int> *slot,
                         int start_value,
                         int end_value,
                         unsigned int duration) {
  return impl_->BeginAnimation(slot,
                               start_value, end_value, duration);
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

} // namespace ggadget
