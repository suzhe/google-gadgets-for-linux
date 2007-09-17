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

namespace ggadget {

namespace internal {

ViewImpl::ViewImpl(int width, int height, ViewInterface *owner) 
    : children_(ElementFactory::GetInstance(), NULL, owner),
      height_(height),
      host_(NULL),
      canvas_(NULL),
      // TODO: Make sure the default value.
      resizable_(ViewInterface::RESIZABLE_TRUE),
      show_caption_always_(false) {
}

ViewImpl::~ViewImpl() {
  if (canvas_) {
    canvas_->Destroy();
    canvas_ = NULL;
  }
}

void ViewImpl::OnMouseDown(MouseEvent *event) {
  DLOG("mousedown");
  FireEvent(event, onmousedown_event_);
}

void ViewImpl::OnMouseUp(MouseEvent *event) {
  DLOG("mouseup");
  FireEvent(event, onmouseup_event_);
}

void ViewImpl::OnClick(MouseEvent *event) {
  DLOG("click %g %g", event->GetX(), event->GetY());
  FireEvent(event, onclick_event_);
} 

void ViewImpl::OnDblClick(MouseEvent *event) {
  DLOG("dblclick %g %g", event->GetX(), event->GetY());
  FireEvent(event, ondblclick_event_);
}

void ViewImpl::OnMouseMove(MouseEvent *event) {
  // DLOG("mousemove");
  // View doesn't have mouse move event according to the API document.
}

void ViewImpl::OnMouseOut(MouseEvent *event) {
  DLOG("mouseout");
  FireEvent(event, onmouseout_event_);
}

void ViewImpl::OnMouseOver(MouseEvent *event) {
  DLOG("mouseover");
  FireEvent(event, onmouseover_event_);
}

void ViewImpl::OnMouseWheel(MouseEvent *event) {
  DLOG("mousewheel");
  // View doesn't have mouse wheel event according to the API document.
}

void ViewImpl::OnKeyDown(KeyboardEvent *event) {
  DLOG("keydown");
  FireEvent(event, onkeydown_event_);
}

void ViewImpl::OnKeyUp(KeyboardEvent *event) {
  DLOG("keyup");
  FireEvent(event, onkeyup_event_);
}

void ViewImpl::OnKeyPress(KeyboardEvent *event) {
  DLOG("keypress");
  FireEvent(event, onkeypress_event_);
}

void ViewImpl::OnFocusIn(Event *event) {
  DLOG("focusin");
  // View doesn't have focus in event according to the API document.
}

void ViewImpl::OnFocusOut(Event *event) {
  DLOG("focusout");
  // View doesn't have focus out event according to the API document.
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

void ViewImpl::OnElementAdded(ElementInterface *element) {
  ASSERT(element);
  const char *name = element->GetName();
  if (name && *name &&
      // Don't overwrite the existing element with the same name.
      all_elements_.find(name) == all_elements_.end())
    all_elements_[name] = element;
}

void ViewImpl::OnElementRemoved(ElementInterface *element) {
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

ElementInterface *ViewImpl::AppendElement(const char *tag_name,
                                          const char *name) {
  return children_.AppendElement(tag_name, name);
}

ElementInterface *ViewImpl::InsertElement(const char *tag_name,
                                          const ElementInterface *before,
                                          const char *name) {
  return children_.InsertElement(tag_name, before, name);
}

bool ViewImpl::RemoveElement(ElementInterface *child) {
  return children_.RemoveElement(child);
}

ElementInterface *ViewImpl::GetElementByName(const char *name) {
  ElementsMap::iterator it = all_elements_.find(name);
  return it == all_elements_.end() ? NULL : it->second;
}

} // namespace internal

using internal::ViewImpl;

static const char *kResizableNames[] = { "false", "true", "zoom" };

View::View(int width, int height)
    : impl_(new ViewImpl(width, height, this)) {
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
  // TODO: RegisterMethod("alert", NewSlot(this, &ViewImpl::Alert));
  // TODO: Support XML parameter.
  RegisterMethod("appendElement",
                 NewSlot(GetChildren(), &Elements::AppendElementFromXML));
  // TODO: RegisterMethod("beginAnimation", NewSlot(this, &ViewImpl::BeginAnimation));
  // TODO: RegisterMethod("cancelAnimation", NewSlot(this, &ViewImpl::CancelAnimation));
  // TODO: RegisterMethod("clearInterval", NewSlot(this, &ViewImpl::ClearInterval));
  // TODO: RegisterMethod("clearTimeout", NewSlot(this, &ViewImpl::ClearTimeout));
  // TODO: RegisterMethod("confirm", NewSlot(this, &ViewImpl::Confirm));
  RegisterMethod("insertElement",
                 NewSlot(GetChildren(), &Elements::InsertElementFromXML));
  RegisterMethod("removeElement",
                 NewSlot(GetChildren(), &Elements::RemoveElement));
  RegisterMethod("resizeBy", NewSlot(impl_, &ViewImpl::ResizeBy));
  RegisterMethod("resizeTo", NewSlot(this, &View::SetSize));
  // TODO: RegisterMethod("setTimeout", NewSlot(this, &ViewImpl::SetTimeout));
  // TODO: RegisterMethod("setInterval", NewSlot(this, &ViewImpl::SetInterval));
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

void View::OnMouseDown(MouseEvent *event) {
  impl_->OnMouseDown(event);
}

void View::OnMouseUp(MouseEvent *event) {
  impl_->OnMouseUp(event);
}

void View::OnClick(MouseEvent *event) {
  impl_->OnClick(event);
} 

void View::OnDblClick(MouseEvent *event) {
  impl_->OnDblClick(event);
}

void View::OnMouseMove(MouseEvent *event) {
  impl_->OnMouseMove(event);
}

void View::OnMouseOut(MouseEvent *event) {
  impl_->OnMouseOut(event);
}

void View::OnMouseOver(MouseEvent *event) {
  impl_->OnMouseOver(event);
}

void View::OnMouseWheel(MouseEvent *event) {
  impl_->OnMouseWheel(event);
}

void View::OnKeyDown(KeyboardEvent *event) {
  impl_->OnKeyDown(event);
}

void View::OnKeyUp(KeyboardEvent *event) {
  impl_->OnKeyUp(event);
}

void View::OnKeyPress(KeyboardEvent *event) {
  impl_->OnKeyPress(event);
}

void View::OnFocusIn(Event *event) {
  impl_->OnFocusIn(event);
}

void View::OnFocusOut(Event *event) {
  impl_->OnFocusOut(event);
}

void View::OnElementAdded(ElementInterface *element) {
  impl_->OnElementAdded(element);
}

void View::OnElementRemoved(ElementInterface *element) {
  impl_->OnElementRemoved(element);
}

void View::FireEvent(Event *event, const EventSignal &event_signal) {
  impl_->FireEvent(event, event_signal);
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

ElementInterface *View::AppendElement(const char *tag_name,
                                      const char *name) {
  return impl_->AppendElement(tag_name, name);
}

ElementInterface *View::InsertElement(const char *tag_name,
                                      const ElementInterface *before,
                                      const char *name) {
  return impl_->InsertElement(tag_name, before, name);
}

bool View::RemoveElement(ElementInterface *child) {
  return impl_->RemoveElement(child);
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

} // namespace ggadget
