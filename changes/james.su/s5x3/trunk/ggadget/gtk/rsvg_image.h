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

#ifndef GGADGET_GTK_RSVG_IMAGE_H__
#define GGADGET_GTK_RSVG_IMAGE_H__

#include <cairo.h>

#include <ggadget/common.h>
#include <ggadget/logger.h>
#include <ggadget/color.h>
#include <ggadget/canvas_interface.h>
#include <ggadget/image_interface.h>

namespace ggadget {
namespace gtk {

class CairoGraphics;

/**
 * This class realizes the ImageInterface using the gdk-pixbuf library.
 */
class RsvgImage : public ImageInterface {
 public:
  RsvgImage(const CairoGraphics *graphics, const std::string &data,
            bool is_mask);

  /** Check if the RsvgImage object is valid. */
  bool IsValid() const;

 public:
  virtual ~RsvgImage();

  virtual void Destroy();
  virtual const CanvasInterface *GetCanvas() const;
  virtual void Draw(CanvasInterface *canvas, double x, double y) const;
  virtual void StretchDraw(CanvasInterface *canvas,
                           double x, double y,
                           double width, double height) const;
  virtual size_t GetWidth() const;
  virtual size_t GetHeight() const;
  virtual void SetColorMultiply(const Color &color);
  virtual bool GetPointValue(double x, double y,
                             Color *color, double *opacity) const;
  virtual void SetTag(const char *tag);
  virtual std::string GetTag() const;

 private:
  class Impl;
  Impl *impl_;

  DISALLOW_EVIL_CONSTRUCTORS(RsvgImage);
};

} // namespace gtk
} // namespace ggadget

#endif // GGADGET_GTK_RSVG_IMAGE_H__
