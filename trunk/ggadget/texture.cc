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

#include <cmath>
#include "texture.h"
#include "canvas_interface.h"
#include "graphics_interface.h"
#include "image.h"
#include "logger.h"
#include "file_manager_interface.h"
#include "string_utils.h"

namespace ggadget {

class Texture::Impl {
 public:
  Impl(const GraphicsInterface *graphics,
       FileManagerInterface *file_manager,
       const char *name)
      : image_(NULL), color_(.0, .0, .0), opacity_(1.0),
        name_(name ? name : "") {
    if (!ParseColorName(name_, &color_, &opacity_)) {
      image_ = new Image(graphics, file_manager, name, false);
    }
  }

  Impl(const Color &color, double opacity)
      : image_(NULL), color_(color), opacity_(opacity),
        name_(StringPrintf("#%02x%02x%02x%02x",
                           static_cast<int>(round(opacity * 255)),
                           static_cast<int>(round(color.red * 255)),
                           static_cast<int>(round(color.green * 255)),
                           static_cast<int>(round(color.blue * 255)))) {
  }

  Impl(const GraphicsInterface *graphics,
       const char *data,
       size_t data_size)
      : image_(new Image(graphics, data, data_size, false)),
        color_(.0, .0, .0), opacity_(1.0) {
  }

  Impl(const Impl &another)
      : image_(another.image_ ? new Image(*(another.image_)) : NULL),
        color_(another.color_),
        opacity_(another.opacity_),
        name_(another.name_) {
  }

  ~Impl() {
    delete image_;
    image_ = NULL;
  }

  void Draw(CanvasInterface *canvas) const {
    size_t canvas_width = canvas->GetWidth();
    size_t canvas_height = canvas->GetHeight();

    if (image_) {
      // Don't apply opacity_ here because it is only applicable with color_.
      canvas->DrawFilledRectWithCanvas(0, 0, canvas_width, canvas_height,
                                       image_->GetCanvas());
    } else if (opacity_ > 0) {
      if (opacity_ != 1.0) {
        canvas->PushState();
        canvas->MultiplyOpacity(opacity_);
      }
      canvas->DrawFilledRect(0, 0, canvas_width, canvas_height, color_);
      if (opacity_ != 1.0)
        canvas->PopState();
    }
  }

  Image *image_;
  Color color_;
  double opacity_;
  std::string name_;
};

Texture::Texture(const GraphicsInterface *graphics,
                 FileManagerInterface *file_manager,
                 const char *name)
    : impl_(new Impl(graphics, file_manager, name)) {
}

Texture::Texture(const GraphicsInterface *graphics,
                 const char *data,
                 size_t data_size)
    : impl_(new Impl(graphics, data, data_size)) {
}

Texture::Texture(const Color &color, double opacity)
    : impl_(new Impl(color, opacity)) {
}

Texture::Texture(const Texture &another)
    : impl_(new Impl(*another.impl_)) {
}

Texture::~Texture() {
  delete impl_;
  impl_ = NULL;
}

void Texture::Draw(CanvasInterface *canvas) const {
  ASSERT(canvas);
  impl_->Draw(canvas);
}

void Texture::DrawText(CanvasInterface *canvas, double x, double y,
                       double width, double height, const char *text,
                       const FontInterface *f, CanvasInterface::Alignment align,
                       CanvasInterface::VAlignment valign,
                       CanvasInterface::Trimming trimming,
                       int text_flags) const {
  ASSERT(canvas);
  if (impl_->image_) {
    // Don't apply opacity_ here because it is only applicable with color_.
    canvas->DrawTextWithTexture(x, y, width, height, text, f,
                                impl_->image_->GetCanvas(),
                                align, valign, trimming, text_flags);
  } else if (impl_->opacity_ > 0) {
    if (impl_->opacity_ != 1.0) {
      canvas->PushState();
      canvas->MultiplyOpacity(impl_->opacity_);
    }
    canvas->DrawText(x, y, width, height, text, f, impl_->color_,
                     align, valign, trimming, text_flags);
    if (impl_->opacity_ != 1.0)
      canvas->PopState();
  }
}

Color Texture::GetColor() {
  return impl_->color_;
}

std::string Texture::GetSrc() const {
  return impl_->name_;
}

} // namespace ggadget
