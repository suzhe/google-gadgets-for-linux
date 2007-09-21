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
#include "graphics_interface.h"
#include "file_manager_interface.h"

namespace ggadget {

class Image::Impl {
 public:
  Impl(const GraphicsInterface *graphics,
       FileManagerInterface *file_manager,
       const char *filename)
      : graphics_(graphics),
        file_manager_(file_manager),
        filename_(filename_),
        canvas_(NULL),
        failed_(false) {
    ASSERT(graphics);
    ASSERT(file_manager);
    ASSERT(filename);
  }

  Impl(const GraphicsInterface *graphics, const char *data, size_t data_size)
      : graphics_(graphics),
        file_manager_(NULL),
        canvas_(NULL),
        failed_(false) {
    ASSERT(graphics);
    ASSERT(data);
    // TODO: Remove IMG_JPEG.
    canvas_ = graphics->NewImage(data, data_size);
  }

  Impl(const Impl &another)
      : graphics_(another.graphics_),
        filename_(another.filename_),
        canvas_(NULL),
        failed_(another.failed_) {
    if (!failed_ && another.canvas_) {
      canvas_ = graphics_->NewCanvas(another.canvas_->GetWidth(),
                                     another.canvas_->GetHeight());
      canvas_->DrawCanvas(0, 0, another.canvas_);
    }
  }

  Impl::~Impl() {
    if (canvas_) {
      canvas_->Destroy();
      canvas_ = NULL;
    }
  }

  const CanvasInterface *GetCanvas() {
    if (!failed_ && !canvas_ && !filename_.empty() && file_manager_) {
      std::string data;
      std::string real_path;
      failed_ = file_manager_->GetFileContents(filename_.c_str(),
                                               &data, &real_path);
      if (!failed_)
        canvas_ = graphics_->NewImage(data.c_str(), data.size());
    }
    return canvas_;
  }

  const GraphicsInterface *graphics_;
  FileManagerInterface *file_manager_;
  std::string filename_;
  CanvasInterface *canvas_;
  bool failed_;
};

Image::Image(const GraphicsInterface *graphics,
             FileManagerInterface *file_manager,
             const char *filename)
    : impl_(new Impl(graphics, file_manager, filename)) {
}

Image::Image(const GraphicsInterface *graphics,
             const char *data, size_t data_size)
    : impl_(new Impl(graphics, data, data_size)) {
}

Image::Image(const Image &another)
    : impl_(new Impl(*another.impl_)) {
}

const CanvasInterface *Image::GetCanvas() {
  return impl_->GetCanvas();
}

} // namespace ggadget
