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
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>
#include <ggadget/logger.h>
#include <ggadget/color.h>
#include "cairo_graphics.h"
#include "cairo_canvas.h"
#include "rsvg_image.h"

namespace ggadget {
namespace gtk {

class RsvgImage::Impl {
 public:
  Impl(const CairoGraphics *graphics, const std::string &data, bool is_mask)
    : width_(0), height_(0), rsvg_(NULL), canvas_(NULL),
      color_multiply_(1, 1, 1), zoom_(graphics->GetZoom()) {
    // RsvgImage doesn't support mask for now.
    ASSERT(!is_mask);
    GError *error = NULL;
    const guint8 *ptr = reinterpret_cast<const guint8*>(data.c_str());
    rsvg_ = rsvg_handle_new_from_data(ptr, data.size(), &error);
    if (error)
      g_error_free(error);
    if (rsvg_) {
      RsvgDimensionData dim;
      rsvg_handle_get_dimensions(rsvg_, &dim);
      width_ = dim.width;
      height_ = dim.height;
      on_zoom_connection_ =
        graphics->ConnectOnZoom(NewSlot(this, &Impl::OnZoom));
    }
  }

  ~Impl() {
    if (rsvg_)
      g_object_unref(rsvg_);
    if (canvas_)
      canvas_->Destroy();
    if (on_zoom_connection_)
      on_zoom_connection_->Disconnect();
  }

  bool IsValid() const {
    return rsvg_;
  }

  void OnZoom(double zoom) {
    if (zoom_ != zoom && zoom > 0) {
      zoom_ = zoom;

      // Destroy the canvas so that it'll be recreated again with new zoom
      // factor when calling GetCanvas().
      if (canvas_) {
        canvas_->Destroy();
        canvas_ = NULL;
      }
    }
  }

  const CanvasInterface *GetCanvas() {
    if (!canvas_ && rsvg_) {
      canvas_ = new CairoCanvas(zoom_, width_, height_, CAIRO_FORMAT_ARGB32);
      if (canvas_) {
        // Draw the image onto the canvas.
        cairo_t *cr = canvas_->GetContext();
        rsvg_handle_render_cairo(rsvg_, cr);
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
    ASSERT(canvas);
    if (canvas && rsvg_) {
      // If no stretch and no color multiply,
      // then use catched canvas to improve performance.
      // Otherwise draw rsvg directly onto the canvas to get better effect.
      if (size_t(round(width)) == width_ && size_t(round(height)) == height_) {
        const CanvasInterface *image = GetCanvas();
        ASSERT(image);
        if (image)
          canvas->DrawCanvas(x, y, image);
      } else {
        double cx = width / width_;
        double cy = height / height_;
        canvas->PushState();
        canvas->IntersectRectClipRegion(x, y, width, height);
        canvas->TranslateCoordinates(x, y);
        canvas->ScaleCoordinates(cx, cy);
        CairoCanvas *cc = down_cast<CairoCanvas*>(canvas);
        if (color_multiply_ == Color::kWhite && cc) {
          rsvg_handle_render_cairo(rsvg_, cc->GetContext());
        } else {
          canvas->DrawCanvas(0, 0, GetCanvas());
        }
        canvas->PopState();
      }
    }
  }

  size_t GetWidth() const { return width_; }

  size_t GetHeight() const { return height_; }

  void SetColorMultiply(const Color &color) {
    if (color != color_multiply_) {
      color_multiply_ = color;

      // Destroy the canvas so that it'll be recreated again with
      // new color multiply when calling GetCanvas().
      if (canvas_) {
        canvas_->Destroy();
        canvas_ = NULL;
      }
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

  size_t width_;
  size_t height_;
  RsvgHandle *rsvg_;
  CairoCanvas *canvas_;
  Color color_multiply_;
  double zoom_;
  Connection *on_zoom_connection_;
  std::string tag_;
};

RsvgImage::RsvgImage(const CairoGraphics *graphics,
                     const std::string &data, bool is_mask)
  : impl_(new Impl(graphics, data, is_mask)) {
}

RsvgImage::~RsvgImage() {
  delete impl_;
  impl_ = NULL;
}

bool RsvgImage::IsValid() const {
  return impl_->IsValid();
}

void RsvgImage::Destroy() {
  delete this;
}

const CanvasInterface *RsvgImage::GetCanvas() const {
  return impl_->GetCanvas();
}

void RsvgImage::Draw(CanvasInterface *canvas, double x, double y) const {
  impl_->Draw(canvas, x, y);
}

void RsvgImage::StretchDraw(CanvasInterface *canvas,
                            double x, double y,
                            double width, double height) const {
  impl_->StretchDraw(canvas, x, y, width, height);
}

size_t RsvgImage::GetWidth() const {
  return impl_->GetWidth();
}

size_t RsvgImage::GetHeight() const {
  return impl_->GetHeight();
}

void RsvgImage::SetColorMultiply(const Color &color) {
  impl_->SetColorMultiply(color);
}

bool RsvgImage::GetPointValue(double x, double y,
                              Color *color, double *opacity) const {
  return impl_->GetPointValue(x, y, color, opacity);
}

void RsvgImage::SetTag(const char *tag) {
  impl_->SetTag(tag);
}

std::string RsvgImage::GetTag() const {
  return impl_->GetTag();
}

} // namespace gtk
} // namespace ggadget
