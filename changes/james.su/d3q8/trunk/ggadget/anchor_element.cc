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

#include "anchor_element.h"
#include "text_frame.h"
#include "string_utils.h"
#include "view_interface.h"
#include "graphics_interface.h"
#include "texture.h"
#include "event.h"

namespace ggadget {

static const char *const kDefaultColor = "#0000FF";

class AnchorElement::Impl {
 public:
  Impl(BasicElement *owner, ViewInterface *view) 
    : text_(owner, view),
      overcolor_texture_(view->LoadTexture(Variant(kDefaultColor))),
      mouseover_(false), overcolor_(kDefaultColor) { 
  }
  ~Impl() {
    delete overcolor_texture_;
    overcolor_texture_ = NULL;
  }

  TextFrame text_;
  Texture *overcolor_texture_;
  bool mouseover_;
  std::string overcolor_, href_;
};

AnchorElement::AnchorElement(ElementInterface *parent,
                             ViewInterface *view,
                             const char *name)
    : BasicElement(parent, view, "a", name, false),
      impl_(new Impl(this, view)) {
  SetCursor(ElementInterface::CURSOR_HAND);
  SetEnabled(true);

  // Moved from Impl constructor to here because they will indirectly call
  // OnDefaultSizeChange() before impl_ is initialized.
  impl_->text_.SetColor(kDefaultColor);
  impl_->text_.SetUnderline(true);

  RegisterProperty("overColor",
                   NewSlot(this, &AnchorElement::GetOverColor),
                   NewSlot(this, &AnchorElement::SetOverColor));
  RegisterProperty("href",
                   NewSlot(this, &AnchorElement::GetHref),
                   NewSlot(this, &AnchorElement::SetHref));  
  RegisterProperty("innerText",
                   NewSlot(&impl_->text_, &TextFrame::GetText),
                   NewSlot(&impl_->text_, &TextFrame::SetText));
}

AnchorElement::~AnchorElement() {
  delete impl_;
}

void AnchorElement::DoDraw(CanvasInterface *canvas,
                        const CanvasInterface *children_canvas) {  
  if (impl_->mouseover_) {
    impl_->text_.DrawWithTexture(canvas, 0, 0, 
                                 GetPixelWidth(), GetPixelHeight(), 
                                 impl_->overcolor_texture_);
  } else {
    impl_->text_.Draw(canvas, 0, 0, GetPixelWidth(), GetPixelHeight());
  }
}

const char *AnchorElement::GetOverColor() const {
  return impl_->overcolor_.c_str();
}

void AnchorElement::SetOverColor(const char *color) {
  if (AssignIfDiffer(color, &impl_->overcolor_)) {
    delete impl_->overcolor_texture_;
    impl_->overcolor_texture_ = GetView()->LoadTexture(Variant(color));
    if (impl_->mouseover_) {
      QueueDraw();
    }
  }
}

const char *AnchorElement::GetHref() const {
  return impl_->href_.c_str();
}

void AnchorElement::SetHref(const char *href) {
  impl_->href_ = href;
}

bool AnchorElement::OnMouseEvent(MouseEvent *event, bool direct,
                                 ElementInterface **fired_element) {
  bool result = BasicElement::OnMouseEvent(event, direct, fired_element);

  // Handle the event only when the event is fired and not canceled.
  if (*fired_element && result) {
    ASSERT(IsEnabled());
    ASSERT(*fired_element == this);
    switch (event->GetType()) {
     case Event::EVENT_MOUSE_OUT:
      impl_->mouseover_ = false;
      QueueDraw();
      break;
     case Event::EVENT_MOUSE_OVER:
      impl_->mouseover_ = true;
      QueueDraw();
      break;
     case Event::EVENT_MOUSE_CLICK:
      if (!impl_->href_.empty()) {
         GetView()->OpenURL(impl_->href_.c_str()); // ignore return
      }
     default:
      break;
    }
  }

  return result;
}

ElementInterface *AnchorElement::CreateInstance(ElementInterface *parent,
                                             ViewInterface *view,
                                             const char *name) {
  return new AnchorElement(parent, view, name);
}


void AnchorElement::GetDefaultSize(double *width, double *height) const {
  CanvasInterface *canvas = GetView()->GetGraphics()->NewCanvas(5, 5);
  if (!impl_->text_.GetSimpleExtents(canvas, width, height)) {
    *width = 0;
    *height = 0;
  }
  canvas->Destroy();
}

} // namespace ggadget
