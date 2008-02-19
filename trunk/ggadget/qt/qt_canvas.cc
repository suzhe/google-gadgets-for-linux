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
#include <vector>
#include <stack>
#include <algorithm>
#include <QPixmap>
#include <QPainter>
#include <ggadget/scoped_ptr.h>
#include <ggadget/signals.h>
#include <ggadget/slot.h>
#include "qt_graphics.h"
#include "qt_canvas.h"
#include "qt_font.h"

namespace ggadget {
namespace qt {

const char *const kEllipsisText = "...";
static inline int D2I(double d) { return static_cast<int>(round(d)); }

class QtCanvas::Impl {
  static int seq;
 public:
  Impl(size_t w, size_t h)
    : width_(w), height_(h), opacity_(1.), pixmap_(w, h) {
    pixmap_.fill(Qt::transparent);
    painter_ = new QPainter(&pixmap_);
    painter_->setRenderHint(QPainter::SmoothPixmapTransform, true);
  }
  Impl(const std::string &data) {
    bool ret = pixmap_.loadFromData(
            reinterpret_cast<const uchar *>(data.c_str()),
            data.length());
    if (ret) {
      width_ = pixmap_.width();
      height_ = pixmap_.height();
      painter_ = new QPainter(&pixmap_);
      painter_->setRenderHint(QPainter::SmoothPixmapTransform, true);
    } else {
      width_ = height_ = 0;
      painter_ = NULL;
    }
  }
  ~Impl() {
    if (painter_) delete painter_;
  }

  QPixmap* GetPixmap() { return &pixmap_; }
  size_t GetWidth() { 
    return pixmap_.width();
  }
  size_t GetHeight() {
    return pixmap_.height();
  }
  bool PushState() {
    LOG("Push:%p", this);
    painter_->save();
    return true;
  }
  bool PopState() {
    LOG("Pop:%p", this);
    painter_->restore();
    return true;
  }

  bool MultiplyOpacity(double opacity) {
    if (opacity >= 0.0 && opacity <= 1.0) {
      painter_->setOpacity(painter_->opacity()*opacity);
      return true;
    }
    return false;
  }
  void RotateCoordinates(double radians) {
    painter_->rotate(radians*180/3.1415926);
  }
  void TranslateCoordinates(double dx, double dy) {
    painter_->translate(dx, dy);
  }
  void ScaleCoordinates(double cx, double cy) {
    painter_->scale(cx, cy);
  }

  bool ClearCanvas() {
    return true;
  }

  bool DrawLine(double x0, double y0, double x1, double y1,
                double width, const Color &c) {
    QPainter *p = painter_;
    QColor color(c.RedInt(), c.GreenInt(), c.BlueInt());
    QPen pen(color);
    pen.setWidthF(width);
    p->setPen(pen);
    p->drawLine(D2I(x0), D2I(y0), D2I(x1), D2I(y1));
    return true;
  }

  bool DrawFilledRect(double x, double y, double w, double h, const Color &c) {
    LOG("DrawFilledRect: %p", this);
    QPainter *p = painter_;
    QColor color(c.RedInt(), c.GreenInt(), c.BlueInt());
    p->fillRect(D2I(x), D2I(y), D2I(w), D2I(h), color); 
    return true;
  }

  bool DrawCanvas(double x, double y, const CanvasInterface *img) {
    QPainter *p = painter_;
    const QtCanvas *canvas = reinterpret_cast<const QtCanvas*>(img);
    p->drawPixmap(D2I(x), D2I(y), *canvas->GetPixmap());
    return true;
  }

  bool DrawFilledRectWithCanvas(double x, double y, double w, double h,
                                const CanvasInterface *img) {
    QPainter *p = painter_;
    const QtCanvas *canvas = reinterpret_cast<const QtCanvas*>(img);
    p->fillRect(D2I(x), D2I(y), D2I(w), D2I(h), *canvas->GetPixmap());
    return true;
  }

  bool DrawCanvasWithMask(double x, double y,
                          const CanvasInterface *img,
                          double mx, double my,
                          const CanvasInterface *mask) {
    QPainter *p = painter_;
    const QtCanvas *s = reinterpret_cast<const QtCanvas*>(img);
    const QtCanvas *m = reinterpret_cast<const QtCanvas*>(mask);
    QPixmap simg = *s->GetPixmap();
    simg.setAlphaChannel(*m->GetPixmap());
    p->drawPixmap(D2I(x), D2I(y), simg);
    return true;
  }
  
  bool DrawText(double x, double y, double width, double height,
                const char *text, const FontInterface *f,
                const Color &c, Alignment align, VAlignment valign,
                Trimming trimming, int text_flags) {
    QPainter *p = painter_;

    int flags = 0;
    if (align == ALIGN_RIGHT) flags |= Qt::AlignRight;
    if (align == ALIGN_LEFT) flags |= Qt::AlignLeft;
    if (align == ALIGN_CENTER) flags |= Qt::AlignHCenter;

    if (valign == VALIGN_TOP) flags |= Qt::AlignTop;
    if (valign == VALIGN_BOTTOM) flags |= Qt::AlignBottom;
    if (valign == VALIGN_MIDDLE) flags |= Qt::AlignVCenter;

    
    p->setPen(QColor(c.RedInt(), c.GreenInt(), c.BlueInt()));

    const QtFont *qt_font = reinterpret_cast<const QtFont*>(f);
    p->setFont(*qt_font->GetQFont());

    p->drawText(D2I(x), D2I(y), D2I(width), D2I(height), flags, text);
    return true;
  }

