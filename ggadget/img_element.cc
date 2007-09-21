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

#include "canvas_interface.h"
#include "img_element.h"

namespace ggadget {

class ImgElement::Impl {
 public:
  Impl() : canvas_(NULL) { }
  std::string src_;
  CanvasInterface *canvas_;
};

ImgElement::ImgElement(ElementInterface *parent,
                       ViewInterface *view,
                       const char *name)
    : BasicElement(parent, view, name, false),
      impl_(new Impl) {
}

ImgElement::~ImgElement() {
  delete impl_;
}

const char *ImgElement::GetSrc() const {
  return impl_->src_.c_str();
}

void ImgElement::SetSrc(const char *src) {
  impl_->src_ = src;
  // TODO: load the image and convert it into a canvas.
}

size_t ImgElement::GetSrcWidth() const {
  return impl_->canvas_ ? impl_->canvas_->GetWidth() : 0;
}


size_t ImgElement::GetSrcHeight() const {
  return impl_->canvas_ ? impl_->canvas_->GetHeight() : 0;
}

void ImgElement::SetSrcSize(size_t width, size_t height) {
  if (impl_->canvas_) {
    // TODO: create a new canvas, and draw the current canvas onto it.
    // May share code with DrawSelf.
  }
}

} // namespace ggadget
