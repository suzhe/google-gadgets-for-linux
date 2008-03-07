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

#include <cmath>
#include <string>
#include <QPixmap>
#include <ggadget/logger.h>
#include <ggadget/color.h>
#include "qt_graphics.h"
#include "qt_canvas.h"
#include "qt_image.h"

namespace ggadget {
namespace qt {

class QtImage::Impl {
 public:
  Impl(const std::string &data, bool is_mask) : is_mask_(is_mask) {
    canvas_ = new QtCanvas(data);
    if (canvas_->GetWidth() == 0) {
      is_valid_ = false;
      delete canvas_;
      canvas_ = NULL;
    } else {
      is_valid_ = true;
    }
    orig_canvas_ = canvas_;
  }

  ~Impl() {
    if (orig_canvas_ != canvas_) delete orig_canvas_;
    if (canvas_) delete canvas_;
  }

  bool IsValid() const {
    return is_valid_;
  }

  const CanvasInterface *GetCanvas() {
    return canvas_;
  }

  void Draw(CanvasInterface *canvas, double x, double y) {
    ASSERT(canvas && canvas_);
    canvas->PushState();
    canvas->DrawCanvas(x, y, canvas_);
    canvas->PopState();
  }

  void StretchDraw(CanvasInterface *canvas,
                   double x, double y,
                   double width, double height) {
    ASSERT(canvas && canvas_);
    double cx = width / canvas_->GetWidth();
    double cy = height / canvas_->GetHeight();
    if (cx != 1.0 || cy != 1.0) {
      canvas->PushState();
      canvas->ScaleCoordinates(cx, cy);
      canvas->DrawCanvas(x / cx, y / cy, canvas_);
      canvas->PopState();
    } else {
      Draw(canvas, x, y);
    }
  }

  size_t GetWidth() const {
    return canvas_->GetWidth();
  }

  size_t GetHeight() const {
    return canvas_->GetHeight();
  }

  void SetColorMultiply(const Color &color) {
    if (!is_mask_ && color != color_multiply_) {
      color_multiply_ = color;
      // Try to make a copy if not have yet
      if (canvas_ == orig_canvas_) {
        canvas_ = new QtCanvas(NULL, GetWidth(), GetHeight());
        if (canvas_ == NULL) {
          canvas_ = orig_canvas_;
          return;
        }
      }
      canvas_->MultiplyColor(orig_canvas_, color);
    }
  }

  bool GetPointValue(double x, double y,
                     Color *color, double *opacity) {
    return canvas_ && canvas_->GetPointValue(x, y, color, opacity);
  }

  void SetTag(const char *tag) {
    tag_ = std::string(tag ? tag : "");
  }

  std::string GetTag() const {
    return tag_;
  }

  double zoom_;
  bool is_valid_;
  bool is_mask_;
  // If color multiply is applied to image, we keep the original copy in
  // orig_canvas_.
  QtCanvas *canvas_, *orig_canvas_;
  Color color_multiply_;
  std::string tag_;
  Connection *on_zoom_connection_;
};

QtImage::QtImage(QtGraphics const *graphics,
                 const std::string &data,
                 bool is_mask)
  : impl_(new Impl(data, is_mask)) {
}

QtImage::~QtImage() {
  delete impl_;
  impl_ = NULL;
}

bool QtImage::IsValid() const {
  return impl_->IsValid();
}

void QtImage::Destroy() {
  delete this;
}

const CanvasInterface *QtImage::GetCanvas() const {
  return impl_->GetCanvas();
}

void QtImage::Draw(CanvasInterface *canvas, double x, double y) const {
  impl_->Draw(canvas, x, y);
}

void QtImage::StretchDraw(CanvasInterface *canvas,
                              double x, double y,
                              double width, double height) const {
  impl_->StretchDraw(canvas, x, y, width, height);
}

size_t QtImage::GetWidth() const {
  return impl_->GetWidth();
}

size_t QtImage::GetHeight() const {
  return impl_->GetHeight();
}

void QtImage::SetColorMultiply(const Color &color) {
  impl_->SetColorMultiply(color);
}

bool QtImage::GetPointValue(double x, double y,
                                Color *color, double *opacity) const {
  return impl_->GetPointValue(x, y, color, opacity);
}

void QtImage::SetTag(const char *tag) {
  impl_->SetTag(tag);
}

std::string QtImage::GetTag() const {
  return impl_->GetTag();
}

bool QtImage::IsFullyOpaque() const {
  // TODO
  return false;
}

} // namespace gtk
} // namespace ggadget
