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

#include "image.h"
#include "canvas_interface.h"
#include "color.h"
#include "graphics_interface.h"
#include "file_manager_interface.h"

namespace ggadget {

static const Color kWhite(1., 1., 1.);

class Image::Impl {
 public:
  Impl(const GraphicsInterface *graphics,
       FileManagerInterface *file_manager,
       const char *filename, bool is_mask)
      : graphics_(graphics),
        filename_(filename),
        is_mask_(is_mask),
        canvas_(NULL),
        colormultiply_(kWhite),
        colormultiply_specified_(false),
        failed_(false) {
    ASSERT(graphics);
    ASSERT(file_manager);
    ASSERT(filename);
    // canvas_ will be lazy initialized when it is used.

    if (!filename_.empty()) {
      std::string real_path;
      failed_ = !file_manager->GetFileContents(filename_.c_str(),
                                               &data_, &real_path);
    }
  }

  Impl(const GraphicsInterface *graphics,
       const char *data, size_t data_size, bool is_mask)
      : graphics_(graphics),
        data_(data, data_size),
        is_mask_(is_mask),
        canvas_(NULL),
        colormultiply_(kWhite),
        colormultiply_specified_(false),
        failed_(false) {
    ASSERT(graphics);
    ASSERT(data);
  }

  Impl(const Impl &another)
      : graphics_(another.graphics_),
        data_(another.data_),
        filename_(another.filename_),
        is_mask_(another.is_mask_),
        canvas_(NULL),
        colormultiply_(another.colormultiply_),
        colormultiply_specified_(another.colormultiply_specified_),
        failed_(another.failed_) {
    if (!failed_ && another.canvas_) {
      canvas_ = graphics_->NewCanvas(another.canvas_->GetWidth(),
                                     another.canvas_->GetHeight());
      canvas_->DrawCanvas(0, 0, another.canvas_);
    }
  }

  void DestroyCanvas() {
    if (canvas_) {
      canvas_->Destroy();
      canvas_ = NULL;
    }
  }

  ~Impl() {
    DestroyCanvas();
  }

  const CanvasInterface *GetCanvas() {
    if (!failed_ && !canvas_) {
      canvas_ = is_mask_ ?
                graphics_->NewMask(data_.c_str(), data_.size()) :
                graphics_->NewImage(data_.c_str(), data_.size(), 
                                    colormultiply_specified_ ? 
                                      &colormultiply_ : NULL);
    }
    return canvas_;
  }

  const GraphicsInterface *graphics_;
  std::string data_;
  std::string filename_;
  bool is_mask_;
  CanvasInterface *canvas_;
  Color colormultiply_; 
  bool colormultiply_specified_;
  bool failed_;
};

Image::Image(const GraphicsInterface *graphics,
             FileManagerInterface *file_manager,
             const char *filename, bool is_mask)
    : impl_(new Impl(graphics, file_manager, filename, is_mask)) {
}

Image::Image(const GraphicsInterface *graphics,
             const char *data, size_t data_size, bool is_mask)
    : impl_(new Impl(graphics, data, data_size, is_mask)) {
}

Image::Image(const Image &another)
    : impl_(new Impl(*another.impl_)) {
}

Image::~Image() {
  delete impl_;
  impl_ = NULL;
}

void Image::SetColorMultiply(const Color &color) {
  impl_->DestroyCanvas();

  // Color multiply is expensive, so keep the value as 
  // unspecified most of the time.
  impl_->colormultiply_specified_ = !(color == kWhite);
  if (impl_->colormultiply_specified_) {
    impl_->colormultiply_ = color;
  }  
}

const CanvasInterface *Image::GetCanvas() {
  return impl_->GetCanvas();
}

void Image::Draw(CanvasInterface *canvas, double x, double y) {
  const CanvasInterface *image_canvas = impl_->GetCanvas();
  ASSERT(canvas);
  if (image_canvas)
    canvas->DrawCanvas(x, y, image_canvas);
}

void Image::StretchDraw(CanvasInterface *canvas,
                        double x, double y,
                        double width, double height) {
  ASSERT(canvas);
  const CanvasInterface *image_canvas = impl_->GetCanvas();
  if (image_canvas) {
    double cx = width / image_canvas->GetWidth();
    double cy = height / image_canvas->GetHeight();
    if (cx != 1.0 || cy != 1.0) {
      canvas->PushState();
      canvas->ScaleCoordinates(cx, cy);
      canvas->DrawCanvas(x / cx, y / cy, image_canvas);
      canvas->PopState();
    } else {
      canvas->DrawCanvas(x, y, image_canvas);
    }
  }
}

size_t Image::GetWidth() {
  const CanvasInterface *canvas = impl_->GetCanvas();
  return canvas ? canvas->GetWidth() : 0;
}

size_t Image::GetHeight() {
  const CanvasInterface *canvas = impl_->GetCanvas();
  return canvas ? canvas->GetHeight() : 0;
}

std::string Image::GetSrc() {
  return impl_->filename_;
}

} // namespace ggadget
