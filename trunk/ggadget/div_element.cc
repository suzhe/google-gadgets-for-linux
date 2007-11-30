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
#include "div_element.h"
#include "canvas_interface.h"
#include "elements.h"
#include "math_utils.h"
#include "scrollbar_element.h"
#include "string_utils.h"
#include "texture.h"
#include "view.h"

namespace ggadget {

class DivElement::Impl {
 public:
  Impl(DivElement *owner)
      : owner_(owner),
        background_texture_(NULL),
        scroll_pos_x_(0), scroll_pos_y_(0),
        scroll_width_(0), scroll_height_(0),
        scroll_range_x_(0), scroll_range_y_(0), 
        scrollbar_(NULL) {
  }
  ~Impl() {
    delete background_texture_;
    background_texture_ = NULL;

    if (scrollbar_) {
      owner_->GetView()->OnElementRemove(scrollbar_);
      delete scrollbar_;
      scrollbar_ = NULL;
    }
  }

  void ScrollBarUpdated() {
    int v = scrollbar_->GetValue();
    if (scroll_pos_y_ != v) {
      // No need to queue draw here since the scrollbar should have already 
      // queued it.
      scroll_pos_y_ = v;
    }
  }

  void CreateScrollBar() {
    if (!scrollbar_) {
      scrollbar_ = new ScrollBarElement(owner_, owner_->GetView(), "");
      scrollbar_->SetPixelHeight(owner_->GetPixelHeight());
      scrollbar_->SetPixelWidth(12); // width of default images
      scrollbar_->SetEnabled(true);
      scrollbar_->SetPixelX(owner_->GetPixelWidth() -  
                            scrollbar_->GetPixelWidth());
      scrollbar_->SetMax(scroll_range_y_);
      scrollbar_->SetValue(scroll_pos_y_);
      Slot0<void> *slot = NewSlot(this, &Impl::ScrollBarUpdated);
      scrollbar_->ConnectOnChangeEvent(slot);
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
    double max_range_x = scroll_width_ - owner_width;
    scroll_range_x_ = std::max(0, static_cast<int>(ceil(max_range_x)));
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

  EventResult HandleKeyEvent(const KeyboardEvent &event) {
    EventResult result = EVENT_RESULT_UNHANDLED;
    if (scrollbar_ && event.GetType() == Event::EVENT_KEY_DOWN) {
      result = EVENT_RESULT_HANDLED;
      switch (event.GetKeyCode()) {
        case KeyboardEvent::KEY_UP:
          ScrollY(-kLineHeight);
          break;
        case KeyboardEvent::KEY_DOWN:
          ScrollY(kLineHeight);
          break;
        case KeyboardEvent::KEY_LEFT:
          ScrollX(-kLineWidth);
          break;
        case KeyboardEvent::KEY_RIGHT:
          ScrollX(kLineWidth);
          break;
        case KeyboardEvent::KEY_PAGE_UP:
          ScrollY(-static_cast<int>(ceil(owner_->GetPixelHeight())));
          break;
        case KeyboardEvent::KEY_PAGE_DOWN:
          ScrollY(static_cast<int>(ceil(owner_->GetPixelHeight())));
          break;
        default:
          result = EVENT_RESULT_UNHANDLED;
          break;
      }
    }
    return result;
  }

  static const int kLineHeight = 5;
  static const int kLineWidth = 5;

  DivElement *owner_;
  Texture *background_texture_;
  int scroll_pos_x_, scroll_pos_y_;
  int scroll_width_, scroll_height_;
  int scroll_range_x_, scroll_range_y_;
  ScrollBarElement *scrollbar_; // != NULL if and only if autoscroll=true
};

DivElement::DivElement(BasicElement *parent, View *view, const char *name)
    : BasicElement(parent, view, "div", name, true),
      impl_(new Impl(this)) {
  RegisterProperty("autoscroll",
                   NewSlot(this, &DivElement::IsAutoscroll),
                   NewSlot(this, &DivElement::SetAutoscroll));
  RegisterProperty("background",
                   NewSlot(this, &DivElement::GetBackground),
                   NewSlot(this, &DivElement::SetBackground));
}

DivElement::DivElement(BasicElement *parent, View *view,
                       const char *tag_name, const char *name)
    : BasicElement(parent, view, tag_name, name, true),
      impl_(new Impl(this)) {
}

DivElement::~DivElement() {
  delete impl_;
  impl_ = NULL;
}

void DivElement::DoDraw(CanvasInterface *canvas,
                        const CanvasInterface *children_canvas) {
  if (impl_->background_texture_)
    impl_->background_texture_->Draw(canvas);

  if (children_canvas) {
    if (impl_->scrollbar_) {
      impl_->UpdateScrollPos(children_canvas->GetWidth(),
                             children_canvas->GetHeight());
      canvas->DrawCanvas(-impl_->scroll_pos_x_, -impl_->scroll_pos_y_,
                         children_canvas);
    } else {
      canvas->DrawCanvas(0, 0, children_canvas);
    }
  }

  if (impl_->scrollbar_) {
    bool c;
    const CanvasInterface *scrollbar = impl_->scrollbar_->Draw(&c);
    canvas->DrawCanvas(impl_->scrollbar_->GetPixelX(),
                       impl_->scrollbar_->GetPixelY(),
                       scrollbar);
  }
}

Variant DivElement::GetBackground() const {
  return Variant(Texture::GetSrc(impl_->background_texture_));
}

void DivElement::SetBackground(const Variant &background) {
  delete impl_->background_texture_;
  impl_->background_texture_ = GetView()->LoadTexture(background);
  QueueDraw();
}

bool DivElement::IsAutoscroll() const {
  return impl_->scrollbar_ != NULL;
}

void DivElement::SetAutoscroll(bool autoscroll) {
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

BasicElement *DivElement::CreateInstance(BasicElement *parent, View *view,
                                         const char *name) {
  return new DivElement(parent, view, name);
}

EventResult DivElement::OnMouseEvent(const MouseEvent &event, bool direct,
                                     BasicElement **fired_element,
                                     BasicElement **in_element) {
  if (!direct && impl_->scrollbar_) {
    double new_x = event.GetX() - impl_->scrollbar_->GetPixelX();
    double new_y = event.GetY() - impl_->scrollbar_->GetPixelY();
    if (impl_->scrollbar_->IsVisible() &&
        IsPointInElement(new_x, new_y, impl_->scrollbar_->GetPixelWidth(),
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

EventResult DivElement::HandleMouseEvent(const MouseEvent &event) {
  if (impl_->scrollbar_ && impl_->scrollbar_->IsVisible() &&
      event.GetType() == Event::EVENT_MOUSE_WHEEL) {
    return impl_->scrollbar_->HandleMouseEvent(event);
  }
  return EVENT_RESULT_UNHANDLED;
}

EventResult DivElement::HandleKeyEvent(const KeyboardEvent &event) {
  return impl_->HandleKeyEvent(event);
}

void DivElement::SelfCoordToChildCoord(BasicElement *child,
                                       double x, double y,
                                       double *child_x, double *child_y) {  
  if (impl_->scrollbar_ && impl_->scrollbar_->IsVisible() && 
      IsPointInElement(x, y, impl_->scrollbar_->GetPixelWidth(),
                       impl_->scrollbar_->GetPixelHeight())) {
    x -= impl_->scroll_pos_x_;
    y -= impl_->scroll_pos_y_;
  }

  BasicElement::SelfCoordToChildCoord(child, x, y, child_x, child_y);
}

void DivElement::OnWidthChange() {
  if (impl_->scrollbar_) {
    impl_->scrollbar_->SetPixelX(GetPixelWidth() -
                                 impl_->scrollbar_->GetPixelWidth());
  }
}

void DivElement::OnHeightChange() {
  if (impl_->scrollbar_) {
    impl_->scrollbar_->SetPixelHeight(GetPixelHeight());
  }
}

} // namespace ggadget
