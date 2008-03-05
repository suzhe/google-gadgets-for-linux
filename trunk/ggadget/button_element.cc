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
#include "canvas_utils.h"
#include "image_interface.h"
#include "gadget_consts.h"
#include "string_utils.h"
#include "text_frame.h"
#include "view.h"

namespace ggadget {

class ButtonElement::Impl {
 public:
  Impl(BasicElement *owner, View *view) : text_(owner, view),
           mousedown_(false), mouseover_(false),
           image_(NULL), downimage_(NULL),
           overimage_(NULL), disabledimage_(NULL),
           stretch_middle_(false) {
    text_.SetAlign(CanvasInterface::ALIGN_CENTER);
    text_.SetVAlign(CanvasInterface::VALIGN_MIDDLE);
  }

  ~Impl() {
    DestroyImage(image_);
    DestroyImage(downimage_);
    DestroyImage(overimage_);
    DestroyImage(disabledimage_);
  }

  TextFrame text_;
  bool mousedown_;
  bool mouseover_;
  ImageInterface *image_, *downimage_, *overimage_, *disabledimage_;
  bool stretch_middle_;
};

ButtonElement::ButtonElement(BasicElement *parent, View *view, const char *name)
    : BasicElement(parent, view, "button", name, false),
      impl_(new Impl(this, view)) {
  SetEnabled(true);
}

void ButtonElement::DoRegister() {
  BasicElement::DoRegister();
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
  RegisterProperty("caption",
                   NewSlot(&impl_->text_, &TextFrame::GetText),
                   NewSlot(&impl_->text_, &TextFrame::SetText));
  RegisterProperty("stretchMiddle",
                   NewSlot(this, &ButtonElement::IsStretchMiddle),
                   NewSlot(this, &ButtonElement::SetStretchMiddle));
}

ButtonElement::~ButtonElement() {
  delete impl_;
  impl_ = NULL;
}

void ButtonElement::DoDraw(CanvasInterface *canvas) {
  ImageInterface *img = NULL;
  if (!IsEnabled()) {
    img = impl_->disabledimage_;
  } else if (impl_->mousedown_) {
    img = impl_->downimage_;
  } else if (impl_->mouseover_) {
    img = impl_->overimage_;
  }

  if (!img) { // draw image_ as last resort
    img = impl_->image_;
  }

  double width = GetPixelWidth();
  double height = GetPixelHeight();
  if (img && width > 0 && height > 0) {
    if (impl_->stretch_middle_) {
      StretchMiddleDrawImage(img, canvas, 0, 0, width, height, -1, -1, -1, -1);
    } else {
      img->StretchDraw(canvas, 0, 0, width, height);
    }
  }
  impl_->text_.Draw(canvas, 0, 0, width, height);
}

void ButtonElement::UseDefaultImages() {
  DestroyImage(impl_->image_);
  impl_->image_ = GetView()->LoadImageFromGlobal(kButtonImage, false);
  DestroyImage(impl_->overimage_);
  impl_->overimage_ = GetView()->LoadImageFromGlobal(kButtonOverImage, false);
  DestroyImage(impl_->downimage_);
  impl_->downimage_ = GetView()->LoadImageFromGlobal(kButtonDownImage, false);
  // No default disabled image.
  DestroyImage(impl_->disabledimage_);
  impl_->disabledimage_ = NULL;
}

Variant ButtonElement::GetImage() const {
  return Variant(GetImageTag(impl_->image_));
}

void ButtonElement::SetImage(const Variant &img) {
  DestroyImage(impl_->image_);
  impl_->image_ = GetView()->LoadImage(img, false);
  QueueDraw();
}

Variant ButtonElement::GetDisabledImage() const {
  return Variant(GetImageTag(impl_->disabledimage_));
}

void ButtonElement::SetDisabledImage(const Variant &img) {
  DestroyImage(impl_->disabledimage_);
  impl_->disabledimage_ = GetView()->LoadImage(img, false);
  if (!IsEnabled()) {
    QueueDraw();
  }
}

Variant ButtonElement::GetOverImage() const {
  return Variant(GetImageTag(impl_->overimage_));
}

void ButtonElement::SetOverImage(const Variant &img) {
  DestroyImage(impl_->overimage_);
  impl_->overimage_ = GetView()->LoadImage(img, false);
  if (impl_->mouseover_ && IsEnabled()) {
    QueueDraw();
  }
}

Variant ButtonElement::GetDownImage() const {
  return Variant(GetImageTag(impl_->downimage_));
}

void ButtonElement::SetDownImage(const Variant &img) {
  DestroyImage(impl_->downimage_);
  impl_->downimage_ = GetView()->LoadImage(img, false);
  if (impl_->mousedown_ && IsEnabled()) {
    QueueDraw();
  }
}

TextFrame *ButtonElement::GetTextFrame() {
  return &impl_->text_;
}

BasicElement *ButtonElement::CreateInstance(BasicElement *parent, View *view,
                                            const char *name) {
  return new ButtonElement(parent, view, name);
}

EventResult ButtonElement::HandleMouseEvent(const MouseEvent &event) {
  EventResult result = EVENT_RESULT_HANDLED;
  switch (event.GetType()) {
   case Event::EVENT_MOUSE_DOWN:
    if (event.GetButton() & MouseEvent::BUTTON_LEFT) {
      impl_->mousedown_ = true;
      QueueDraw();
    }
    break;
   case Event::EVENT_MOUSE_UP:
    if (impl_->mousedown_) {
      impl_->mousedown_ = false;
      QueueDraw();
    }
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
    result = EVENT_RESULT_UNHANDLED;
    break;
  }
  return result;
}

void ButtonElement::GetDefaultSize(double *width, double *height) const {
  if (impl_->image_) {
    *width = impl_->image_->GetWidth();
    *height = impl_->image_->GetHeight();
  } else {
    *width = 0;
    *height = 0;
  }
}

bool ButtonElement::IsStretchMiddle() const {
  return impl_->stretch_middle_;
}

void ButtonElement::SetStretchMiddle(bool stretch_middle) {
  if (stretch_middle != impl_->stretch_middle_) {
    impl_->stretch_middle_ = stretch_middle;
    QueueDraw();
  }
}

bool ButtonElement::HasOpaqueBackground() const {
  ImageInterface *img = NULL;
  if (!IsEnabled()) {
    img = impl_->disabledimage_;
  } else if (impl_->mousedown_) {
    img = impl_->downimage_;
  } else if (impl_->mouseover_) {
    img = impl_->overimage_;
  }

  if (!img) { // draw image_ as last resort
    img = impl_->image_;
  }

  return img->IsFullyOpaque();
}

} // namespace ggadget
