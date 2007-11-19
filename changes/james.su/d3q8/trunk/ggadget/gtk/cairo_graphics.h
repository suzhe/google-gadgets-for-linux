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

#ifndef GGADGET_GTK_CAIRO_GRAPHICS_H__
#define GGADGET_GTK_CAIRO_GRAPHICS_H__

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <ggadget/common.h>
#include <ggadget/graphics_interface.h>

namespace ggadget {

/**
 * This class realizes the GraphicsInterface using the Cairo graphics library.
 * It is responsible for creating CanvasInterface objects for ggadget.
 */
class CairoGraphics : public GraphicsInterface {
 public:
  /**
   * Constructs a CairoGraphics object
   * @param zoom The zoom level for all new canvases.
   */
  CairoGraphics(double zoom);
  virtual ~CairoGraphics() {};

  virtual CanvasInterface *NewCanvas(size_t w, size_t h) const;

  virtual CanvasInterface *NewImage(const char *img_bytes,
                                    size_t img_bytes_count) const;
  virtual CanvasInterface *NewMask(const char *img_bytes,
                                   size_t img_bytes_count) const;

  virtual FontInterface *NewFont(const char *family, size_t pt_size,
                                 FontInterface::Style style,
                                 FontInterface::Weight weight) const;

 private:
   double zoom_;

   /**
    * Load a GdkPixbuf from raw image data.
    * @return NULL on failure, GdkPixbuf otherwise.
    */
   static GdkPixbuf *LoadPixbufFromData(const char *img_bytes,
                                        size_t img_bytes_count);

   DISALLOW_EVIL_CONSTRUCTORS(CairoGraphics);
};

}

#endif // GGADGET_GTK_CAIRO_GRAPHICS_H__
