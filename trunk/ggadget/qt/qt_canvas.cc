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
#include <QImage>
#include <QPainter>
#include <ggadget/scoped_ptr.h>
#include <ggadget/signals.h>
#include <ggadget/slot.h>
#include <ggadget/math_utils.h>
#include "qt_graphics.h"
#include "qt_canvas.h"
#include "qt_font.h"

#if 1
#undef DLOG
#define DLOG  true ? (void) 0 : LOG
#endif

namespace ggadget {
namespace qt {

const char *const kEllipsisText = "...";
static inline int D2I(double d) { return static_cast<int>(round(d)); }

class QtCanvas::Impl {
 public:
  Impl(size_t w, size_t h)
    : width_(w), height_(h), opacity_(1.),
      image_(w, h, QImage::Format_ARGB32) {
    image_.fill(Qt::transparent);
    painter_ = new QPainter(&image_);
    painter_->setRenderHint(QPainter::SmoothPixmapTransform, true);
    not_free_painter_ = false;
  }

  Impl(const std::string &data) {
    bool ret = image_.loadFromData(
        reinterpret_cast<const uchar *>(data.c_str()),
        data.length());
    if (ret) {
      width_ = image_.width();
      height_ = image_.height();
      painter_ = new QPainter(&image_);
      painter_->setRenderHint(QPainter::SmoothPixmapTransform, true);
    } else {
      width_ = height_ = 0;
      painter_ = NULL;
    }
    not_free_painter_ = false;
  }

  Impl(size_t w, size_t h, QPainter *painter)
    : width_(w), height_(h), opacity_(1.), painter_(painter) {
    painter_->setRenderHint(QPainter::SmoothPixmapTransform, true);
    not_free_painter_ = true;
  }

  ~Impl() {
    if (painter_ && !not_free_painter_) delete painter_;
  }

  QImage* GetImage() { return &image_; }

  size_t GetWidth() { return width_; }

  size_t GetHeight() {  return height_;  }

  bool PushState() {
    painter_->save();
    return true;
  }

  bool PopState() {
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
    painter_->rotate(RadiansToDegrees(radians));
  }

  void TranslateCoordinates(double dx, double dy) {
    painter_->translate(dx, dy);
  }

  void ScaleCoordinates(double cx, double cy) {
    painter_->scale(cx, cy);
  }

  bool ClearCanvas() {
    DLOG("ClearCanvas");
    painter_->eraseRect(0, 0, width_, height_);
    return true;
  }

  bool DrawTextDocument(QTextDocument &doc) {
    doc.drawContents(painter_, QRect(0, 0, GetWidth(), GetHeight()));
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
    DLOG("DrawFilledRect:%p", this);
    QPainter *p = painter_;
    QColor color(c.RedInt(), c.GreenInt(), c.BlueInt());
    p->fillRect(D2I(x), D2I(y), D2I(w), D2I(h), color);
    return true;
  }

  bool DrawCanvas(double x, double y, const CanvasInterface *img) {
    DLOG("DrawCanvas:%p on %p", img, this);
    QPainter *p = painter_;
    const QtCanvas *canvas = reinterpret_cast<const QtCanvas*>(img);
    p->drawImage(D2I(x), D2I(y), *canvas->GetImage());
    return true;
  }

  bool DrawFilledRectWithCanvas(double x, double y, double w, double h,
                                const CanvasInterface *img) {
    DLOG("DrawFilledRectWithCanvas: %p on %p", img, this);
    QPainter *p = painter_;
    const QtCanvas *canvas = reinterpret_cast<const QtCanvas*>(img);
    p->fillRect(D2I(x), D2I(y), D2I(w), D2I(h), *canvas->GetImage());
    return true;
  }

