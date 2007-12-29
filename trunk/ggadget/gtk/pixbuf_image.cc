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
#include <gdk/gdkcairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <ggadget/logger.h>
#include <ggadget/color.h>
#include "cairo_graphics.h"
#include "cairo_canvas.h"
#include "pixbuf_image.h"
#include "pixbuf_utils.h"

namespace ggadget {
namespace gtk {

class PixbufImage::Impl {
 public:
  Impl(const CairoGraphics *graphics, const std::string &data, bool is_mask)
    : is_mask_(is_mask), width_(0), height_(0), pixbuf_(NULL),
      canvas_(NULL), color_multiply_(1, 1, 1) {
    // No zoom for PixbufImage.
    pixbuf_ = LoadPixbufFromData(data);
    if (pixbuf_) {
      width_ = gdk_pixbuf_get_width(pixbuf_);
      height_ = gdk_pixbuf_get_height(pixbuf_);
      if (is_mask) {
        // clone pixbuf with alpha channel and free the old one.
        // black color will be set to fully transparent.
        GdkPixbuf *a_pixbuf = gdk_pixbuf_add_alpha(pixbuf_, TRUE, 0, 0, 0);
        g_object_unref(pixbuf_);
        pixbuf_ = a_pixbuf;
        // Initialize the canvas.
        (void) GetCanvas();
        // It's ok to free pixbuf data for a mask, because it will be
        // readonly afterwards.
        g_object_unref(pixbuf_);
        pixbuf_ = NULL;
      }
    }
  }

  ~Impl() {
    if (pixbuf_) g_object_unref(pixbuf_);
    if (canvas_) canvas_->Destroy();
  }

  bool IsValid() const {
    return pixbuf_ || canvas_;
  }

  const CanvasInterface *GetCanvas() {
    if (!canvas_ && pixbuf_) {
      cairo_format_t fmt = (is_mask_ ? CAIRO_FORMAT_A8 : CAIRO_FORMAT_ARGB32);
      // No zoom for Image canvas.
      canvas_ = new CairoCanvas(1, width_, height_, fmt);
      if (canvas_) {
        // Draw the image onto the canvas.
        cairo_t *cr = canvas_->GetContext();
        gdk_cairo_set_source_pixbuf(cr, pixbuf_, 0, 0);
        cairo_paint(cr);
        cairo_set_source_rgba(cr, 0., 0., 0., 0.);
        if (!is_mask_)
          canvas_->MultiplyColor(color_multiply_);
      }
    }
    return canvas_;
  }

  void Draw(CanvasInterface *canvas, double x, double y) {
    const CanvasInterface *image = GetCanvas();
    ASSERT(canvas && image);
    if (image && canvas)
      canvas->DrawCanvas(x, y, image);
  }

  void StretchDraw(CanvasInterface *canvas,
                   double x, double y,
                   double width, double height) {
    const CanvasInterface *image = GetCanvas();
    ASSERT(canvas && image);
    if (image && canvas) {
      double cx = width / image->GetWidth();
      double cy = height / image->GetHeight();
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

  size_t GetWidth() const { return width_; }

  size_t GetHeight() const { return height_; }

  void SetColorMultiply(const Color &color) {
    if (!is_mask_ && color != color_multiply_) {
      // If the canvas is not created yet, color multiply will be
      // applied when creating the canvas.
      if (canvas_) {
        // If the previous color multiply is not white then the canvas needs
        // redrawing.
        if (color_multiply_ != Color::kWhite) {
          canvas_->ClearCanvas();
          cairo_t *cr = canvas_->GetContext();
          gdk_cairo_set_source_pixbuf(cr, pixbuf_, 0, 0);
          cairo_paint(cr);
          cairo_set_source_rgba(cr, 0., 0., 0., 0.);
        }
        canvas_->MultiplyColor(color);
      }
      color_multiply_ = color;
    }
  }

  bool GetPointValue(double x, double y,
                     Color *color, double *opacity) {
    const CanvasInterface *canvas = GetCanvas();
    return canvas && canvas->GetPointValue(x, y, color, opacity);
  }

  void SetTag(const char *tag) {
    tag_ = std::string(tag ? tag : "");
  }

  std::string GetTag() const {
    return tag_;
  }

  bool is_mask_;
  size_t width_;
  size_t height_;
  GdkPixbuf *pixbuf_;
  CairoCanvas *canvas_;
  Color color_multiply_;
  std::string tag_;
};

PixbufImage::PixbufImage(const CairoGraphics *graphics,
                         const std::string &data, bool is_mask)
  : impl_(new Impl(graphics, data, is_mask)) {
}

PixbufImage::~PixbufImage() {
  delete impl_;
  impl_ = NULL;
}

bool PixbufImage::IsValid() const {
  return impl_->IsValid();
}

void PixbufImage::Destroy() {
  delete this;
}

const CanvasInterface *PixbufImage::GetCanvas() const {
  return impl_->GetCanvas();
}

void PixbufImage::Draw(CanvasInterface *canvas, double x, double y) const {
  impl_->Draw(canvas, x, y);
}

void PixbufImage::StretchDraw(CanvasInterface *canvas,
                              double x, double y,
                              double width, double height) const {
  impl_->StretchDraw(canvas, x, y, width, height);
}

size_t PixbufImage::GetWidth() const {
  return impl_->GetWidth();
}

size_t PixbufImage::GetHeight() const {
  return impl_->GetHeight();
}

void PixbufImage::SetColorMultiply(const Color &color) {
  impl_->SetColorMultiply(color);
}

bool PixbufImage::GetPointValue(double x, double y,
                                Color *color, double *opacity) const {
  return impl_->GetPointValue(x, y, color, opacity);
}

void PixbufImage::SetTag(const char *tag) {
  impl_->SetTag(tag);
}

std::string PixbufImage::GetTag() const {
  return impl_->GetTag();
}

} // namespace gtk
} // namespace ggadget
