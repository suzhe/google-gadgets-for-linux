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

#include <cmath>
#include "scrolling_element.h"
#include "canvas_interface.h"
#include "elements.h"
#include "math_utils.h"
#include "scrollbar_element.h"
#include "view.h"

namespace ggadget {

class ScrollingElement::Impl {
 public:
  Impl(ScrollingElement *owner)
      : owner_(owner),
        scroll_pos_x_(0), scroll_pos_y_(0),
        scroll_width_(0), scroll_height_(0),
        scroll_range_x_(0), scroll_range_y_(0), 
        scrollbar_(NULL) {
  }
  ~Impl() {
    if (scrollbar_) {
      delete scrollbar_;
      scrollbar_ = NULL;
    }
  }

  void CreateScrollBar() {
    if (!scrollbar_) {
      scrollbar_ = new ScrollBarElement(owner_, owner_->GetView(), "");
      scrollbar_->SetImplicit(true);
      scrollbar_->SetPixelHeight(owner_->GetPixelHeight());
      scrollbar_->SetPixelWidth(12); // width of default images
      scrollbar_->SetEnabled(true);
      scrollbar_->SetPixelX(owner_->GetPixelWidth() -
                            scrollbar_->GetPixelWidth());
      scrollbar_->SetMax(scroll_range_y_);
      scrollbar_->SetValue(scroll_pos_y_);
      // Inform the view of this scrollbar to let the view handle mouse grabbing
      // and mouse over/out logics of the scrollbar.
      owner_->GetView()->OnElementAdd(scrollbar_);
    }
  }

  void UpdateScrollPos(size_t width, size_t height) {
    ASSERT(scrollbar_);
    scroll_height_ = static_cast<int>(height);
    int owner_height = static_cast<int>(ceil(owner_->GetPixelHeight()));
    int old_range_y = scroll_range_y_;
    scroll_range_y_ = std::max(0, scroll_height_ - owner_height);
    scroll_pos_y_ = std::min(scroll_pos_y_, scroll_range_y_);
    bool show_scrollbar = scrollbar_ && (scroll_range_y_ > 0);
    if (old_range_y != scroll_range_y_) {
      scrollbar_->SetMax(scroll_range_y_);
      scrollbar_->SetValue(scroll_pos_y_);
    }
    scrollbar_->SetVisible(show_scrollbar);

    scroll_width_ = static_cast<int>(width);
    double owner_width = owner_->GetPixelWidth();
    if (show_scrollbar) {
      owner_width -= scrollbar_->GetPixelWidth();
    }
    int max_range_x = scroll_width_ - static_cast<int>(ceil(owner_width));
    scroll_range_x_ = std::max(0, max_range_x);
    scroll_pos_x_ = std::min(scroll_pos_x_, scroll_range_x_);
  }

  void ScrollX(int distance) {
    int old_pos = scroll_pos_x_;
    scroll_pos_x_ += distance;
    scroll_pos_x_ = std::min(scroll_range_x_, std::max(0, scroll_pos_x_));
    if (old_pos != scroll_pos_x_) {
      owner_->QueueDraw();
    }
  }

  void ScrollY(int distance) {
    int old_pos = scroll_pos_y_;
    scroll_pos_y_ += distance;
    scroll_pos_y_ = std::min(scroll_range_y_, std::max(0, scroll_pos_y_));
    if (old_pos != scroll_pos_y_) {
      scrollbar_->SetValue(scroll_pos_y_); // SetValue calls QueueDraw
    }
  }

