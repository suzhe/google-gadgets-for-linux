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
#include "logger.h"
#include "signals.h"
#include "view.h"

namespace ggadget {

class ViewElement::Impl {
 public:
  Impl(ViewElement *owner, View *child_view)
    : owner_(owner), child_view_(child_view) {
    // Connect slots
    Connection *c = child_view_->ConnectOnSizeEvent(NewSlot(this, &Impl::ViewOnSize));
    connections_.push_back(c);
  }

  ~Impl() {
    // Clear all connections in View on destruction.
    for (std::vector<Connection *>::iterator iter = connections_.begin();
         connections_.end() != iter; iter++) {
      (*iter)->Disconnect();      
    }
    connections_.clear();
  }

  void ViewOnSize() {
    owner_->SetPixelWidth(child_view_->GetWidth());
    owner_->SetPixelHeight(child_view_->GetHeight());
  }

  ViewElement *owner_;
  View *child_view_; // This view is not owned by the element.
  std::vector<Connection *> connections_;
};

ViewElement::ViewElement(BasicElement *parent, View *child_view, 
                         View *parent_view)
    // Only 1 child so no need to involve Elements here.
    : BasicElement(parent, parent_view, "", NULL, false), 
      impl_(new Impl(this, child_view)) {
  SetEnabled(true);
}

ViewElement::~ViewElement() {
  delete impl_;
  impl_ = NULL;
}

void ViewElement::Layout() {
  int vw = impl_->child_view_->GetWidth();
  int vh = impl_->child_view_->GetHeight();
  int pw = static_cast<int>(GetPixelWidth());
  int ph = static_cast<int>(GetPixelHeight());
  if (vw != pw || vh != ph) {
    // No need to perform layout on View manually, just update size.
    impl_->child_view_->SetSize(pw, ph);    
  }
}

void ViewElement::MarkRedraw() {
  impl_->child_view_->MarkRedraw();  
}

void ViewElement::DoDraw(CanvasInterface *canvas) {
  impl_->child_view_->Draw(canvas);
}

void ViewElement::GetDefaultSize(double *width, double *height) const {
  *width  = impl_->child_view_->GetWidth();
  *height = impl_->child_view_->GetHeight();
}

EventResult ViewElement::HandleMouseEvent(const MouseEvent &event) {
  return impl_->child_view_->OnMouseEvent(event);
}

EventResult ViewElement::HandleDragEvent(const DragEvent &event) {
  return impl_->child_view_->OnDragEvent(event);
}

EventResult ViewElement::HandleKeyEvent(const KeyboardEvent &event) {
  return impl_->child_view_->OnKeyEvent(event);
}

EventResult ViewElement::HandleOtherEvent(const Event &event) {
  return impl_->child_view_->OnOtherEvent(event);
}

} // namespace ggadget
