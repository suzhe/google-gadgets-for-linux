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

#ifndef GGADGET_QT_QT_CANVAS_H__
#define GGADGET_QT_QT_CANVAS_H__

#include <stack>

#include <QImage>
#include <QTextDocument>
#include <QTextCursor>
#include <QGLFramebufferObject>
#include <ggadget/common.h>
#include <ggadget/logger.h>
#include <ggadget/canvas_interface.h>

namespace ggadget {
namespace qt {
class QtGraphics;
/**
 * This class realizes the CanvasInterface using the qt graphics library.
 */
class QtCanvas : public CanvasInterface {
 public:
  /**
   * Creates a QtCanvas object which uses a fixed zoom factor.
   */
  QtCanvas(const QtGraphics *g, size_t w, size_t h);
  QtCanvas(const std::string &data);
  QtCanvas(const QtGraphics *g, size_t w, size_t h, QPainter *painter);

  virtual ~QtCanvas();

  virtual void Destroy();

  virtual size_t GetWidth() const;
  virtual size_t GetHeight() const;

  virtual bool PushState();
  virtual bool PopState();

  virtual bool MultiplyOpacity(double opacity);
  virtual void RotateCoordinates(double radians);
  virtual void TranslateCoordinates(double dx, double dy);
  virtual void ScaleCoordinates(double cx, double cy);

  virtual bool ClearCanvas();

  virtual bool DrawLine(double x0, double y0, double x1, double y1,
                        double width, const Color &c);
  virtual bool DrawFilledRect(double x, double y,
                              double w, double h, const Color &c);

  virtual bool DrawCanvas(double x, double y, const CanvasInterface *img);
  virtual bool DrawFilledRectWithCanvas(double x, double y,
                                        double w, double h,
                                        const CanvasInterface *img);
  virtual bool DrawCanvasWithMask(double x, double y,
                                  const CanvasInterface *img,
                                  double mx, double my,
                                  const CanvasInterface *mask);

  virtual bool DrawText(double x, double y, double width, double height,
                        const char *text, const FontInterface *f,
                        const Color &c, Alignment align, VAlignment valign,
                        Trimming trimming, int text_flags);
  virtual bool DrawTextWithTexture(double x, double y, double width,
                                   double height, const char *text,
                                   const FontInterface *f,
                                   const CanvasInterface *texture,
                                   Alignment align, VAlignment valign,
                                   Trimming trimming, int text_flags);

  virtual bool IntersectRectClipRegion(double x, double y,
                                       double w, double h);

  virtual bool GetTextExtents(const char *text, const FontInterface *f,
                              int text_flags, double in_width,
                              double *width, double *height);

  virtual bool GetPointValue(double x, double y,
                             Color *color, double *opacity) const;

 public:
  /**
   * Multiplies a specified color to every pixel in the canvas.
   */
  void MultiplyColor(const Color &color);

  /** Checks if the canvas is valid */
  bool IsValid() const;

  QImage* GetImage() const;
  bool DrawTextDocument(QTextDocument &doc);

 private:
  class Impl;
  friend class Impl;
  Impl *impl_;

  DISALLOW_EVIL_CONSTRUCTORS(QtCanvas);
};

} // namespace qt
} // namespace ggadget

#endif // GGADGET_QT_QT_CANVAS_H__
