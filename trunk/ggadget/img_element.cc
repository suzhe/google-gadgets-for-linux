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

#include <algorithm>

#include "img_element.h"
#include "canvas_interface.h"
#include "color.h"
#include "image.h"
#include "string_utils.h"
#include "texture.h"
#include "view.h"
#include "color.h"

namespace ggadget {

static const char *kCropMaintainAspectNames[] = {
  "false", "true", "photo"
};

class ImgElement::Impl {
 public:
  Impl()
    : image_(NULL), src_width_(0), src_height_(0),
      crop_(CROP_FALSE) { }
  ~Impl() {
    delete image_;
    image_ = NULL;
  }

  Image *image_;
  size_t src_width_, src_height_;
  CropMaintainAspect crop_;
  std::string colormultiply_;
};

ImgElement::ImgElement(BasicElement *parent, View *view, const char *name)
    : BasicElement(parent, view, "img", name, false),
      impl_(new Impl) {
  RegisterProperty("src",
                   NewSlot(this, &ImgElement::GetSrc),
                   NewSlot(this, &ImgElement::SetSrc));
  RegisterProperty("srcWidth", NewSlot(this, &ImgElement::GetSrcWidth), NULL);
  RegisterProperty("srcHeight", NewSlot(this, &ImgElement::GetSrcHeight), NULL);
  RegisterProperty("colorMultiply",
                   NewSlot(this, &ImgElement::GetColorMultiply),
                   NewSlot(this, &ImgElement::SetColorMultiply));
  RegisterStringEnumProperty("cropMaintainAspect",
                             NewSlot(this, &ImgElement::GetCropMaintainAspect),
                             NewSlot(this, &ImgElement::SetCropMaintainAspect),
                             kCropMaintainAspectNames,
                             arraysize(kCropMaintainAspectNames));
  RegisterMethod("setSrcSize", NewSlot(this, &ImgElement::SetSrcSize));
}

ImgElement::~ImgElement() {
  delete impl_;
}

bool ImgElement::IsPointIn(double x, double y) {
  // Return false directly if the point is outside the element boundary.
  if (!BasicElement::IsPointIn(x, y))
    return false;

  bool changed;
  const CanvasInterface *canvas = Draw(&changed);

  // If the element is invisible then just return false.
  if (!canvas)
    return false;

  double opacity;

  // If failed to get the value of the point, then just return true, assuming
  // it's an opaque point.
  if (!canvas->GetPointValue(x, y, NULL, &opacity))
    return true;

  return opacity > 0;
}

void ImgElement::DoDraw(CanvasInterface *canvas,
                        const CanvasInterface *children_canvas) {
  if (impl_->image_) {
    double x, y, w, h;
    double pxwidth = GetPixelWidth();
    double pxheight = GetPixelHeight();
    if (impl_->crop_ == CROP_FALSE) {
      x = y = 0;
      w = pxwidth;
      h = pxheight;
    } else {
      size_t imgw = impl_->image_->GetWidth();
      size_t imgh = impl_->image_->GetHeight();
      if (imgw == 0 || imgh == 0) {
        return;
      }

      double scale = std::max(pxwidth / imgw, pxheight / imgh);
      // Windows also caps the scale to a fixed maximum. This is probably a bug.
      w = scale * imgw;
      h = scale * imgh;

      x = (pxwidth - w) / 2.;
      y = (pxheight - h) / 2.;
      if (impl_->crop_ == CROP_PHOTO && y < 0.) {
        y = 0.; // Never crop the top in photo setting.
      }
    }
    impl_->image_->StretchDraw(canvas, x, y, w, h);
  }
}

Variant ImgElement::GetSrc() const {
  return Variant(Image::GetSrc(impl_->image_));
}

void ImgElement::SetSrc(const Variant &src) {
  delete impl_->image_;
  impl_->image_ = GetView()->LoadImage(src, false);
  if (impl_->image_) {
    impl_->src_width_ = impl_->image_->GetWidth();
    impl_->src_height_ = impl_->image_->GetHeight();
  } else {
    impl_->src_width_ = 0;
    impl_->src_height_ = 0;
  }

  QueueDraw();
}

std::string ImgElement::GetColorMultiply() const {
  return impl_->colormultiply_;
}

void ImgElement::SetColorMultiply(const char *color) {
  if (AssignIfDiffer(color, &impl_->colormultiply_, strcmp)) {
    View *view = GetView();
    Texture texture(view->GetGraphics(), view->GetFileManager(), color);
    impl_->image_->SetColorMultiply(texture.GetColor());
    QueueDraw();
  }
}

ImgElement::CropMaintainAspect ImgElement::GetCropMaintainAspect() const {
  return impl_->crop_;
}

void ImgElement::SetCropMaintainAspect(ImgElement::CropMaintainAspect crop) {
  if (crop != impl_->crop_) {
    impl_->crop_ = crop;
    QueueDraw();
  }
}

size_t ImgElement::GetSrcWidth() const {
  return impl_->src_width_;
}

size_t ImgElement::GetSrcHeight() const {
  return impl_->src_height_;
}

void ImgElement::GetDefaultSize(double *width, double *height) const {
  *width = impl_->src_width_;
  *height = impl_->src_height_;
}

void ImgElement::SetSrcSize(size_t width, size_t height) {
  // Because image data may shared among elements, we don't think this method
  // is useful, because we may require more memory to store the new resized
  // canvas.
  impl_->src_width_ = width;
  impl_->src_height_ = height;
}

BasicElement *ImgElement::CreateInstance(BasicElement *parent, View *view,
                                         const char *name) {
  return new ImgElement(parent, view, name);
}

} // namespace ggadget