  ScrollingElement *owner_;
  Texture *background_texture_;
  BasicElement *grabbed_scrollbar_, *mouseover_scrollbar_; 
  int scroll_pos_x_, scroll_pos_y_;
  int scroll_width_, scroll_height_;
  int scroll_range_x_, scroll_range_y_;
  ScrollBarElement *scrollbar_; // NULL if and only if autoscroll=false
};

ScrollingElement::ScrollingElement(BasicElement *parent, View *view,
                                   const char *tag_name, const char *name,
                                   Elements *children)
    : BasicElement(parent, view, tag_name, name, children),
      impl_(new Impl(this)) {
}

ScrollingElement::~ScrollingElement() {
  delete impl_;
  impl_ = NULL;
}

bool ScrollingElement::IsAutoscroll() const {
  return impl_->scrollbar_ != NULL;
}

void ScrollingElement::SetAutoscroll(bool autoscroll) {
  if ((impl_->scrollbar_ != NULL) != autoscroll) {
    if (autoscroll) {
      impl_->CreateScrollBar();
    }
    else {
      delete impl_->scrollbar_;
      impl_->scrollbar_ = NULL;
    }
    SetChildrenScrollable(autoscroll);
    QueueDraw();
  }
}

void ScrollingElement::ScrollX(int distance) {
  if (impl_->scrollbar_) {
    impl_->ScrollX(distance);
  }
}

void ScrollingElement::ScrollY(int distance) {
  if (impl_->scrollbar_) {
    impl_->ScrollY(distance);
  }
}

int ScrollingElement::GetScrollXPosition() const {
  return impl_->scroll_pos_x_;
}

int ScrollingElement::GetScrollYPosition() const {
  return impl_->scroll_pos_y_;
}

void ScrollingElement::SetScrollXPosition(int pos) {
  if (impl_->scrollbar_) {
    impl_->ScrollX(pos - impl_->scroll_pos_x_);
  }
}

void ScrollingElement::SetScrollYPosition(int pos) {
  if (impl_->scrollbar_) {
    impl_->ScrollY(pos - impl_->scroll_pos_y_);
  }
}

EventResult ScrollingElement::OnMouseEvent(const MouseEvent &event, bool direct,
                                           BasicElement **fired_element,
                                           BasicElement **in_element) {
  if (!direct && impl_->scrollbar_ && impl_->scrollbar_->IsVisible()) {
    double new_x = event.GetX() - impl_->scrollbar_->GetPixelX();
    double new_y = event.GetY() - impl_->scrollbar_->GetPixelY();
    if (IsPointInElement(new_x, new_y, impl_->scrollbar_->GetPixelWidth(),
                         impl_->scrollbar_->GetPixelHeight())) {
      MouseEvent new_event(event);
      new_event.SetX(new_x);
      new_event.SetY(new_y);
      return impl_->scrollbar_->OnMouseEvent(new_event, direct,
                                             fired_element, in_element);
    }
  }

  return BasicElement::OnMouseEvent(event, direct, fired_element, in_element);
}

EventResult ScrollingElement::HandleMouseEvent(const MouseEvent &event) {
  if (impl_->scrollbar_ && impl_->scrollbar_->IsVisible()) {
    if (event.GetType() == Event::EVENT_MOUSE_WHEEL) {
      return impl_->scrollbar_->HandleMouseEvent(event);
    }
  }
  return EVENT_RESULT_UNHANDLED;
}

void ScrollingElement::SelfCoordToChildCoord(const BasicElement *child,
                                             double x, double y,
                                             double *child_x,
                                             double *child_y) const {  
  if (child != impl_->scrollbar_ && impl_->scrollbar_) {
    x += impl_->scroll_pos_x_;
    y += impl_->scroll_pos_y_;
  }

  BasicElement::SelfCoordToChildCoord(child, x, y, child_x, child_y);
}

void ScrollingElement::DrawScrollbar(CanvasInterface *canvas) {
  if (impl_->scrollbar_ && impl_->scrollbar_->IsVisible()) {
    bool c;
    const CanvasInterface *scrollbar = impl_->scrollbar_->Draw(&c);
    canvas->DrawCanvas(impl_->scrollbar_->GetPixelX(),
                       impl_->scrollbar_->GetPixelY(),
                       scrollbar);
  }
}

bool ScrollingElement::UpdateScrollBar(int content_width, int content_height) {

  if (impl_->scrollbar_) {
    bool scrollbar_visible = impl_->scrollbar_->IsVisible();
    impl_->scrollbar_->SetPixelX(GetPixelWidth() -
                                 impl_->scrollbar_->GetPixelWidth());
    impl_->scrollbar_->SetPixelHeight(GetPixelHeight());

    int v = impl_->scrollbar_->GetValue();
    if (impl_->scroll_pos_y_ != v) {
      impl_->scroll_pos_y_ = v;
    }

    impl_->UpdateScrollPos(content_width, content_height);
    return scrollbar_visible != impl_->scrollbar_->IsVisible();
  }
  return false;
}

double ScrollingElement::GetClientWidth() {
  return impl_->scrollbar_ && impl_->scrollbar_->IsVisible() ?
         std::max(GetPixelWidth() - impl_->scrollbar_->GetPixelWidth(), 0.0) :
         GetPixelWidth();
}

double ScrollingElement::GetClientHeight() {
  // Horizontal scrollbar is not supported for now.
  return GetPixelHeight();
}

} // namespace ggadget
