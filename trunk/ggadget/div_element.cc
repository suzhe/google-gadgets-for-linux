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

#include <cmath>
#include "div_element.h"
#include "canvas_interface.h"
#include "canvas_utils.h"
#include "elements.h"
#include "logger.h"
#include "texture.h"
#include "view.h"

namespace ggadget {

static const char *kBackgroundModes[] = { "tile", "stretch", "stretchMiddle" };

class DivElement::Impl {
 public:
  Impl(DivElement *owner)
      : owner_(owner),
        background_texture_(NULL),
        background_mode_(BACKGROUND_MODE_TILE) {
  }
  ~Impl() {
    delete background_texture_;
    background_texture_ = NULL;
  }

  DivElement *owner_;
  Texture *background_texture_;
  BackgroundMode background_mode_;
};

DivElement::DivElement(BasicElement *parent, View *view, const char *name)
    : ScrollingElement(parent, view, "div", name, true),
      impl_(new Impl(this)) {
}

DivElement::DivElement(BasicElement *parent, View *view,
                       const char *tag_name, const char *name)
    : ScrollingElement(parent, view, tag_name, name, true),
      impl_(new Impl(this)) {
}

void DivElement::DoRegister() {
  ScrollingElement::DoRegister();
  RegisterProperty("autoscroll",
                   NewSlot(implicit_cast<ScrollingElement *>(this),
                           &ScrollingElement::IsAutoscroll),
                   NewSlot(implicit_cast<ScrollingElement *>(this),
                           &ScrollingElement::SetAutoscroll));
  RegisterProperty("background",
                   NewSlot(this, &DivElement::GetBackground),
                   NewSlot(this, &DivElement::SetBackground));
  RegisterStringEnumProperty("backgroundMode",
                             NewSlot(this, &DivElement::GetBackgroundMode),
                             NewSlot(this, &DivElement::SetBackgroundMode),
                             kBackgroundModes, arraysize(kBackgroundModes));
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

  SetYPageStep(static_cast<int>(round(GetClientHeight())));
  SetXPageStep(static_cast<int>(round(GetClientWidth())));

  if (UpdateScrollBar(x_range, y_range)) {
    // Layout again to reflect visibility change of the scroll bars.
    Layout();
  }
}

void DivElement::DoDraw(CanvasInterface *canvas) {
  if (impl_->background_texture_) {
    const ImageInterface *texture_image =
        impl_->background_texture_->GetImage();
    if (!texture_image || impl_->background_mode_ == BACKGROUND_MODE_TILE) {
      impl_->background_texture_->Draw(canvas);
    } else if (impl_->background_mode_ == BACKGROUND_MODE_STRETCH) {
      texture_image->StretchDraw(canvas, 0, 0,
                                 GetPixelWidth(), GetPixelHeight());
    } else {
      StretchMiddleDrawImage(texture_image, canvas, 0, 0,
                             GetPixelWidth(), GetPixelHeight(),
                             -1, -1, -1, -1);
    }
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

DivElement::BackgroundMode DivElement::GetBackgroundMode() const {
  return impl_->background_mode_;
}

void DivElement::SetBackgroundMode(BackgroundMode mode) {
  if (mode != impl_->background_mode_) {
    impl_->background_mode_ = mode;
    QueueDraw();
  }
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

bool DivElement::IsChildInVisibleArea(const BasicElement *child) const {
  ASSERT(child && child->GetParentElement() == this);

  double min_x, min_y, max_x, max_y;
  double x, y;
  double width = child->GetPixelWidth();
  double height = child->GetPixelHeight();

  struct Vertex {
    double x, y;
  } vertexes[] = {
    { 0, height },
    { width, height },
    { width, 0 }
  };

  ChildCoordToSelfCoord(child, 0, 0, &min_x, &min_y);
  ChildCoordToSelfCoord(child, 0, 0, &max_x, &max_y);

  for (size_t i = 0; i < 3; ++i) {
    ChildCoordToSelfCoord(child, vertexes[i].x, vertexes[i].y, &x, &y);
    min_x = std::min(x, min_x);
    min_y = std::min(y, min_y);
    max_x = std::max(x, max_x);
    max_y = std::max(y, max_y);
  }

  if (max_x < 0 || max_y < 0 ||
      min_x > GetPixelWidth() || min_y > GetPixelHeight())
    return false;

  return true;
}

bool DivElement::HasOpaqueBackground() const {
  return impl_->background_texture_ ?
         impl_->background_texture_->IsFullyOpaque() :
         false;
}

} // namespace ggadget
