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
#include "event.h"
#include "string_utils.h"
#include "texture.h"
#include "math_utils.h"
#include "view_interface.h"
#include "scrollbar_element.h"

namespace ggadget {

class DivElement::Impl {
 public:
  Impl(DivElement *owner, ViewInterface *view)
      : owner_(owner),
        background_texture_(NULL),
        autoscroll_(false),
        scroll_pos_x_(0), scroll_pos_y_(0),
        scroll_width_(0), scroll_height_(0),
        scroll_range_x_(0), scroll_range_y_(0), 
        scrollbar_(NULL) {
  }
  ~Impl() {
    delete background_texture_;
    background_texture_ = NULL;
    
    delete scrollbar_;
    scrollbar_ = NULL;
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
      scrollbar_->SetPixelHeight(owner_->GetPixelHeight());    DLOG("a height %f", scrollbar_->GetPixelHeight());
      scrollbar_->SetPixelWidth(12); // width of default images
      scrollbar_->SetEnabled(true);
      scrollbar_->SetPixelX(owner_->GetPixelWidth() -  
                            scrollbar_->GetPixelWidth());
      scrollbar_->SetMax(scroll_range_y_);
      scrollbar_->SetValue(scroll_pos_y_);
      Slot0<void> *slot = NewSlot(this, &Impl::ScrollBarUpdated);
      scrollbar_->ConnectOnChangeEvent(slot);
    }
  }
  
  void UpdateScrollPos(size_t width, size_t height) {
    scroll_height_ = static_cast<int>(height);
    int owner_height = static_cast<int>(ceil(owner_->GetPixelHeight()));
    int old_range_y = scroll_range_y_;
    scroll_range_y_ = std::max(0, scroll_height_ - owner_height);
    scroll_pos_y_ = std::min(scroll_pos_y_, scroll_range_y_);
    bool show_scrollbar = autoscroll_ && (scroll_range_y_ > 0);
    if (scrollbar_ && old_range_y != scroll_range_y_) {
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

  bool OnKeyEvent(KeyboardEvent *event) {
    bool result = true;
    if (autoscroll_ && event->GetType() == Event::EVENT_KEY_DOWN) {
      result = false;
      switch (event->GetKeyCode()) {
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
          result = true;
          break;
      }
    }
    return result;
  }

  static const int kLineHeight = 5;
  static const int kLineWidth = 5;

  DivElement *owner_;
  std::string background_;
  Texture *background_texture_;
  bool autoscroll_;
  int scroll_pos_x_, scroll_pos_y_;
  int scroll_width_, scroll_height_;
  int scroll_range_x_, scroll_range_y_;
  ScrollBarElement *scrollbar_;
};

DivElement::DivElement(ElementInterface *parent,
                       ViewInterface *view,
                       const char *name)
    : BasicElement(parent, view, "div", name, true),
      impl_(new Impl(this, view)) {
  RegisterProperty("autoscroll",
                   NewSlot(this, &DivElement::IsAutoscroll),
                   NewSlot(this, &DivElement::SetAutoscroll));
  RegisterProperty("background",
                   NewSlot(this, &DivElement::GetBackground),
                   NewSlot(this, &DivElement::SetBackground));
}

DivElement::~DivElement() {
  delete impl_;
}

void DivElement::DoDraw(CanvasInterface *canvas,
                        const CanvasInterface *children_canvas) {
  if (impl_->background_texture_)
    impl_->background_texture_->Draw(canvas);

  if (children_canvas) {
    if (impl_->autoscroll_) {
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

const char *DivElement::GetBackground() const {
  return impl_->background_.c_str();
}

void DivElement::SetBackground(const char *background) {
  if (AssignIfDiffer(background, &impl_->background_)) {
    delete impl_->background_texture_;
    impl_->background_texture_ = GetView()->LoadTexture(background);
    QueueDraw();
  }
}

bool DivElement::IsAutoscroll() const {
  return impl_->autoscroll_;
}

void DivElement::SetAutoscroll(bool autoscroll) {
  if (impl_->autoscroll_ != autoscroll) {
    impl_->autoscroll_ = autoscroll;
    if (autoscroll) {
      impl_->CreateScrollBar();
    }
    else {
      delete impl_->scrollbar_;
      impl_->scrollbar_ = NULL;
    }
    GetChildren()->SetScrollable(autoscroll);
    QueueDraw();
  }
}

ElementInterface *DivElement::CreateInstance(ElementInterface *parent,
                                             ViewInterface *view,
                                             const char *name) {
  return new DivElement(parent, view, name);
}

bool DivElement::OnMouseEvent(MouseEvent *event, bool direct,
                              ElementInterface **fired_element) {
  bool result;  
  if (impl_->scrollbar_ && IsEnabled()) {
    double new_x = event->GetX() - impl_->scrollbar_->GetPixelX();
    double new_y = event->GetY() - impl_->scrollbar_->GetPixelY();
    if (impl_->scrollbar_->IsVisible() && 
        IsPointInElement(new_x, new_y, impl_->scrollbar_->GetPixelWidth(),
                         impl_->scrollbar_->GetPixelHeight())) {
      MouseEvent new_event(*event);
      new_event.SetX(new_x);
      new_event.SetY(new_y);
      result = impl_->scrollbar_->OnMouseEvent(&new_event, direct, fired_element);
      return result;
    }
  }

  result = BasicElement::OnMouseEvent(event, direct, fired_element);

  // Handle mouse event only if the event has been fired and not canceled.
  if (*fired_element && result) {
    if (impl_->autoscroll_ && event->GetType() == Event::EVENT_MOUSE_WHEEL) {
      // TODO:
    }
  }
  return result;
}

bool DivElement::OnKeyEvent(KeyboardEvent *event) {
  return BasicElement::OnKeyEvent(event) && impl_->OnKeyEvent(event);
}

void DivElement::SelfCoordToChildCoord(ElementInterface *child,
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

void DivElement::SetPixelWidth(double width) {
  BasicElement::SetPixelWidth(width);
  if (impl_->scrollbar_) {
    impl_->scrollbar_->SetPixelX(width - impl_->scrollbar_->GetPixelWidth());
  }
}
  
void DivElement::SetPixelHeight(double height) {
  BasicElement::SetPixelHeight(height);
  if (impl_->scrollbar_) {
    impl_->scrollbar_->SetPixelHeight(height);
  }
}

void DivElement::SetRelativeWidth(double width) {
  BasicElement::SetRelativeWidth(width);
  if (impl_->scrollbar_) {
    impl_->scrollbar_->SetPixelX(GetPixelWidth() - 
                                 impl_->scrollbar_->GetPixelWidth());
  }
}

void DivElement::SetRelativeHeight(double height) {
  BasicElement::SetRelativeHeight(height);
  if (impl_->scrollbar_) {
    impl_->scrollbar_->SetPixelHeight(GetPixelHeight());
  }
}

void DivElement::OnParentWidthChange(double width) {
  BasicElement::OnParentWidthChange(width);
  if (impl_->scrollbar_) {
    impl_->scrollbar_->SetPixelX(width - impl_->scrollbar_->GetPixelWidth());
  }
}

void DivElement::OnParentHeightChange(double height) {
  BasicElement::OnParentHeightChange(height);
  if (impl_->scrollbar_) {
    impl_->scrollbar_->SetPixelHeight(GetPixelHeight());
  }
}

} // namespace ggadget
