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
#include "graphics_interface.h"
#include "logger.h"
#include "signals.h"
#include "view.h"
#include "view_host_interface.h"

namespace ggadget {

class ViewElement::Impl {
 public:
  Impl(ViewElement *owner, View *parent_view, 
      ViewHostInterface *child_view_host, View *child_view)
    : owner_(owner), child_view_host_(child_view_host), 
    child_view_(child_view), scale_(1.) {
    ConnectSlots();
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

  ViewElement *owner_;
  ViewHostInterface *child_view_host_;
  View *child_view_; // This view is not owned by the element.
  double scale_;
  std::vector<Connection *> connections_;
};

ViewElement::ViewElement(BasicElement *parent, View *parent_view, 
                         ViewHostInterface *child_view_host,
                         View *child_view)
    // Only 1 child so no need to involve Elements here.
    : BasicElement(parent, parent_view, "", NULL, false), 
      impl_(new Impl(this, parent_view, child_view_host, child_view)) {
  SetEnabled(true);
}

ViewElement::~ViewElement() {
  delete impl_;
  impl_ = NULL;
}

void ViewElement::SetChildView(ViewHostInterface *child_view_host, 
                               View *child_view) {  
  impl_->DisconnectSlots(); 
  impl_->child_view_host_ = child_view_host;
  impl_->child_view_ = child_view;
  if (child_view && child_view_host) {
    impl_->ConnectSlots();
    QueueDraw();  
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
  if (impl_->child_view_) {
    if (impl_->scale_ != 1.) {
      canvas->ScaleCoordinates(impl_->scale_, impl_->scale_);      
    }

    impl_->child_view_->Draw(canvas);
  }
}

void ViewElement::SetScale(double scale) {
  if (scale != impl_->scale_) {
    impl_->scale_ = scale;
    QueueDraw();
  }
}

void ViewElement::GetDefaultSize(double *width, double *height) const {  
  *width  = impl_->child_view_->GetWidth() * impl_->scale_;
  *height = impl_->child_view_->GetHeight() * impl_->scale_;
}

EventResult ViewElement::HandleMouseEvent(const MouseEvent &event) {
  if (impl_->scale_ != 1.) {
    MouseEvent new_event(event);
    new_event.SetX(event.GetX() / impl_->scale_);
    new_event.SetY(event.GetY() / impl_->scale_);

    return impl_->child_view_->OnMouseEvent(new_event);
  } else {

    return impl_->child_view_->OnMouseEvent(event);
  }
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
