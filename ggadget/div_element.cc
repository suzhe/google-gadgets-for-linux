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
#include "texture.h"
#include "view.h"

namespace ggadget {

class DivElement::Impl {
 public:
  Impl(DivElement *owner)
      : owner_(owner),
        background_texture_(NULL) {
  }
  ~Impl() {
    delete background_texture_;
    background_texture_ = NULL;
  }

  DivElement *owner_;
  Texture *background_texture_;
};

DivElement::DivElement(BasicElement *parent, View *view, const char *name)
    : ScrollingElement(parent, view, "div", name, true),
      impl_(new Impl(this)) {
  RegisterProperty("autoscroll",
                   NewSlot(implicit_cast<ScrollingElement *>(this),
                           &ScrollingElement::IsAutoscroll),
                   NewSlot(implicit_cast<ScrollingElement *>(this),
                           &ScrollingElement::SetAutoscroll));
  RegisterProperty("background",
                   NewSlot(this, &DivElement::GetBackground),
                   NewSlot(this, &DivElement::SetBackground));
}

DivElement::DivElement(BasicElement *parent, View *view,
                       const char *tag_name, const char *name)
    : ScrollingElement(parent, view, tag_name, name, true),
      impl_(new Impl(this)) {
}

DivElement::~DivElement() {
  delete impl_;
  impl_ = NULL;
}

void DivElement::Layout() {
  ScrollingElement::Layout();
  double children_width = 0, children_height = 0;
  GetChildrenExtents(&children_width, &children_height);

  int x_range = static_cast<int>(ceil(children_width - GetClientWidth()));
  int y_range = static_cast<int>(ceil(children_height - GetClientHeight()));

  if (x_range < 0) x_range = 0;
  if (y_range < 0) y_range = 0;

  if (UpdateScrollBar(x_range, y_range)) {
    // Layout again to reflect change of the scroll bar.
    Layout();
  }
}

void DivElement::DoDraw(CanvasInterface *canvas) {
  if (impl_->background_texture_) {
    impl_->background_texture_->Draw(canvas);
  }

  canvas->TranslateCoordinates(-GetScrollXPosition(),
                               -GetScrollYPosition());
  DrawChildren(canvas);
  canvas->TranslateCoordinates(GetScrollXPosition(),
                               GetScrollYPosition());
  DrawScrollbar(canvas);
}

Variant DivElement::GetBackground() const {
  return Variant(Texture::GetSrc(impl_->background_texture_));
}

void DivElement::SetBackground(const Variant &background) {
  delete impl_->background_texture_;
  impl_->background_texture_ = GetView()->LoadTexture(background);
  QueueDraw();
}

BasicElement *DivElement::CreateInstance(BasicElement *parent, View *view,
                                         const char *name) {
  return new DivElement(parent, view, name);
}

EventResult DivElement::HandleKeyEvent(const KeyboardEvent &event) {
  static const int kLineHeight = 5;
  static const int kLineWidth = 5;

  EventResult result = EVENT_RESULT_UNHANDLED;
  if (IsAutoscroll() && event.GetType() == Event::EVENT_KEY_DOWN) {
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
        ScrollY(-static_cast<int>(ceil(GetClientHeight())));
        break;
      case KeyboardEvent::KEY_PAGE_DOWN:
        ScrollY(static_cast<int>(ceil(GetClientHeight())));
        break;
      default:
        result = EVENT_RESULT_UNHANDLED;
        break;
    }
  }
  return result;
}

} // namespace ggadget