  bool DrawCanvasWithMask(double x, double y,
                          const CanvasInterface *img,
                          double mx, double my,
                          const CanvasInterface *mask) {
    DLOG("DrawCanvasWithMask: (%p, %p) on %p", img, mask, this);
    QPainter *p = painter_;
    const QtCanvas *s = reinterpret_cast<const QtCanvas*>(img);
    const QtCanvas *m = reinterpret_cast<const QtCanvas*>(mask);
    QImage simg = *s->GetImage();
    simg.setAlphaChannel(*m->GetImage());
    p->drawImage(D2I(x), D2I(y), simg);
    return true;
  }

  bool DrawText(double x, double y, double width, double height,
                const char *text, const FontInterface *f,
                const Color &c, Alignment align, VAlignment valign,
                Trimming trimming, int text_flags) {
    DLOG("DrawText:%s", text);
    QPainter *p = painter_;
    QTextDocument doc(text);

    const QtFont *qtfont = down_cast<const QtFont*>(f);
    QFont font = *qtfont->GetQFont();
    if (text_flags & TEXT_FLAGS_UNDERLINE)
      font.setUnderline(true);
    else
      font.setUnderline(false);
    if (text_flags & TEXT_FLAGS_STRIKEOUT)
      font.setStrikeOut(true);
    else
      font.setStrikeOut(false);
    doc.setDefaultFont(font);

    QTextOption option;

    if (align == ALIGN_RIGHT) option.setAlignment(Qt::AlignRight);
    if (align == ALIGN_LEFT) option.setAlignment(Qt::AlignLeft);
    if (align == ALIGN_CENTER) option.setAlignment(Qt::AlignHCenter);

    if (valign == VALIGN_TOP) option.setAlignment(Qt::AlignTop);
    if (valign == VALIGN_BOTTOM) option.setAlignment(Qt::AlignBottom);
    if (valign == VALIGN_MIDDLE) option.setAlignment(Qt::AlignVCenter);
    if (text_flags & TEXT_FLAGS_WORDWRAP)
      option.setWrapMode(QTextOption::WordWrap);

    doc.setDefaultTextOption(option);
    doc.setTextWidth(width);

    p->setPen(QColor(c.RedInt(), c.GreenInt(), c.BlueInt()));
    doc.drawContents(p, QRect(D2I(x), D2I(y), D2I(width), D2I(height)));
    return true;
  }

  bool DrawTextWithTexture(double x, double y, double width,
                           double height, const char *text,
                           const FontInterface *f,
                           const CanvasInterface *texture,
                           Alignment align, VAlignment valign,
                           Trimming trimming, int text_flags) {
    DLOG("DrawTextWithTexture: %s", text);
    return true;
  }

  bool IntersectRectClipRegion(double x, double y,
                               double w, double h) {
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
    // Canvas without image_ doesn't support GetPointValue
    if (not_free_painter_) return false;

    int xi = D2I(x);
    int yi = D2I(y);
    if (xi < 0 || xi >= width_ || yi < 0 || yi >= height_) return false;
    QColor qcolor = image_.pixel(xi, yi);
    if (color) {
      color->red = qcolor.redF();
      color->green = qcolor.greenF();
      color->blue = qcolor.blueF();
    }

    if (opacity) *opacity = qcolor.alphaF();

    return true;
  }

  int width_, height_;
  double opacity_;
  double zoom_;
  bool not_free_painter_;
  QPainter *painter_;
  QImage image_;
  Connection *on_zoom_connection_;
};

QtCanvas::QtCanvas(const QtGraphics *g, size_t w, size_t h) : impl_(new Impl(w, h)) { }
QtCanvas::QtCanvas(const QtGraphics *g, size_t w, size_t h, QPainter *painter)
  : impl_(new Impl(w, h, painter)) { }

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

bool QtCanvas::DrawTextDocument(QTextDocument &doc) {
  return impl_->DrawTextDocument(doc);
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

QImage* QtCanvas::GetImage() const {
  return impl_->GetImage();
}

} // namespace qt
} // namespace ggadget
