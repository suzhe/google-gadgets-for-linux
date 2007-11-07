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

#include "label_element.h"
#include "graphics_interface.h"
#include "view_interface.h"
#include "text_frame.h"

namespace ggadget {

class LabelElement::Impl {
 public:
  Impl(BasicElement *owner, ViewInterface *view) : text_(owner, view) { }
  ~Impl() {
  }

  TextFrame text_;
};

LabelElement::LabelElement(ElementInterface *parent,
                       ViewInterface *view,
                       const char *name)
    : BasicElement(parent, view, "label", name, false),
      impl_(new Impl(this, view)) {
  RegisterProperty("innerText",
                   NewSlot(&impl_->text_, &TextFrame::GetText),
                   NewSlot(this, &LabelElement::SetText));
}

LabelElement::~LabelElement() {
  delete impl_;
}

void LabelElement::DoDraw(CanvasInterface *canvas,
                        const CanvasInterface *children_canvas) {
  impl_->text_.Draw(canvas, 0, 0, GetPixelWidth(), GetPixelHeight());
}

void LabelElement::SetText(const char *text) {
  impl_->text_.SetText(text);
  if (!WidthIsSpecified() || !HeightIsSpecified()) {
    double w, h;
    CanvasInterface *canvas = GetView()->GetGraphics()->NewCanvas(5, 5);
    if (impl_->text_.GetSimpleExtents(canvas, &w, &h)) {
      if (!WidthIsSpecified()) {
        SetPixelWidth(w);
      }
      if (!HeightIsSpecified()) {
        SetPixelHeight(h);
      }
    }
    canvas->Destroy();
    canvas = NULL;
  }
}

ElementInterface *LabelElement::CreateInstance(ElementInterface *parent,
                                             ViewInterface *view,
                                             const char *name) {
  return new LabelElement(parent, view, name);
}

} // namespace ggadget
