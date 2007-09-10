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
#include "graphics_interface.h"
#include "event.h"

namespace ggadget {

using internal::ViewImpl;

View::View(int width, int height) : impl_(new ViewImpl(width, height)) {
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

void View::OnMouseDown(const MouseEvent *event) {
  impl_->OnMouseDown(event);
}

void View::OnMouseUp(const MouseEvent *event) {
  impl_->OnMouseUp(event);
}

void View::OnMouseClick(const MouseEvent *event) {
  impl_->OnMouseClick(event);
} 

void View::OnMouseDblClick(const MouseEvent *event) {
  impl_->OnMouseDblClick(event);
}

void View::OnMouseMove(const MouseEvent *event) {
  impl_->OnMouseMove(event);
}

void View::OnMouseOut(const MouseEvent *event) {
  impl_->OnMouseOut(event);
}

void View::OnMouseOver(const MouseEvent *event) {
  impl_->OnMouseOver(event);
}

void View::OnMouseWheel(const MouseEvent *event) {
  impl_->OnMouseWheel(event);
}

void View::OnKeyDown(const KeyboardEvent *event) {
  impl_->OnKeyDown(event);
}

void View::OnKeyUp(const KeyboardEvent *event) {
  impl_->OnKeyUp(event);
}

void View::OnKeyPress(const KeyboardEvent *event) {
  impl_->OnKeyPress(event);
}

void View::OnFocusIn(const Event *event) {
  impl_->OnFocusIn(event);
}

void View::OnFocusOut(const Event *event) {
  impl_->OnFocusOut(event);
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

namespace internal {

void ViewImpl::OnMouseDown(const MouseEvent *event) {
  DLOG("mousedown");
  // TODO: generate script event
}

void ViewImpl::OnMouseUp(const MouseEvent *event) {
  DLOG("mouseup");
  // TODO: generate script event
}

void ViewImpl::OnMouseClick(const MouseEvent *event) {
  DLOG("mouseclick %g %g", event->GetX(), event->GetY());
  // TODO: generate script event
} 

void ViewImpl::OnMouseDblClick(const MouseEvent *event) {
  DLOG("mousedblclick %g %g", event->GetX(), event->GetY());
  // TODO: generate script event
}

void ViewImpl::OnMouseMove(const MouseEvent *event) {
  //DLOG("mousemove");
  // TODO: generate script event
}

void ViewImpl::OnMouseOut(const MouseEvent *event) {
  DLOG("mouseout");
  // TODO: generate script event
}

void ViewImpl::OnMouseOver(const MouseEvent *event) {
  DLOG("mouseover");
  // TODO: generate script event
}

void ViewImpl::OnMouseWheel(const MouseEvent *event) {
  DLOG("mousewheel");
  // TODO: generate script event
}

void ViewImpl::OnKeyDown(const KeyboardEvent *event) {
  DLOG("keydown");
  // TODO: generate script event
}

void ViewImpl::OnKeyUp(const KeyboardEvent *event) {
  DLOG("keyup");
  // TODO: generate script event
}

void ViewImpl::OnKeyPress(const KeyboardEvent *event) {
  DLOG("keypress");
  // TODO: generate script event
}

void ViewImpl::OnFocusIn(const Event *event) {
  DLOG("focusin");
  // TODO: generate script event
}

void ViewImpl::OnFocusOut(const Event *event) {
  DLOG("focusout");
  // TODO: generate script event
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
  // TODO: generate script event
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
  // TODO: generate script event
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
  // TODO: generate script event
  return true;
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

ViewImpl::ViewImpl(int width, int height) 
    : width_(width), height_(height), host_(NULL), canvas_(NULL) {
  RegisterSignal(kOnCancelEvent, &oncancle_event_);
  RegisterSignal(kOnClickEvent, &onclick_event_);
  RegisterSignal(kOnCloseEvent, &onclose_event_);
  RegisterSignal(kOnDblClickEvent, &ondblclick_event_);
  RegisterSignal(kOnDockEvent, &ondock_event_);
  RegisterSignal(kOnKeyDownEvent, &onkeydown_event_);
  RegisterSignal(kOnKeyPressEvent, &onkeypress_event_);
  RegisterSignal(kOnKeyReleaseEvent, &onkeyrelease_event_);
  RegisterSignal(kOnMinimizeEvent, &onminimize_event_);
  RegisterSignal(kOnMouseDownEvent, &onmousedown_event_);
  RegisterSignal(kOnMouseOutEvent, &onmouseout_event_);
  RegisterSignal(kOnMouseOverEvent, &onmouseover_event_);
  RegisterSignal(kOnMouseUpEvent, &onmouseup_event_);
  RegisterSignal(kOnOkEvent, &onok_event_);
  RegisterSignal(kOnOpenEvent, &onopen_event_);
  RegisterSignal(kOnOptionChangedEvent, &onoptionchanged_event_);
  RegisterSignal(kOnPopInEvent, &onpopin_event_);
  RegisterSignal(kOnPopOutEvent, &onpopout_event_);
  RegisterSignal(kOnRestoreEvent, &onrestore_event_);
  RegisterSignal(kOnSizeEvent, &onsize_event_);
  RegisterSignal(kOnSizingEvent, &onsizing_event_);
  RegisterSignal(kOnUndockEvent, &onundock_event_);
}

ViewImpl::~ViewImpl() {
  if (canvas_) {
    canvas_->Destroy();
    canvas_ = NULL;
  }
}

} // namespace internal

} // namespace ggadget
