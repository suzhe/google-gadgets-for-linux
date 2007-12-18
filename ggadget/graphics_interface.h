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

#ifndef GGADGET_GRAPHICS_INTERFACE_H__
#define GGADGET_GRAPHICS_INTERFACE_H__

#include <ggadget/canvas_interface.h>
#include <ggadget/font_interface.h>

namespace ggadget {

struct Color;

/**
 * This class is the interface for creating objects used in ggadget's
 * graphics rendering. It's implementation should come bundled with a
 * corresponding implementation of CanvasInterface. The gadget view obtains
 * an instance of this class from its HostInterface. Unlike the HostInterface,
 * the host can decide, depending on requirements,
 * how to assign GraphicsInterface objects to Views. For example, the host may
 * choose to:
 * - use a different GraphicsInterface for each view
 * - use a different GraphicsInterface for each gadget, but share it amongst views
 * - use the same GraphicsInterface for all views in the process.
 */
class GraphicsInterface {
 public:
  virtual ~GraphicsInterface() { }

  /**
   * Creates a new blank canvas.
   * @param w Width of the new canvas.
   * @param h Height of the new canvas.
   */
  virtual CanvasInterface *NewCanvas(size_t w, size_t h) const = 0;

  /**
   * Creates a new image canvas.
   * @param img_bytes Array containing the raw bytes of the image.
   * @param img_bytes_count Number of bytes of the img_bytes array.
   * @param colormultiply a color value by which each pixel value is multiplied.
   *   Pass in NULL if no color value should be multiplied. 
   * @return NULL on error, an ImageInterface object otherwise.
   */
  virtual CanvasInterface *NewImage(const char *img_bytes,
                                    size_t img_bytes_count,
                                    const Color *colormultiply) const = 0;

  /**
   * Creates a new image mask canvas.
   * Any black pixels in the mask image are considered to be transparent.
   * @param img_bytes Array containing the raw bytes of the image.
   * @param img_bytes_count Number of bytes of the img_bytes array.
   * @return NULL on error, an ImageInterface object otherwise.
   */
  virtual CanvasInterface *NewMask(const char *img_bytes,
                                   size_t img_bytes_count) const = 0;

  /**
   * Create a new font. This font is used when rendering text to a canvas.
   */
  virtual FontInterface *NewFont(const char *family, size_t pt_size,
                                 FontInterface::Style style,
                                 FontInterface::Weight weight) const = 0;

};

} // namespace ggadget

#endif // GGADGET_GRAPHICS_INTERFACE_H__
