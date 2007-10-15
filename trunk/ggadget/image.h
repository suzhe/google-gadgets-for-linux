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

#ifndef GGADGETS_IMAGE_H__
#define GGADGETS_IMAGE_H__

#include "element_interface.h"
#include "scriptable_helper.h"

namespace ggadget {

class CanvasInterface;
class GraphicsInterface;
class FileManagerInterface;

class Image {
 public:
  /**
   * Creates a new image from the gadget package or directory.
   * The actual data may be load lazily when the image is first used.
   */
  Image(const GraphicsInterface *graphics,
        FileManagerInterface *file_manager,
        const char *filename,
        bool is_mask);

  /**
   * Creates a new image from raw image data.
   * The image is initialized immediately.
   */
  Image(const GraphicsInterface *graphics,
        const char *data,
        size_t data_size,
        bool is_mask);

  /**
   * Duplicate the image.
   */ 
  Image(const Image &another);

  ~Image();

  /**
   * Get the canvas containing the image data.
   * Note: this method is not a const method because we allow lazy
   * initialization.  A side effect is that you can't use
   * <code>const Image *</code>.
   */
  const CanvasInterface *GetCanvas();

  /**
   * Draw the image on a canvas without stretching. 
   */
  void Draw(CanvasInterface *canvas, double x, double y);

  /**
   * Draw the image on a canvas stretching the image to given width and height.
   */
  void StretchDraw(CanvasInterface *canvas,
                   double x, double y,
                   double width, double height);

  size_t GetWidth();
  size_t GetHeight();

 private:
  // Don't allow assignment.
  void operator=(const Image&);
  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGETS_IMAGE_H__
