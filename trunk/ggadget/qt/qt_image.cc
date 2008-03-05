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
  Impl(const std::string &data) {
    image_ = new QtCanvas(data);
    if (image_->GetWidth() == 0) {
      is_valid_ = false;
      delete image_;
      image_ = NULL;
    } else {
      is_valid_ = true;
    }
  }

  ~Impl() {
  }

  bool IsValid() const {
    return is_valid_;
  }

  const CanvasInterface *GetCanvas() {
    return image_;
  }

  void Draw(CanvasInterface *canvas, double x, double y) {
    ASSERT(canvas && image_);
    canvas->DrawCanvas(x, y, image_);
  }

  void StretchDraw(CanvasInterface *canvas,
                   double x, double y,
                   double width, double height) {
    ASSERT(canvas && image_);
    double cx = width / image_->GetWidth();
    double cy = height / image_->GetHeight();
    if (cx != 1.0 || cy != 1.0) {
      canvas->PushState();
      canvas->ScaleCoordinates(cx, cy);
      canvas->DrawCanvas(x / cx, y / cy, image_);
      canvas->PopState();
    } else {
      canvas->DrawCanvas(x, y, image_);
    }
  }

  size_t GetWidth() const {
    return image_->GetWidth();
  }

  size_t GetHeight() const {
    return image_->GetHeight();
  }

  void SetColorMultiply(const Color &color) {
  }

  bool GetPointValue(double x, double y,
                     Color *color, double *opacity) {
    return image_ && image_->GetPointValue(x, y, color, opacity);
  }

  void SetTag(const char *tag) {
    tag_ = std::string(tag ? tag : "");
  }

  std::string GetTag() const {
    return tag_;
  }

  double zoom_;
  bool is_valid_;
  QtCanvas *image_;
  Color color_multiply_;
  std::string tag_;
  Connection *on_zoom_connection_;
};

QtImage::QtImage(QtGraphics const *graphics,
                 const std::string &data)
  : impl_(new Impl(data)) {
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
