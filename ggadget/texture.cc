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

#include "texture.h"
#include "canvas_interface.h"
#include "graphics_interface.h"
#include "image.h"
#include "file_manager_interface.h"

namespace ggadget {

class Texture::Impl {
 public:
  Impl(const GraphicsInterface *graphics,
       FileManagerInterface *file_manager,
       const char *name)
      : image_(NULL), color_(.0, .0, .0), opacity_(1.0) {
    ASSERT(name);
    size_t name_len = strlen(name);
    if (name[0] == '#' && (name_len == 7 || name_len == 9)) {
      int r = 0, g = 0, b = 0;
      int alpha = 255;
      int result = sscanf(name + 1, "%02x%02x%02x%02x", &r, &g, &b, &alpha);
      if (result < 3)
        LOG("Invalid color: %s", name);
      color_ = Color(r / 255.0, g / 255.0, b / 255.0);
      opacity_ = alpha / 255.0;
    } else {
      image_ = new Image(graphics, file_manager, name, false);
    }
  }

  Impl(const Impl &another)
      : image_(new Image(*(another.image_))),
        color_(another.color_),
        opacity_(another.opacity_) {
  }

  ~Impl() {
    delete image_;
    image_ = NULL;
  }

  void Draw(CanvasInterface *canvas) {
    size_t canvas_width = canvas->GetWidth();
    size_t canvas_height = canvas->GetHeight();

    if (image_) {
      // Don't apply opacity_ here because it is only applicable with color_. 
      size_t image_width = image_->GetWidth();
      size_t image_height = image_->GetHeight();
      if (image_width > 0 && image_height > 0) {
        for (size_t x = 0; x < canvas_width; x += image_width)
          for (size_t y = 0; y < canvas_height; y += image_height)
            image_->Draw(canvas, x, y);
      }
    } else if (opacity_ > 0) {
      canvas->MultiplyOpacity(opacity_);
      canvas->DrawFilledRect(0, 0, canvas_width, canvas_height, color_);
      canvas->MultiplyOpacity(1.0/opacity_);
    }
  }

  Image *image_;
  Color color_;
  double opacity_;
};

Texture::Texture(const GraphicsInterface *graphics,
                 FileManagerInterface *file_manager,
                 const char *name)
    : impl_(new Impl(graphics, file_manager, name)) {
}

Texture::Texture(const Texture &another)
    : impl_(new Impl(*another.impl_)) {
}

void Texture::Draw(CanvasInterface *canvas) {
  ASSERT(canvas);
  impl_->Draw(canvas);
}

} // namespace ggadget
