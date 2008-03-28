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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <gdk/gdkcairo.h>
#include <ggadget/logger.h>
#include <ggadget/color.h>
#include <ggadget/signals.h>
#include "cairo_image_base.h"
#include "cairo_graphics.h"
#include "cairo_canvas.h"

namespace ggadget {
namespace gtk {

class CairoImageBase::Impl {
 public:
  Impl(const CairoGraphics *graphics, const std::string &tag, bool is_mask)
    : graphics_(graphics),
      tag_(tag), is_mask_(is_mask),
      ref_count_(1) {
  }

  ~Impl() {
    ASSERT(ref_count_ == 0);
  }

  const CairoGraphics *graphics_;
  std::string tag_;
  bool is_mask_;
  int ref_count_;
};

CairoImageBase::CairoImageBase(const CairoGraphics *graphics,
                               const std::string &tag, bool is_mask)
    : impl_(new Impl(graphics, tag, is_mask)) {
}

CairoImageBase::~CairoImageBase() {
  impl_->graphics_->OnImageDelete(impl_->tag_, impl_->is_mask_);
  delete impl_;
  impl_ = NULL;
}

void CairoImageBase::Ref() {
  impl_->ref_count_++;
}

void CairoImageBase::Unref() {
  if (--impl_->ref_count_ == 0)
    delete this;
}

void CairoImageBase::Destroy() {
  Unref();
}

void CairoImageBase::Draw(CanvasInterface *canvas, double x, double y) const {
  const CanvasInterface *image = GetCanvas();
  ASSERT(canvas && image);
  if (image && canvas)
    canvas->DrawCanvas(x, y, image);
}

void CairoImageBase::StretchDraw(CanvasInterface *canvas,
                                 double x, double y,
                                 double width, double height) const {
  const CanvasInterface *image = GetCanvas();
  ASSERT(canvas && image);
  double image_width = image->GetWidth();
  double image_height = image->GetHeight();
  if (image && canvas && image_width > 0 && image_height > 0) {
    double cx = width / image_width;
    double cy = height / image_height;
    if (cx != 1.0 || cy != 1.0) {
      canvas->PushState();
      canvas->ScaleCoordinates(cx, cy);
      canvas->DrawCanvas(x / cx, y / cy, image);
      canvas->PopState();
    } else {
      canvas->DrawCanvas(x, y, image);
    }
  }
}

class ColorMultipliedImage : public CairoImageBase {
 public:
  ColorMultipliedImage(const CairoGraphics *graphics,
                       const CairoImageBase *image,
                       const Color &color_multiply)
      : CairoImageBase(graphics, "", false),
        width_(0), height_(0), fully_opaque_(false),
        color_multiply_(color_multiply), canvas_(NULL) {
    if (image) {
      width_ = image->GetWidth();
      height_ = image->GetHeight();
      fully_opaque_ = image->IsFullyOpaque();
      canvas_ = new CairoCanvas(1, width_, height_, CAIRO_FORMAT_ARGB32);
      image->Draw(canvas_, 0, 0);
      canvas_->MultiplyColor(color_multiply_);
    }
  }

  virtual ~ColorMultipliedImage() {
    if (canvas_)
      canvas_->Destroy();
  }

  virtual double GetWidth() const { return width_; }
  virtual double GetHeight() const { return height_; }
  virtual bool IsFullyOpaque() const { return fully_opaque_; }
  virtual bool IsValid() const { return canvas_ != NULL; }
  virtual CanvasInterface *GetCanvas() const { return canvas_; }

  double width_;
  double height_;
  bool fully_opaque_;
  Color color_multiply_;
  CairoCanvas *canvas_;
};

ImageInterface *CairoImageBase::MultiplyColor(const Color &color) const {
  // Because this method is const, we should always return a new image even
  // if the color is pure white.
  return new ColorMultipliedImage(impl_->graphics_, this, color);
}

bool CairoImageBase::GetPointValue(double x, double y,
                                   Color *color, double *opacity) const {
  const CanvasInterface *canvas = GetCanvas();
  return canvas && canvas->GetPointValue(x, y, color, opacity);
}

std::string CairoImageBase::GetTag() const {
  return impl_->tag_;
}

} // namespace gtk
} // namespace ggadget
