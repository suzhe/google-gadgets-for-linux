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
        grabbed_scrollbar_(NULL), mouseover_scrollbar_(NULL),
        scroll_pos_x_(0), scroll_pos_y_(0),
        scroll_width_(0), scroll_height_(0),
        scroll_range_x_(0), scroll_range_y_(0), 
        scrollbar_(NULL) {
  }
  ~Impl() {
    delete background_texture_;
    background_texture_ = NULL;

    if (scrollbar_) {
      delete scrollbar_;
      scrollbar_ = NULL;
    }
  }

  void ScrollBarUpdated() {
    // Scroll position will be updated in subsequent Layout(). 
    owner_->QueueDraw();
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
      scrollbar_->ConnectOnRedrawEvent(slot);
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
    scroll_pos_y_ += distance;
    scroll_pos_y_ = std::min(scroll_range_y_, std::max(0, scroll_pos_y_));
    if (scrollbar_) {
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
  BasicElement *grabbed_scrollbar_, *mouseover_scrollbar_; 
  int scroll_pos_x_, scroll_pos_y_;
  int scroll_width_, scroll_height_;
  int scroll_range_x_, scroll_range_y_;
  ScrollBarElement *scrollbar_; // NULL if and only if autoscroll=false
};

DivElement::DivElement(BasicElement *parent, View *view, const char *name)
    : BasicElement(parent, view, "div", name, 
                   new Elements(view->GetElementFactory(), this, view)),
      impl_(new Impl(this)) {
  RegisterProperty("autoscroll",
                   NewSlot(this, &DivElement::IsAutoscroll),
                   NewSlot(this, &DivElement::SetAutoscroll));
  RegisterProperty("background",
                   NewSlot(this, &DivElement::GetBackground),
                   NewSlot(this, &DivElement::SetBackground));
}

DivElement::DivElement(BasicElement *parent, View *view,
                       const char *tag_name, const char *name, 
                       Elements *children)
    : BasicElement(parent, view, tag_name, name, children),
      impl_(new Impl(this)) {
}

DivElement::~DivElement() {
  delete impl_;
  impl_ = NULL;
}

void DivElement::SetScrollYPosition(int pos) {
  if (impl_->scrollbar_) {
    impl_->ScrollY(pos - impl_->scroll_pos_y_);
  }
}

void DivElement::DoDraw(CanvasInterface *canvas,
                        const CanvasInterface *children_canvas) {
  if (impl_->background_texture_)
    impl_->background_texture_->Draw(canvas);

  if (children_canvas) {
    if (impl_->scrollbar_) {
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
  if (impl_->scrollbar_ && impl_->scrollbar_->IsVisible()) {
    Event::Type t = event.GetType();
    double new_x = event.GetX() - impl_->scrollbar_->GetPixelX();
    BasicElement *new_fired = NULL, *new_in = NULL;
    EventResult r;    
    if (t == Event::EVENT_MOUSE_OUT && impl_->mouseover_scrollbar_) {
      // Case: Mouse moved out of parent and child at same time.
      // Clone mouse out event and send to child in addition to parent.
      MouseEvent new_event(event);
      new_event.SetX(new_x);
      impl_->mouseover_scrollbar_->OnMouseEvent(new_event, true,
                                                &new_fired, &new_in);
      impl_->mouseover_scrollbar_ = NULL;

      // Do not return, parent needs to handle this mouse out event too.
    } else if (impl_->grabbed_scrollbar_ && 
        (t == Event::EVENT_MOUSE_MOVE || t == Event::EVENT_MOUSE_UP)) {
      // Case: Mouse is grabbed by child. Send to child regardless of position.
      // Send to child directly.
      MouseEvent new_event(event);
      new_event.SetX(new_x);
      r = impl_->grabbed_scrollbar_->OnMouseEvent(new_event, true, 
                                                  fired_element, in_element);
      if (t == Event::EVENT_MOUSE_UP) {
        impl_->grabbed_scrollbar_ = NULL; 
      }
      if (*fired_element == impl_->scrollbar_) {
        *fired_element = this;
      }
      if (*in_element == impl_->scrollbar_) {
        *in_element = this;
      }
      return r;
    } else if (!direct && new_x >= 0) {
      // Case: Mouse is inside child. Dispatch event to child,
      // except in the case where the event is a mouse over event 
      // (when the mouse enters the child and parent at the same time).
      if (!impl_->mouseover_scrollbar_) {
        // Case: Mouse just moved inside child. Turn on mouseover bit and  
        // synthesize a mouse over event. The original event still needs to 
        // be dispatched to child.
        impl_->mouseover_scrollbar_ = impl_->scrollbar_;
        MouseEvent in(Event::EVENT_MOUSE_OVER, new_x, event.GetY(), 
                      event.GetButton(), event.GetWheelDelta(),
                      event.GetModifier());
        impl_->mouseover_scrollbar_->OnMouseEvent(in, true, 
                                                  &new_fired, &new_in);
        // Ignore return from handler and don't return to continue processing.
        if (t == Event::EVENT_MOUSE_OVER) {
          // Case: Mouse entered child and parent at same time.
          // Parent also needs this event, so send to parent
          // and return.
          return BasicElement::OnMouseEvent(event, direct, 
                                            fired_element, in_element);         
        } 
      }

      // Send event to child.
      MouseEvent new_event(event);
      new_event.SetX(new_x);
      r = impl_->scrollbar_->OnMouseEvent(new_event, direct,
                                          fired_element, in_element);
      if (*fired_element == impl_->scrollbar_) {
        if (t == Event::EVENT_MOUSE_DOWN) {
          impl_->grabbed_scrollbar_ = impl_->scrollbar_; 
        }      
        *fired_element = this;
      }
      if (*in_element == impl_->scrollbar_) {
        *in_element = this;
      }
      return r;
    } else if (impl_->mouseover_scrollbar_) {
      // Case: Mouse isn't in child, but mouseover bit is on, so turn 
      // it off and send a mouse out event to child. The original event is 
      // still dispatched to parent.
      MouseEvent new_event(Event::EVENT_MOUSE_OUT, new_x, event.GetY(), 
                           event.GetButton(), event.GetWheelDelta(),
                           event.GetModifier());
      impl_->mouseover_scrollbar_->OnMouseEvent(new_event, true, 
                                                &new_fired, &new_in);
      impl_->mouseover_scrollbar_ = NULL;

      // Don't return, dispatch event to parent.
    }

    // Else not handled, send to BasicElement::OnMouseEvent
  } else {
    // Visible state may change while grabbed or hovered.
    impl_->grabbed_scrollbar_ = NULL;
    impl_->mouseover_scrollbar_ = NULL;
  }

  return BasicElement::OnMouseEvent(event, direct, fired_element, in_element);
}

EventResult DivElement::OnDragEvent(const DragEvent &event, bool direct,
                                     BasicElement **fired_element) {
  if (!direct && impl_->scrollbar_ && impl_->scrollbar_->IsVisible()) {
    double new_x = event.GetX() - impl_->scrollbar_->GetPixelX();
    if (new_x >= 0) {
      DragEvent new_event(event);
      new_event.SetX(new_x);
      EventResult r = impl_->scrollbar_->OnDragEvent(new_event, direct,
                                                     fired_element);
      if (*fired_element == impl_->scrollbar_) {
        *fired_element = this;
      }
      return r;
    }
  }

  return BasicElement::OnDragEvent(event, direct, fired_element);
}

EventResult DivElement::HandleMouseEvent(const MouseEvent &event) {
  if (impl_->scrollbar_ && impl_->scrollbar_->IsVisible()) {
    if (event.GetType() == Event::EVENT_MOUSE_WHEEL) {
      return impl_->scrollbar_->HandleMouseEvent(event);
    }
  }
  return EVENT_RESULT_UNHANDLED;
}

EventResult DivElement::HandleKeyEvent(const KeyboardEvent &event) {
  return impl_->HandleKeyEvent(event);
}

void DivElement::SelfCoordToChildCoord(const BasicElement *child,
                                       double x, double y,
                                       double *child_x, double *child_y) const {  
  if (child != impl_->scrollbar_ && impl_->scrollbar_) {
    x += impl_->scroll_pos_x_;
    y += impl_->scroll_pos_y_;
  }

  BasicElement::SelfCoordToChildCoord(child, x, y, child_x, child_y);
}

void DivElement::Layout() {
  BasicElement::Layout();
  if (impl_->scrollbar_) {
    impl_->scrollbar_->SetPixelX(GetPixelWidth() -
                                 impl_->scrollbar_->GetPixelWidth());
    impl_->scrollbar_->SetPixelHeight(GetPixelHeight());

    int v = impl_->scrollbar_->GetValue();
    if (impl_->scroll_pos_y_ != v) {
      impl_->scroll_pos_y_ = v;
    } 

    double children_width = 0, children_height = 0;
    down_cast<Elements *>(GetChildren())->GetChildrenExtents(&children_width,
                                                             &children_height);
    impl_->UpdateScrollPos(static_cast<size_t>(ceil(children_width)),
                           static_cast<size_t>(ceil(children_height)));
  }
}

} // namespace ggadget
