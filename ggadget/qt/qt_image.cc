/*
  Copyright 2008 Google Inc.

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
#include <QtGui/QPixmap>
#include <ggadget/logger.h>
#include <ggadget/color.h>
#include "qt_graphics.h"
#include "qt_canvas.h"
#include "qt_image.h"

namespace ggadget {
namespace qt {
static void QImageMultiplyColor(QImage* dest,
                                const QImage *src,
                                const Color &c) {
  int width = src->width();
  int height = src->height();
  int rm = c.RedInt();
  int gm = c.GreenInt();
  int bm = c.BlueInt();
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      QRgb rgb = src->pixel(x, y);
      if (qAlpha(rgb) == 0) {
        dest->setPixel(x, y, qRgba(0, 0, 0, 0));
      } else {
        int r = (qRed(rgb) * rm) >> 8;
        int g = (qGreen(rgb) * gm) >> 8;
        int b = (qBlue(rgb) * bm) >> 8;
        dest->setPixel(x, y, qRgb(r, g, b));
      }
    }
  }
}

class QtImage::Impl {
 public:
  Impl(QtGraphics *g, const char *tag,
       const std::string &data, bool is_mask)
    : is_mask_(is_mask),
      canvas_(NULL),
      tag_(tag),
      graphics_(g),
      fully_opaque_(false) {
    canvas_ = new QtCanvas(data);
    if (!canvas_) return;
    if (canvas_->GetWidth() == 0) {
      delete canvas_;
      canvas_ = NULL;
      return;
    }
    QImage *img = canvas_->GetImage();
    if (is_mask) {
      // Setup alpha channel and black color will be set to fully transparent
      QImage mask = img->createMaskFromColor(qRgb(0, 0, 0), Qt::MaskOutColor);
      img->setAlphaChannel(mask);
    } else if (!canvas_->GetImage()->hasAlphaChannel()) {
      fully_opaque_ = true;
    } else {
      for (int y = 0; y < img->height() && fully_opaque_; y++) {
        for (int x = 0; x < img->width(); x++) {
          QRgb rgb = img->pixel(x, y);
          if (qAlpha(rgb) != 255) {
            fully_opaque_ = false;
            break;
          }
        }
      }
    }
  }

  Impl(size_t width, size_t height)
    : is_mask_(false),
      canvas_(NULL),
      graphics_(NULL) {
    canvas_ = new QtCanvas(NULL, width, height);
    if (!canvas_) return;
    if (canvas_->GetWidth() == 0) {
      delete canvas_;
      canvas_ = NULL;
    }
  }

  ~Impl() {
    if (canvas_) delete canvas_;
  }

  void Draw(CanvasInterface *canvas, double x, double y) {
    ASSERT(canvas && canvas_);
    canvas->DrawCanvas(x, y, canvas_);
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

  bool is_mask_;
  QtCanvas *canvas_;
  std::string tag_;
  QtGraphics *graphics_;
  bool fully_opaque_;
};

QtImage::QtImage(QtGraphics *graphics,
                 const char *tag,
                 const std::string &data,
                 bool is_mask)
  : impl_(new Impl(graphics, tag, data, is_mask)) {
}

QtImage::QtImage(size_t width, size_t height)
  : impl_(new Impl(width, height)) {
}

QtImage::~QtImage() {
  delete impl_;
  impl_ = NULL;
}

bool QtImage::IsValid() const {
  return impl_->canvas_ != NULL;
}

void QtImage::Destroy() {
  delete this;
}

const CanvasInterface *QtImage::GetCanvas() const {
  return impl_->canvas_;
}

void QtImage::Draw(CanvasInterface *canvas, double x, double y) const {
  impl_->Draw(canvas, x, y);
}

void QtImage::StretchDraw(CanvasInterface *canvas,
                              double x, double y,
                              double width, double height) const {
  impl_->StretchDraw(canvas, x, y, width, height);
}

double QtImage::GetWidth() const {
  return impl_->canvas_->GetWidth();
}

double QtImage::GetHeight() const {
  return impl_->canvas_->GetHeight();
}

ImageInterface* QtImage::MultiplyColor(const Color &color) const {

  QtImage *new_image = new QtImage(D2I(GetWidth()), D2I(GetHeight()));
  if (!new_image) return NULL;
  if (!new_image->IsValid()) {
    delete new_image;
    return NULL;
  }
  new_image->impl_->fully_opaque_ = impl_->fully_opaque_;
  QImageMultiplyColor(new_image->impl_->canvas_->GetImage(),
                      impl_->canvas_->GetImage(),
                      color);
  return new_image;
}

bool QtImage::GetPointValue(double x, double y,
                                Color *color, double *opacity) const {
  return impl_->canvas_ && impl_->canvas_->GetPointValue(x, y, color, opacity);
}

std::string QtImage::GetTag() const {
  return impl_->tag_;
}

bool QtImage::IsFullyOpaque() const {
  return impl_->fully_opaque_;
}

} // namespace qt
} // namespace ggadget
