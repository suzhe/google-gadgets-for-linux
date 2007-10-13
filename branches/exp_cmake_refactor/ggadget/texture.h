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

#ifndef GGADGETS_TEXTURE_H__
#define GGADGETS_TEXTURE_H__

#include "element_interface.h"
#include "scriptable_helper.h"

namespace ggadget {

class CanvasInterface;
class GraphicsInterface;
class FileManagerInterface;

class Texture {
 public:
  /**
   * Creates a new image from the gadget package or directory.
   * The actual data may be load lazily when the image is first used.
   * @param graphics
   * @param file_manager
   * @param name the name of an image file within the gadget base path, or a
   *     color description with HTML-style color ("#rrggbb"), or HTML-style
   *     color with alpha ("#rrggbbaa").
   */
  Texture(const GraphicsInterface *graphics,
          FileManagerInterface *file_manager,
          const char *name);

  /**
   * Duplicate the image.
   */ 
  Texture(const Texture &another);

  /**
   * Draws the texture onto a canvas.
   * If the texture is an image, the image is repeated to fill the canvas.
   */ 
  void Draw(CanvasInterface *canvas);

 private:
  // Don't allow assignment.
  void operator=(const Texture&);
  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGETS_IMAGE_H__
