/*
  Copyright 2008 Google Inc.

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

#include "view_element.h"
#include "canvas_interface.h"
#include "elements.h"
#include "graphics_interface.h"
#include "logger.h"
#include "signals.h"
#include "view.h"
#include "view_host_interface.h"

namespace ggadget {

class ViewElement::Impl {
 public:
  Impl(ViewElement *owner, View *child_view)
    : owner_(owner), child_view_(child_view), scale_(1.), offset_x_(0) {
    ConnectSlots();
    DLOG("ViewElement Ctor: %p, size: %f x %f", child_view,
         child_view->GetWidth(), child_view->GetHeight());
  }

  ~Impl() {
    DisconnectSlots();
  }

  void ConnectSlots() {
    ASSERT(child_view_);
  }

  void DisconnectSlots() {
    // Clear all connections in View on destruction.
    for (std::vector<Connection *>::iterator iter = connections_.begin();
         connections_.end() != iter; iter++) {
      (*iter)->Disconnect();
    }
    connections_.clear();
  }

  void Scale(double width, double height) {
    double vw = child_view_->GetWidth();
    double vh = child_view_->GetHeight();
    if (!vw || !vh) return; //FIXME: a good solution is needed
    double min = std::min(width / vw, height / vh);
    if (scale_ < 1 && width / vw > scale_)
      scale_ = std::min(1.0, width / vw);
    else
      scale_ = min;
    double pw = vw * scale_;
    double ph = vh * scale_;
    offset_x_ = pw < width ? (width - pw) / 2 : 0;
    owner_->SetPixelWidth(width);
    owner_->SetPixelHeight(ph);
  }

  ViewElement *owner_;
  View *child_view_; // This view is not owned by the element.
  double scale_;
  double offset_x_;
  std::vector<Connection *> connections_;
  static const double kZoomFactor = 1.0 / 0.141421;
};

ViewElement::ViewElement(BasicElement *parent, View *parent_view,
                         View *child_view)
    // Only 1 child so no need to involve Elements here.
    : BasicElement(parent, parent_view, "", NULL, false),
      impl_(new Impl(this, child_view)) {
  DLOG("MEMORY: ViewElement Ctor %p", this);
  SetEnabled(true);
}

ViewElement::~ViewElement() {
  DLOG("MEMORY: ViewElement Dtor %p", this);
  delete impl_;
  impl_ = NULL;
}

void ViewElement::SetChildView(View *child_view) {
  impl_->DisconnectSlots();
  impl_->child_view_ = child_view;
  if (child_view) {
    impl_->ConnectSlots();
    QueueDraw();
    impl_->child_view_->GetChildren()->Layout();
  }
}

void ViewElement::Layout() {
  BasicElement::Layout();
}

void ViewElement::MarkRedraw() {
  if (impl_->child_view_) {
    impl_->child_view_->MarkRedraw();
  }
}

void ViewElement::DoDraw(CanvasInterface *canvas) {
  // DLOG("DoDraw ViewElement: %p on canvas %p", this, canvas);
  if (impl_->child_view_) {
    if (impl_->offset_x_ != 0.) {
      canvas->PushState();
      canvas->TranslateCoordinates(impl_->offset_x_, 0);
    }
    if (impl_->scale_ != 1.) {
      canvas->ScaleCoordinates(impl_->scale_, impl_->scale_);      
    }
    impl_->child_view_->Draw(canvas);
    if (impl_->offset_x_ != 0.)
      canvas->PopState();
  }
}

void ViewElement::SetScale(double scale) {
  if (scale != impl_->scale_) {
    impl_->scale_ = scale;
    QueueDraw();
  }
}

bool ViewElement::OnSizing(double *width, double *height) {
  ASSERT(width && height);
  ViewInterface::ResizableMode mode = impl_->child_view_->GetResizable();
  if (mode == ViewInterface::RESIZABLE_TRUE &&
      impl_->child_view_->OnSizing(width, height))
    return true;
  if (*width == 0 || *height == 0) return false;
  double zoom_x = impl_->child_view_->GetWidth() / *width;
  double zoom_y = impl_->child_view_->GetHeight() / *height;
  return zoom_x < Impl::kZoomFactor && zoom_y < Impl::kZoomFactor;
}

void ViewElement::SetSize(double width, double height) {
  double vw = impl_->child_view_->GetWidth();
  double vh = impl_->child_view_->GetHeight();
  if (width == 0 || height == 0 ||  // ignore zeroize request
      (width == vw && height == vh))
    return;
  double w = width, h = height;
  ViewInterface::ResizableMode mode = impl_->child_view_->GetResizable();
  if (mode == ViewInterface::RESIZABLE_TRUE) {
    if (impl_->child_view_->OnSizing(&w, &h)) {
      DLOG("resizable, request: %.1lfx%.1lf, allowed: %.1lfx%.1lf",
          width, height, w, h);
      impl_->child_view_->SetSize(w, h);
    }
  } else {
    impl_->Scale(width, height);
  }
}

void ViewElement::GetDefaultSize(double *width, double *height) const {  
  *width  = impl_->child_view_->GetWidth() * impl_->scale_;
  *height = impl_->child_view_->GetHeight() * impl_->scale_;
}

View *ViewElement::GetChildView() const {
  return impl_->child_view_;
}

EventResult ViewElement::HandleMouseEvent(const MouseEvent &event) {
  EventResult result;
  if (impl_->scale_ != 1.) {
    MouseEvent new_event(event);
    new_event.SetX(event.GetX() / impl_->scale_);
    new_event.SetY(event.GetY() / impl_->scale_);
    result = impl_->child_view_->OnMouseEvent(new_event);
  } else {
    result = impl_->child_view_->OnMouseEvent(event);
  }
  // set hittest
  SetHitTest(impl_->child_view_->GetHitTest());
  return result;
}

EventResult ViewElement::HandleDragEvent(const DragEvent &event) {
  if (impl_->scale_ != 1.) {
    DragEvent new_event(event);
    new_event.SetX(event.GetX() / impl_->scale_);
    new_event.SetY(event.GetY() / impl_->scale_);
    return impl_->child_view_->OnDragEvent(new_event);
  } else {
    return impl_->child_view_->OnDragEvent(event);
  }
}

EventResult ViewElement::HandleKeyEvent(const KeyboardEvent &event) {
  return impl_->child_view_->OnKeyEvent(event);
}

EventResult ViewElement::HandleOtherEvent(const Event &event) {
  return impl_->child_view_->OnOtherEvent(event);
}

} // namespace ggadget
