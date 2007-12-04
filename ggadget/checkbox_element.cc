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

#include "checkbox_element.h"
#include "canvas_interface.h"
#include "graphics_interface.h"
#include "image.h"
#include "scriptable_event.h"
#include "string_utils.h"
#include "text_frame.h"
#include "view.h"

namespace ggadget {

enum CheckedState {
  STATE_NORMAL,
  STATE_CHECKED,
  STATE_COUNT
};

static const char *const kOnChangeEvent = "onchange";

class CheckBoxElement::Impl {
 public:
  Impl(BasicElement *owner, View *view, bool is_checkbox)
    : is_checkbox_(is_checkbox), text_(owner, view),
      mousedown_(false), mouseover_(false), checkbox_on_right_(false),
      value_(STATE_CHECKED) {
    for (int i = 0; i < STATE_COUNT; i++) {
      image_[i] = NULL;
      downimage_[i] = NULL;
      overimage_[i] = NULL;
      disabledimage_[i] = NULL;
    }
    text_.SetVAlign(CanvasInterface::VALIGN_MIDDLE);
  }
  ~Impl() {
    for (int i = 0; i < STATE_COUNT; i++) {
      delete image_[i];
      image_[i] = NULL;

      delete downimage_[i];
      downimage_[i] = NULL;

      delete overimage_[i];
      overimage_[i] = NULL;

      delete disabledimage_[i];
      disabledimage_[i] = NULL;
    }
  }

  Image *GetCurrentImage(const CheckBoxElement *owner) {
    Image *img = NULL;
    if (!owner->IsEnabled()) {
      img = disabledimage_[value_];
    } else if (mousedown_) {
      img = downimage_[value_];
    } else if (mouseover_) {
      img = overimage_[value_];
    }

    if (!img) { // Leave this case as fallback if the exact image is NULL. 
      img = image_[value_];  
    }    
    return img;
  }

