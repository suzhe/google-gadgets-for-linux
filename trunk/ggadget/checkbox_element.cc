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
#include "image.h"
#include "text_frame.h"
#include "string_utils.h"
#include "view_interface.h"
#include "event.h"
#include "scriptable_event.h"

namespace ggadget {

enum CheckedState {
  STATE_NORMAL,
  STATE_CHECKED,
  STATE_COUNT
};

static const char *const kOnChangeEvent = "onchange";

class CheckBoxElement::Impl {
 public:
  Impl(BasicElement *owner, ViewInterface *view) 
    : text_(owner, view), 
      mousedown_(false), mouseover_(false), checkbox_on_right_(false), 
      value_(STATE_NORMAL) {
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

  TextFrame text_;
  bool mousedown_;
  bool mouseover_;
  bool checkbox_on_right_;
  CheckedState value_;
  std::string image_src_[STATE_COUNT], downimage_src_[STATE_COUNT], 
              overimage_src_[STATE_COUNT], disabledimage_src_[STATE_COUNT];
  Image *image_[STATE_COUNT], *downimage_[STATE_COUNT], 
        *overimage_[STATE_COUNT], *disabledimage_[STATE_COUNT];
  EventSignal onchange_event_;
};

CheckBoxElement::CheckBoxElement(ElementInterface *parent,
                       ViewInterface *view,
                       const char *name)
    : BasicElement(parent, view, "checkbox", name, false),
      impl_(new Impl(this, view)) {
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
  Image *img = NULL;
  if (!IsEnabled()) {
    img = impl_->disabledimage_[impl_->value_];
  }
  else if (impl_->mousedown_) {
    img = impl_->downimage_[impl_->value_];
  }
  else if (impl_->mouseover_) {
    img = impl_->overimage_[impl_->value_];
  }

  if (!img) { // draw image_ as last resort
    img = impl_->image_[impl_->value_];
  }

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
    impl_->value_ = value ? STATE_CHECKED : STATE_NORMAL;
    Event event(Event::EVENT_CHANGE);
    ScriptableEvent s_event(&event, this, 0, 0);
    GetView()->FireEvent(&s_event, impl_->onchange_event_);    
    QueueDraw();
  }
}

const char *CheckBoxElement::GetImage() const {
  return impl_->image_src_[STATE_NORMAL].c_str();
}

void CheckBoxElement::SetImage(const char *img) {
  if (AssignIfDiffer(img, &impl_->image_src_[STATE_NORMAL])) {
    delete impl_->image_[STATE_NORMAL];
    impl_->image_[STATE_NORMAL] = GetView()->LoadImage(img, false);
    if (impl_->image_[STATE_NORMAL]) {
      if (!WidthIsSpecified()) {
        const CanvasInterface *canvas = impl_->image_[STATE_NORMAL]->GetCanvas();
        if (canvas) {
          SetPixelWidth(canvas->GetWidth());
        }
      }
      if (!HeightIsSpecified()) {
        const CanvasInterface *canvas = impl_->image_[STATE_NORMAL]->GetCanvas();
        if (canvas) {
          SetPixelHeight(canvas->GetHeight());
        }
      }
    }
    QueueDraw();
  }
}

const char *CheckBoxElement::GetDisabledImage() const {
  return impl_->disabledimage_src_[STATE_NORMAL].c_str();
}

void CheckBoxElement::SetDisabledImage(const char *img) {
  if (AssignIfDiffer(img, &impl_->disabledimage_src_[STATE_NORMAL])) {
    delete impl_->disabledimage_[STATE_NORMAL];
    impl_->disabledimage_[STATE_NORMAL] = GetView()->LoadImage(img, false);
    if (!IsEnabled()) {
      QueueDraw();
    }
  }
}

const char *CheckBoxElement::GetOverImage() const {
  return impl_->overimage_src_[STATE_NORMAL].c_str();
}

void CheckBoxElement::SetOverImage(const char *img) {
  if (AssignIfDiffer(img, &impl_->overimage_src_[STATE_NORMAL])) {
    delete impl_->overimage_[STATE_NORMAL];
    impl_->overimage_[STATE_NORMAL] = GetView()->LoadImage(img, false);
    if (impl_->mouseover_) {
      QueueDraw();
    }
  }
}

const char *CheckBoxElement::GetDownImage() const {
  return impl_->downimage_src_[STATE_NORMAL].c_str();
}

void CheckBoxElement::SetDownImage(const char *img) {
  if (AssignIfDiffer(img, &impl_->downimage_src_[STATE_NORMAL])) {
    delete impl_->downimage_[STATE_NORMAL];
    impl_->downimage_[STATE_NORMAL] = GetView()->LoadImage(img, false);
    if (impl_->mousedown_) {
      QueueDraw();
    }
  }
}

const char *CheckBoxElement::GetCheckedImage() const {
  return impl_->image_src_[STATE_CHECKED].c_str();
}

void CheckBoxElement::SetCheckedImage(const char *img) {
  if (AssignIfDiffer(img, &impl_->image_src_[STATE_CHECKED])) {
    delete impl_->image_[STATE_CHECKED];
    impl_->image_[STATE_CHECKED] = GetView()->LoadImage(img, false);
    if (impl_->image_[STATE_CHECKED]) {
      if (!WidthIsSpecified()) {
        const CanvasInterface *canvas = impl_->image_[STATE_CHECKED]->GetCanvas();
        if (canvas) {
          SetPixelWidth(canvas->GetWidth());
        }
      }
      if (!HeightIsSpecified()) {
        const CanvasInterface *canvas = impl_->image_[STATE_CHECKED]->GetCanvas();
        if (canvas) {
          SetPixelHeight(canvas->GetHeight());
        }
      }
    }
    QueueDraw();
  }
}

const char *CheckBoxElement::GetCheckedDisabledImage() const {
  return impl_->disabledimage_src_[STATE_CHECKED].c_str();
}

void CheckBoxElement::SetCheckedDisabledImage(const char *img) {
  if (AssignIfDiffer(img, &impl_->disabledimage_src_[STATE_CHECKED])) {
    delete impl_->disabledimage_[STATE_CHECKED];
    impl_->disabledimage_[STATE_CHECKED] = GetView()->LoadImage(img, false);
    if (!IsEnabled()) {
      QueueDraw();
    }
  }
}

const char *CheckBoxElement::GetCheckedOverImage() const {
  return impl_->overimage_src_[STATE_CHECKED].c_str();
}

void CheckBoxElement::SetCheckedOverImage(const char *img) {
  if (AssignIfDiffer(img, &impl_->overimage_src_[STATE_CHECKED])) {
    delete impl_->overimage_[STATE_CHECKED];
    impl_->overimage_[STATE_CHECKED] = GetView()->LoadImage(img, false);
    if (impl_->mouseover_) {
      QueueDraw();
    }
  }
}

const char *CheckBoxElement::GetCheckedDownImage() const {
  return impl_->downimage_src_[STATE_CHECKED].c_str();
}

void CheckBoxElement::SetCheckedDownImage(const char *img) {
  if (AssignIfDiffer(img, &impl_->downimage_src_[STATE_CHECKED])) {
    delete impl_->downimage_[STATE_CHECKED];
    impl_->downimage_[STATE_CHECKED] = GetView()->LoadImage(img, false);
    if (impl_->mousedown_) {
      QueueDraw();
    }
  }
}

ElementInterface *CheckBoxElement::CreateInstance(ElementInterface *parent,
                                             ViewInterface *view,
                                             const char *name) {
  return new CheckBoxElement(parent, view, name);
}

bool CheckBoxElement::OnMouseEvent(MouseEvent *event, bool direct,
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
     case Event::EVENT_MOUSE_CLICK:
      { // Toggle checked state and fire event         
        impl_->value_ = (impl_->value_ == STATE_NORMAL) ? 
                          STATE_CHECKED : STATE_NORMAL;
        Event event(Event::EVENT_CHANGE);
        ScriptableEvent s_event(&event, this, 0, 0);
        GetView()->FireEvent(&s_event, impl_->onchange_event_);       
        QueueDraw();
      }
      break;
     default:
      break;
    }
  }

  return result;
}

} // namespace ggadget
