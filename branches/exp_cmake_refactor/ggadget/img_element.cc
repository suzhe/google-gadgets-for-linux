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

#include "img_element.h"
#include "canvas_interface.h"
#include "image.h"
#include "string_utils.h"
#include "view_interface.h"

namespace ggadget {

class ImgElement::Impl {
 public:
  Impl() : image_(NULL) { }
  ~Impl() {
    delete image_;
    image_ = NULL;
  }

  std::string src_;
  Image *image_;
};

ImgElement::ImgElement(ElementInterface *parent,
                       ViewInterface *view,
                       const char *name)
    : BasicElement(parent, view, "img", name, false),
      impl_(new Impl) {
  RegisterProperty("src",
                   NewSlot(this, &ImgElement::GetSrc),
                   NewSlot(this, &ImgElement::SetSrc));
  RegisterProperty("srcWidth", NewSlot(this, &ImgElement::GetSrcWidth), NULL);
  RegisterProperty("srcHeight", NewSlot(this, &ImgElement::GetSrcHeight), NULL);
  RegisterMethod("setSrcSize", NewSlot(this, &ImgElement::SetSrcSize));
}

ImgElement::~ImgElement() {
  delete impl_;
}

void ImgElement::DoDraw(CanvasInterface *canvas,
                        const CanvasInterface *children_canvas) {
  if (impl_->image_)
    impl_->image_->StretchDraw(canvas, 0, 0,
                               GetPixelWidth(), GetPixelHeight());
}

const char *ImgElement::GetSrc() const {
  return impl_->src_.c_str();
}

void ImgElement::SetSrc(const char *src) {
  if (AssignIfDiffer(src, &impl_->src_)) {
    SetSelfChanged(true);
    delete impl_->image_;
    impl_->image_ = GetView()->LoadImage(src, false);
    if (GetPixelWidth() == 0.0)
      SetPixelWidth(GetSrcWidth());
    if (GetPixelHeight() == 0.0)
      SetPixelHeight(GetSrcHeight());
  }
}

size_t ImgElement::GetSrcWidth() const {
  return impl_->image_ ? impl_->image_->GetWidth() : 0; 
}

size_t ImgElement::GetSrcHeight() const {
  return impl_->image_ ? impl_->image_->GetHeight() : 0;
}

void ImgElement::SetSrcSize(size_t width, size_t height) {
  // Because image data may shared among elements, we don't think this method
  // is useful, because we may require more memory to store the new resized
  // canvas.
}

ElementInterface *ImgElement::CreateInstance(ElementInterface *parent,
                                             ViewInterface *view,
                                             const char *name) {
  return new ImgElement(parent, view, name);
}

} // namespace ggadget