  bool DrawTextWithTexture(double x, double y, double width,
                           double height, const char *text,
                           const FontInterface *f,
                           const CanvasInterface *texture,
                           Alignment align, VAlignment valign,
                           Trimming trimming, int text_flags) {
    LOG("DrawTextWithTexture: %s", text);
    return true;
  }

  bool IntersectRectClipRegion(double x, double y,
                               double w, double h) {
    LOG("Clip:%p %f %f %f %f", this, x, y, w, h);
    if (w <= 0.0 || h <= 0.0) {
      return false;
    }
    painter_->setClipping(true);
    painter_->setClipRect(D2I(x), D2I(y), D2I(w), D2I(h), Qt::IntersectClip);
    return true;
  }

  bool GetTextExtents(const char *text, const FontInterface *f,
                      int text_flags, double in_width,
                      double *width, double *height) const {
    const QtFont *qt_font = reinterpret_cast<const QtFont*>(f);
    return qt_font->GetTextExtents(text, width, height);
  }
  bool GetPointValue(double x, double y,
                     Color *color, double *opacity) const {
    LOG("GetPointValue");
    return true;
  }

  size_t width_, height_;
  double opacity_;
  double zoom_;
  QPainter *painter_;
  QTransform matrix_;
  std::stack<QTransform> states_;
  std::stack<double> opacity_states_;
  std::stack<bool> clipping_states_;
  std::stack<double> clipping_values_;
  QPixmap pixmap_;
  bool clipping_;
  double clipping_x_, clipping_y_, clipping_w_, clipping_h_;
  Connection *on_zoom_connection_;
};

QtCanvas::QtCanvas(const QtGraphics *g, size_t w, size_t h) : impl_(new Impl(w, h)) { }

QtCanvas::QtCanvas(const std::string &data) : impl_(new Impl(data)) 
 { }                                                        

QtCanvas::~QtCanvas() {
  delete impl_;
  impl_ = NULL;
}

void QtCanvas::Destroy() {
  delete this;
}

bool QtCanvas::ClearCanvas() {
  return impl_->ClearCanvas();
}

bool QtCanvas::PopState() {
  return impl_->PopState();
}

bool QtCanvas::PushState() {
  return impl_->PushState();
}

bool QtCanvas::MultiplyOpacity(double opacity) {
  return impl_->MultiplyOpacity(opacity);
}

bool QtCanvas::DrawLine(double x0, double y0, double x1, double y1,
                        double width, const Color &c) {
  return impl_->DrawLine(x0, y0, x1, y1, width, c);
}

void QtCanvas::RotateCoordinates(double radians) {
   return impl_->RotateCoordinates(radians);
}

void QtCanvas::TranslateCoordinates(double dx, double dy) {
  return impl_->TranslateCoordinates(dx, dy);
}

void QtCanvas::ScaleCoordinates(double cx, double cy) {
  return impl_->ScaleCoordinates(cx, cy);
}

bool QtCanvas::DrawFilledRect(double x, double y,
                              double w, double h, const Color &c) {
  return impl_->DrawFilledRect(x, y, w, h, c);
}

bool QtCanvas::IntersectRectClipRegion(double x, double y,
                                          double w, double h) {
  return impl_->IntersectRectClipRegion(x, y, w, h);
}

bool QtCanvas::DrawCanvas(double x, double y, const CanvasInterface *img) {
  return impl_->DrawCanvas(x, y, img);
}

bool QtCanvas::DrawFilledRectWithCanvas(double x, double y,
                                           double w, double h,
                                           const CanvasInterface *img) {
  return impl_->DrawFilledRectWithCanvas(x, y, w, h, img);
}

bool QtCanvas::DrawCanvasWithMask(double x, double y,
                                    const CanvasInterface *img,
                                    double mx, double my,
                                    const CanvasInterface *mask) {
  return impl_->DrawCanvasWithMask(x, y, img, mx, my, mask);
}

bool QtCanvas::DrawText(double x, double y, double width, double height,
                           const char *text, const FontInterface *f,
                           const Color &c, Alignment align, VAlignment valign,
                           Trimming trimming,  int text_flags) {
  return impl_->DrawText(x, y, width, height, text, f,
                         c, align, valign, trimming, text_flags);
}


bool QtCanvas::DrawTextWithTexture(double x, double y, double w, double h,
                                   const char *text,
                                   const FontInterface *f,
                                   const CanvasInterface *texture,
                                   Alignment align, VAlignment valign,
                                   Trimming trimming, int text_flags) {
  return impl_->DrawTextWithTexture(x, y, w, h, text, f, texture,
                                    align, valign, trimming, text_flags);
}

bool QtCanvas::GetTextExtents(const char *text, const FontInterface *f,
                                 int text_flags, double in_width,
                                 double *width, double *height) {
  return impl_->GetTextExtents(text, f, text_flags, in_width, width, height);
}

bool QtCanvas::GetPointValue(double x, double y,
                                Color *color, double *opacity) const {
  return impl_->GetPointValue(x, y, color, opacity);
}

size_t QtCanvas::GetWidth() const {
  return impl_->GetWidth();
}

size_t QtCanvas::GetHeight() const {
  return impl_->GetHeight();
}

bool QtCanvas::IsValid() const {
  return true;
}

QPixmap* QtCanvas::GetPixmap() const {
  return impl_->GetPixmap();
}

int QtCanvas::Impl::seq = 0;
} // namespace qt
} // namespace ggadget
