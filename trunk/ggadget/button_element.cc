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

#include "button_element.h"
#include "canvas_interface.h"
#include "image.h"
#include "text_frame.h"
#include "string_utils.h"
#include "view_interface.h"
#include "event.h"

namespace ggadget {

class ButtonElement::Impl {
 public:
  Impl(BasicElement *owner, ViewInterface *view) : text_(owner, view),
           mousedown_(false), mouseover_(false),
           image_(NULL), downimage_(NULL),
           overimage_(NULL), disabledimage_(NULL) { 
    text_.SetAlign(CanvasInterface::ALIGN_CENTER);
    text_.SetVAlign(CanvasInterface::VALIGN_MIDDLE);
  }
  ~Impl() {
    delete image_;
    image_ = NULL;

    delete downimage_;
    downimage_ = NULL;

    delete overimage_;
    overimage_ = NULL;

    delete disabledimage_;
    disabledimage_ = NULL;
  }

  TextFrame text_;
  bool mousedown_;
  bool mouseover_;
  std::string image_src_, downimage_src_, overimage_src_, disabledimage_src_;
  Image *image_, *downimage_, *overimage_, *disabledimage_;
};

ButtonElement::ButtonElement(ElementInterface *parent,
                       ViewInterface *view,
                       const char *name)
    : BasicElement(parent, view, "button", name, false),
      impl_(new Impl(this, view)) {
  SetEnabled(true);
  RegisterProperty("image",
                   NewSlot(this, &ButtonElement::GetImage),
                   NewSlot(this, &ButtonElement::SetImage));
  RegisterProperty("downImage",
                   NewSlot(this, &ButtonElement::GetDownImage),
                   NewSlot(this, &ButtonElement::SetDownImage));
  RegisterProperty("overImage",
                   NewSlot(this, &ButtonElement::GetOverImage),
                   NewSlot(this, &ButtonElement::SetOverImage));
  RegisterProperty("disabledImage",
                   NewSlot(this, &ButtonElement::GetDisabledImage),
                   NewSlot(this, &ButtonElement::SetDisabledImage));
  
  // undocumented
  RegisterProperty("caption",
                   NewSlot(&impl_->text_, &TextFrame::GetText),
                   NewSlot(&impl_->text_, &TextFrame::SetText));
}

ButtonElement::~ButtonElement() {
  delete impl_;
}

void ButtonElement::DoDraw(CanvasInterface *canvas,
                           const CanvasInterface *children_canvas) {
  Image *img = NULL;
  if (!IsEnabled()) {
    img = impl_->disabledimage_;
  }
  else if (impl_->mousedown_) {
    img = impl_->downimage_;
  }
  else if (impl_->mouseover_) {
    img = impl_->overimage_;
  }

  if (!img) { // draw image_ as last resort
    img = impl_->image_;
  }

  double width = GetPixelWidth();
  double height = GetPixelHeight();
  if (img) {
    img->StretchDraw(canvas, 0, 0, width, height);
  }
  impl_->text_.Draw(canvas, 0, 0, width, height);
}

const char *ButtonElement::GetImage() const {
  return impl_->image_src_.c_str();
}

void ButtonElement::SetImage(const char *img) {
  if (AssignIfDiffer(img, &impl_->image_src_)) {
    delete impl_->image_;
    impl_->image_ = GetView()->LoadImage(img, false);
    if (impl_->image_) {
      if (!WidthIsSpecified()) {
        const CanvasInterface *canvas = impl_->image_->GetCanvas();
        if (canvas) {
          SetPixelWidth(canvas->GetWidth());
        }
      }
      if (!HeightIsSpecified()) {
        const CanvasInterface *canvas = impl_->image_->GetCanvas();
        if (canvas) {
          SetPixelHeight(canvas->GetHeight());
        }
      }
    }
    QueueDraw();
  }
}

const char *ButtonElement::GetDisabledImage() const {
  return impl_->disabledimage_src_.c_str();
}

void ButtonElement::SetDisabledImage(const char *img) {
  if (AssignIfDiffer(img, &impl_->disabledimage_src_)) {
    delete impl_->disabledimage_;
    impl_->disabledimage_ = GetView()->LoadImage(img, false);
    if (!IsEnabled()) {
      QueueDraw();
    }
  }
}

const char *ButtonElement::GetOverImage() const {
  return impl_->overimage_src_.c_str();
}

void ButtonElement::SetOverImage(const char *img) {
  if (AssignIfDiffer(img, &impl_->overimage_src_)) {
    delete impl_->overimage_;
    impl_->overimage_ = GetView()->LoadImage(img, false);
    if (impl_->mouseover_) {
      QueueDraw();
    }
  }
}

const char *ButtonElement::GetDownImage() const {
  return impl_->downimage_src_.c_str();
}

void ButtonElement::SetDownImage(const char *img) {
  if (AssignIfDiffer(img, &impl_->downimage_src_)) {
    delete impl_->downimage_;
    impl_->downimage_ = GetView()->LoadImage(img, false);
    if (impl_->mousedown_) {
      QueueDraw();
    }
  }
}

ElementInterface *ButtonElement::CreateInstance(ElementInterface *parent,
                                             ViewInterface *view,
                                             const char *name) {
  return new ButtonElement(parent, view, name);
}

bool ButtonElement::OnMouseEvent(MouseEvent *event, bool direct,
                                 ElementInterface **fired_element) {
  bool result = BasicElement::OnMouseEvent(event, direct, fired_element);

  // Handle the event only when the event is fired and not canceled.
  if (*fired_element && result) {
    ASSERT(IsEnabled());
    ASSERT(*fired_element == this);
    switch (event->GetType()) {
     case Event::EVENT_MOUSE_DOWN:
      impl_->mousedown_ = true;
      QueueDraw();
      break;
     case Event::EVENT_MOUSE_UP:
      impl_->mousedown_ = false;
      QueueDraw();
      break;
     case Event::EVENT_MOUSE_OUT:
      impl_->mouseover_ = false;
      QueueDraw();
      break;
     case Event::EVENT_MOUSE_OVER:
      impl_->mouseover_ = true;
      QueueDraw();
      break;
     default:
      break;
    }
  }

  return result;
}

} // namespace ggadget
