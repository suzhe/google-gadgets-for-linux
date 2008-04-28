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
  Impl(ViewElement *owner)
    : owner_(owner),
      child_view_(NULL),
      scale_(1),
      onsize_connection_(NULL),
      onopen_connection_(NULL) {
  }

  ~Impl() {
    if (onsize_connection_)
      onsize_connection_->Disconnect();
    if (onopen_connection_)
      onopen_connection_->Disconnect();
  }

  void UpdateScaleAndSize() {
    if (child_view_) {
      scale_ = child_view_->GetGraphics()->GetZoom() /
               owner_->GetView()->GetGraphics()->GetZoom();
    } else {
      scale_ = 1.0;
    }

    double width = owner_->GetPixelWidth();
    double height = owner_->GetPixelHeight();

    owner_->BasicElement::SetPixelWidth(width);
    owner_->BasicElement::SetPixelHeight(height);

    DLOG("ViewElement new scale: %lf, new size: %lf, %lf",
         scale_, width, height);
  }

  ViewElement *owner_;
  View *child_view_; // This view is not owned by the element.
  double scale_;
  Connection *onsize_connection_;
  Connection *onopen_connection_;
};

ViewElement::ViewElement(BasicElement *parent, View *parent_view,
                         View *child_view)
    // Only 1 child so no need to involve Elements here.
    : BasicElement(parent, parent_view, "", NULL, false),
      impl_(new Impl(this)) {
  DLOG("MEMORY: ViewElement Ctor %p", this);
  SetEnabled(true);
  SetChildView(child_view);
}

ViewElement::~ViewElement() {
  DLOG("MEMORY: ViewElement Dtor %p", this);
  delete impl_;
  impl_ = NULL;
}

void ViewElement::SetChildView(View *child_view) {
  if (child_view == impl_->child_view_)
    return;

  if (impl_->onsize_connection_) {
    impl_->onsize_connection_->Disconnect();
    impl_->onsize_connection_ = NULL;
  }

  if (impl_->onopen_connection_) {
    impl_->onopen_connection_->Disconnect();
    impl_->onopen_connection_ = NULL;
  }

  // Hook onopen event to do the first time initialization.
  // Because when View is initialized from XML, the event is disabled, so the
  // onsize event can't be received.
  if (child_view) {
    impl_->onsize_connection_ = child_view->ConnectOnSizeEvent(
        NewSlot(impl_, &Impl::UpdateScaleAndSize));
    impl_->onopen_connection_ = child_view->ConnectOnOpenEvent(
        NewSlot(impl_, &Impl::UpdateScaleAndSize));
  }

  impl_->child_view_ = child_view;
  impl_->UpdateScaleAndSize();
  QueueDraw();
}

View *ViewElement::GetChildView() const {
  return impl_->child_view_;
}

bool ViewElement::OnSizing(double *width, double *height) {
  ASSERT(width && height);
  DLOG("ViewElement::OnSizing(%lf, %lf)", *width, *height);

  if (*width <= 0 || *height <= 0)
    return false;

  // Any size is allowed if there is no child view.
  if (!impl_->child_view_)
    return true;

  ViewInterface::ResizableMode mode = impl_->child_view_->GetResizable();
  double child_width;
  double child_height;

  // If child view is resizable then just delegate OnSizing request to child
  // view.
  // The resizable view might also be zoomed, so count the scale factor in.
  if (mode == ViewInterface::RESIZABLE_TRUE) {
    child_width = *width / impl_->scale_;
    child_height = *height / impl_->scale_;
    bool ret = impl_->child_view_->OnSizing(&child_width, &child_height);
    *width = child_width * impl_->scale_;
    *height = child_height * impl_->scale_;
    return ret;
  }

  // Otherwise adjust the width or height to maintain the aspect ratio of child
  // view.
  child_width = impl_->child_view_->GetWidth();
  child_height = impl_->child_view_->GetHeight();
  double aspect_ratio = child_width / child_height;

  // Keep the shorter edge unchanged.
  if (*width / *height < aspect_ratio) {
    *height = *width / aspect_ratio;
  } else {
    *width = *height * aspect_ratio;
  }

  return true;
}

void ViewElement::SetSize(double width, double height) {
  DLOG("ViewElement::SetSize(%lf, %lf)", width, height);
  double old_width = GetPixelWidth();
  double old_height = GetPixelHeight();

  if (width <= 0 || height <= 0) return;
  if (width == old_width && height == old_height) return;

  // If there is no child view, then just adjust BasicElement's size.
  if (!impl_->child_view_) {
    BasicElement::SetPixelWidth(width);
    BasicElement::SetPixelHeight(height);
    return;
  }

  ViewInterface::ResizableMode mode = impl_->child_view_->GetResizable();
  if (mode == ViewInterface::RESIZABLE_TRUE) {
    // The resizable view might also be zoomed, so count the scale factor in.
    impl_->child_view_->SetSize(width / impl_->scale_, height / impl_->scale_);
    impl_->UpdateScaleAndSize();
  } else {
    double child_width = impl_->child_view_->GetWidth();
    double child_height = impl_->child_view_->GetHeight();
    double aspect_ratio = child_width / child_height;

    // Calculate the scale factor according to the shorter edge.
    if (width / height < aspect_ratio)
      SetScale(width / child_width);
    else
      SetScale(height / child_height);
  }

  QueueDraw();
}

void ViewElement::SetScale(double scale) {
  // Only set scale if child view is available.
  if (impl_->child_view_ && scale > 0 && scale != impl_->scale_) {
    double new_zoom = GetView()->GetGraphics()->GetZoom() * scale;
    impl_->child_view_->GetGraphics()->SetZoom(new_zoom);
    impl_->child_view_->MarkRedraw();
    impl_->UpdateScaleAndSize();
    QueueDraw();
  }
}

double ViewElement::GetScale() const {
  return impl_->scale_;
}

double ViewElement::GetPixelWidth() const {
  if (impl_->child_view_)
    return impl_->child_view_->GetWidth() * impl_->scale_;

  return BasicElement::GetPixelWidth();
}

double ViewElement::GetPixelHeight() const {
  if (impl_->child_view_)
    return impl_->child_view_->GetHeight() * impl_->scale_;

  return BasicElement::GetPixelHeight();
}

void ViewElement::MarkRedraw() {
  BasicElement::MarkRedraw();
  if (impl_->child_view_)
    impl_->child_view_->MarkRedraw();
}

void ViewElement::DoDraw(CanvasInterface *canvas) {
  if (impl_->child_view_) {
    if (impl_->scale_ != 1)
      canvas->ScaleCoordinates(impl_->scale_, impl_->scale_);
    impl_->child_view_->Draw(canvas);
  }
}

EventResult ViewElement::HandleMouseEvent(const MouseEvent &event) {
  EventResult result = EVENT_RESULT_UNHANDLED;
  // The view containing this ViewElement translates EVENT_MOUSE_OVER as
  // EVENT_MOUSE_MOVE, and then send EVENT_MOUSE_OVER to this ViewElement,
  // so don't pass this EVENT_MOUSE_OVER to the child_view because it has
  // already processed mouse over logic on the last EVENT_MOUSE_MOVE.
  if (event.GetType() != Event::EVENT_MOUSE_OVER) {
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
  }
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

void ViewElement::GetDefaultSize(double *width, double *height) const {
  if (impl_->child_view_) {
    *width  = impl_->child_view_->GetWidth() * impl_->scale_;
    *height = impl_->child_view_->GetHeight() * impl_->scale_;
  } else {
    BasicElement::GetDefaultSize(width, height);
  }
}

} // namespace ggadget