  bool is_checkbox_;
  TextFrame text_;
  bool mousedown_;
  bool mouseover_;
  bool checkbox_on_right_;
  CheckedState value_;
  Image *image_[STATE_COUNT], *downimage_[STATE_COUNT],
        *overimage_[STATE_COUNT], *disabledimage_[STATE_COUNT];
  EventSignal onchange_event_;
};

CheckBoxElement::CheckBoxElement(BasicElement *parent, View *view,
                                 const char *name, bool is_checkbox)
    : BasicElement(parent, view, is_checkbox ? "checkbox" : "radio", name, NULL),
      impl_(new Impl(this, view, is_checkbox)) {
  SetEnabled(true);

  RegisterProperty("value",
                   NewSlot(this, &CheckBoxElement::GetValue),
                   NewSlot(this, &CheckBoxElement::SetValue));
  RegisterProperty("image",
                   NewSlot(this, &CheckBoxElement::GetImage),
                   NewSlot(this, &CheckBoxElement::SetImage));
  RegisterProperty("downImage",
                   NewSlot(this, &CheckBoxElement::GetDownImage),
                   NewSlot(this, &CheckBoxElement::SetDownImage));
  RegisterProperty("overImage",
                   NewSlot(this, &CheckBoxElement::GetOverImage),
                   NewSlot(this, &CheckBoxElement::SetOverImage));
  RegisterProperty("disabledImage",
                   NewSlot(this, &CheckBoxElement::GetDisabledImage),
                   NewSlot(this, &CheckBoxElement::SetDisabledImage));
  RegisterProperty("checkedImage",
                   NewSlot(this, &CheckBoxElement::GetCheckedImage),
                   NewSlot(this, &CheckBoxElement::SetCheckedImage));
  RegisterProperty("checkedDownImage",
                   NewSlot(this, &CheckBoxElement::GetCheckedDownImage),
                   NewSlot(this, &CheckBoxElement::SetCheckedDownImage));
  RegisterProperty("checkedOverImage",
                   NewSlot(this, &CheckBoxElement::GetCheckedOverImage),
                   NewSlot(this, &CheckBoxElement::SetCheckedOverImage));
  RegisterProperty("checkedDisabledImage",
                   NewSlot(this, &CheckBoxElement::GetCheckedDisabledImage),
                   NewSlot(this, &CheckBoxElement::SetCheckedDisabledImage));

  // undocumented properties
  RegisterProperty("caption",
                   NewSlot(&impl_->text_, &TextFrame::GetText),
                   NewSlot(&impl_->text_, &TextFrame::SetText));
  RegisterProperty("checkboxOnRight",
                   NewSlot(this, &CheckBoxElement::IsCheckBoxOnRight),
                   NewSlot(this, &CheckBoxElement::SetCheckBoxOnRight));

  RegisterSignal(kOnChangeEvent, &impl_->onchange_event_);
}

CheckBoxElement::~CheckBoxElement() {
  delete impl_;
}

void CheckBoxElement::DoDraw(CanvasInterface *canvas,
                             const CanvasInterface *children_canvas) {
  Image *img = impl_->GetCurrentImage(this);

  const double h = GetPixelHeight();
  double textx = 0;
  double textwidth = GetPixelWidth();
  if (img) {
    double imgw = img->GetWidth();
    textwidth -= imgw;
    double imgx;
    if (impl_->checkbox_on_right_) {
      imgx = textwidth;
    } else {
      textx = imgw;
      imgx = 0.;
    }
    img->Draw(canvas, imgx, (h - img->GetHeight()) / 2.);
  }
  impl_->text_.Draw(canvas, textx, 0., textwidth, h);
}

bool CheckBoxElement::IsCheckBoxOnRight() const {
  return impl_->checkbox_on_right_;
}

void CheckBoxElement::SetCheckBoxOnRight(bool right) {
  if (right != impl_->checkbox_on_right_) {
    impl_->checkbox_on_right_ = right;
    QueueDraw();
  }
}

bool CheckBoxElement::GetValue() const {
  return (impl_->value_ == STATE_CHECKED);
}

void CheckBoxElement::SetValue(bool value) {
  if (value != (impl_->value_ == STATE_CHECKED)) {
    QueueDraw();
    impl_->value_ = value ? STATE_CHECKED : STATE_NORMAL;
    Event event(Event::EVENT_CHANGE);
    ScriptableEvent s_event(&event, this, NULL);
    GetView()->FireEvent(&s_event, impl_->onchange_event_);
  }
}

Variant CheckBoxElement::GetImage() const {
  return Variant(Image::GetSrc(impl_->image_[STATE_NORMAL]));
}

void CheckBoxElement::SetImage(const Variant &img) {
  delete impl_->image_[STATE_NORMAL];
  impl_->image_[STATE_NORMAL] = GetView()->LoadImage(img, false);
  OnDefaultSizeChange();
  QueueDraw();
}

Variant CheckBoxElement::GetDisabledImage() const {
  return Variant(Image::GetSrc(impl_->disabledimage_[STATE_NORMAL]));
}

void CheckBoxElement::SetDisabledImage(const Variant &img) {
  delete impl_->disabledimage_[STATE_NORMAL];
  impl_->disabledimage_[STATE_NORMAL] = GetView()->LoadImage(img, false);
  if (!IsEnabled()) {
    OnDefaultSizeChange();
    QueueDraw();
  }
}

Variant CheckBoxElement::GetOverImage() const {
  return Variant(Image::GetSrc(impl_->overimage_[STATE_NORMAL]));
}

void CheckBoxElement::SetOverImage(const Variant &img) {
  delete impl_->overimage_[STATE_NORMAL];
  impl_->overimage_[STATE_NORMAL] = GetView()->LoadImage(img, false);
  if (impl_->mouseover_ && IsEnabled()) {
    OnDefaultSizeChange();
    QueueDraw();
  }
}

Variant CheckBoxElement::GetDownImage() const {
  return Variant(Image::GetSrc(impl_->downimage_[STATE_NORMAL]));
}

void CheckBoxElement::SetDownImage(const Variant &img) {
  delete impl_->downimage_[STATE_NORMAL];
  impl_->downimage_[STATE_NORMAL] = GetView()->LoadImage(img, false);
  if (impl_->mousedown_ && IsEnabled()) {
    OnDefaultSizeChange();
    QueueDraw();
  }
}

Variant CheckBoxElement::GetCheckedImage() const {
  return Variant(Image::GetSrc(impl_->image_[STATE_CHECKED]));
}

void CheckBoxElement::SetCheckedImage(const Variant &img) {
  delete impl_->image_[STATE_CHECKED];
  impl_->image_[STATE_CHECKED] = GetView()->LoadImage(img, false);
  OnDefaultSizeChange();
  QueueDraw();
}

Variant CheckBoxElement::GetCheckedDisabledImage() const {
  return Variant(Image::GetSrc(impl_->disabledimage_[STATE_CHECKED]));
}

void CheckBoxElement::SetCheckedDisabledImage(const Variant &img) {
  delete impl_->disabledimage_[STATE_CHECKED];
  impl_->disabledimage_[STATE_CHECKED] = GetView()->LoadImage(img, false);
  if (!IsEnabled()) {
    OnDefaultSizeChange();
    QueueDraw();
  }
}

Variant CheckBoxElement::GetCheckedOverImage() const {
  return Variant(Image::GetSrc(impl_->overimage_[STATE_CHECKED]));
}

void CheckBoxElement::SetCheckedOverImage(const Variant &img) {
  delete impl_->overimage_[STATE_CHECKED];
  impl_->overimage_[STATE_CHECKED] = GetView()->LoadImage(img, false);
  if (impl_->mouseover_ && IsEnabled()) {
    OnDefaultSizeChange();
    QueueDraw();
  }
}

Variant CheckBoxElement::GetCheckedDownImage() const {
  return Variant(Image::GetSrc(impl_->downimage_[STATE_CHECKED]));
}

void CheckBoxElement::SetCheckedDownImage(const Variant &img) {
  delete impl_->downimage_[STATE_CHECKED];
  impl_->downimage_[STATE_CHECKED] = GetView()->LoadImage(img, false);
  if (impl_->mousedown_ && IsEnabled()) {
    OnDefaultSizeChange();
    QueueDraw();
  }
}

TextFrame *CheckBoxElement::GetTextFrame() {
  return &impl_->text_;
}

EventResult CheckBoxElement::HandleMouseEvent(const MouseEvent &event) {
  EventResult result = EVENT_RESULT_HANDLED;
  switch (event.GetType()) {
    case Event::EVENT_MOUSE_DOWN:
      impl_->mousedown_ = true;
      OnDefaultSizeChange();
      QueueDraw();
      break;
    case Event::EVENT_MOUSE_UP:
      impl_->mousedown_ = false;
      OnDefaultSizeChange();
      QueueDraw();
      break;
    case Event::EVENT_MOUSE_OUT:
      impl_->mouseover_ = false;
      OnDefaultSizeChange();
      QueueDraw();
      break;
    case Event::EVENT_MOUSE_OVER:
      impl_->mouseover_ = true;
      OnDefaultSizeChange();
      QueueDraw();
      break;
    case Event::EVENT_MOUSE_CLICK: {
      // Toggle checked state and fire event
      if (impl_->is_checkbox_) {
        impl_->value_ = (impl_->value_ == STATE_NORMAL) ?
                          STATE_CHECKED : STATE_NORMAL;
      } else {
        if (impl_->value_ == STATE_CHECKED) {
          break; // Radio buttons don't change state in this situation.
        }
        impl_->value_ = STATE_CHECKED;
      }
      OnDefaultSizeChange();
      QueueDraw();
      Event event(Event::EVENT_CHANGE);
      ScriptableEvent s_event(&event, this, NULL);
      GetView()->FireEvent(&s_event, impl_->onchange_event_);
      break;
    }
    default:
      result = EVENT_RESULT_UNHANDLED;
      break;
  }
  return result;
}

Connection *CheckBoxElement::ConnectEvent(const char *event_name,
                                          Slot0<void> *handler) {
  if (GadgetStrCmp(event_name, kOnChangeEvent) == 0)
    return impl_->onchange_event_.Connect(handler);
  return BasicElement::ConnectEvent(event_name, handler);
}

void CheckBoxElement::GetDefaultSize(double *width, double *height) const {
  double image_width = 0, image_height = 0;
  Image *image = impl_->GetCurrentImage(this);
  if (image) {
    image_width = image->GetWidth();
    image_height = image->GetHeight();
  }

  double text_width = 0, text_height = 0;
  CanvasInterface *canvas = GetView()->GetGraphics()->NewCanvas(1, 1);
  impl_->text_.GetSimpleExtents(canvas, &text_width, &text_height);
  canvas->Destroy();

  *width = image_width + text_width;
  *height = std::max(image_height, text_height);
}

BasicElement *CheckBoxElement::CreateCheckBoxInstance(BasicElement *parent,
                                                      View *view,
                                                      const char *name) {
  return new CheckBoxElement(parent, view, name, true);
}

BasicElement *CheckBoxElement::CreateRadioInstance(BasicElement *parent,
                                                   View *view,
                                                   const char *name) {
  return new CheckBoxElement(parent, view, name, false);
}

} // namespace ggadget
