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

#include "div_element.h"
#include "canvas_interface.h"
#include "elements.h"
#include "string_utils.h"
#include "texture.h"
#include "view_interface.h"

namespace ggadget {

class DivElement::Impl {
 public:
  Impl() : background_texture_(NULL), autoscroll_(false) { }
  ~Impl() {
    delete background_texture_;
    background_texture_ = NULL;
  }

  std::string background_;
  Texture *background_texture_;
  bool autoscroll_;
};

DivElement::DivElement(ElementInterface *parent,
                       ViewInterface *view,
                       const char *name)
    : BasicElement(parent, view, name, true),
      impl_(new Impl) {
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

  // TODO: scroll.
  if (children_canvas)
    canvas->DrawCanvas(0, 0, children_canvas);
}

const char *DivElement::GetBackground() const {
  return impl_->background_.c_str();
}

void DivElement::SetBackground(const char *background) {
  if (AssignIfDiffer(background, &impl_->background_)) {
    SetSelfChanged(true);
    delete impl_->background_texture_;
    impl_->background_texture_ = GetView()->LoadTexture(background);
  }
}

bool DivElement::IsAutoscroll() const {
  return impl_->autoscroll_;
}

void DivElement::SetAutoscroll(bool autoscroll) {
  // TODO:
  impl_->autoscroll_ = autoscroll;
}

ElementInterface *DivElement::CreateInstance(ElementInterface *parent,
                                             ViewInterface *view,
                                             const char *name) {
  return new DivElement(parent, view, name);
}

bool DivElement::OnMouseEvent(MouseEvent *event) {
  // TODO:
  return BasicElement::OnMouseEvent(event);
}

void DivElement::OnKeyEvent(KeyboardEvent *event) {
  // TODO:
  BasicElement::OnKeyEvent(event);
}


} // namespace ggadget
