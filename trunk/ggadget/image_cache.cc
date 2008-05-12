/*
  Copyright 2008 Google Inc.

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

#include <string>
#include <map>
#include <algorithm>
#include "common.h"
#include "logger.h"
#include "image_cache.h"
#include "graphics_interface.h"
#include "file_manager_factory.h"

namespace ggadget {

class ImageCache::Impl {
  class SharedImage;
  typedef std::map<std::string, SharedImage *> ImageMap;

  class SharedImage : public ImageInterface {
   public:
    SharedImage(ImageMap *owner, ImageInterface *image)
      : owner_(owner), image_(image), ref_(1) {
      ASSERT(owner_);
      ASSERT(image_);
    }

    virtual ~SharedImage() {
      image_->Destroy();
    }

    void Ref() {
      ASSERT(ref_ >= 0);
      ++ref_;
    }

    void Unref() {
      ASSERT(ref_ > 0);
      --ref_;
      if (ref_ == 0) {
        if (owner_)
          owner_->erase(image_->GetTag());
        delete this;
      }
    }

    virtual void Destroy() {
      Unref();
    }

    virtual const CanvasInterface *GetCanvas() const {
      return image_->GetCanvas();
    }
    virtual void Draw(CanvasInterface *canvas, double x, double y) const {
      image_->Draw(canvas, x, y);
    }
    virtual void StretchDraw(CanvasInterface *canvas,
                             double x, double y,
                             double width, double height) const {
      image_->StretchDraw(canvas, x, y, width, height);
    }
    virtual double GetWidth() const {
      return image_->GetWidth();
    }
    virtual double GetHeight() const {
      return image_->GetHeight();
    }
    virtual ImageInterface *MultiplyColor(const Color &color) const {
      // TODO: share images with color multiplied.

      // Reture self if the color is white.
      if (color == Color::kWhite) {
        SharedImage *img = const_cast<SharedImage *>(this);
        img->Ref();
        return img;
      }

      return image_->MultiplyColor(color);
    }
    virtual bool GetPointValue(double x, double y,
                               Color *color, double *opacity) const {
      return image_->GetPointValue(x, y, color, opacity);
    }
    virtual std::string GetTag() const {
      return image_->GetTag();
    }
    virtual bool IsFullyOpaque() const {
      return image_->IsFullyOpaque();
    }
   public:
    ImageMap *owner_;
    ImageInterface *image_;
    int ref_;
  };

 public:
  Impl() {
#ifdef _DEBUG
    num_new_images_ = 0;
    num_shared_images_ = 0;
#endif
  }

  ~Impl() {
#ifdef _DEBUG
    DLOG("Image statistics(new/shared): local %d/%d;"
         " global %d/%d remain local %zd global %zd",
         num_new_images_, num_shared_images_,
         global_num_new_images_, global_num_shared_images_,
         images_.size() + mask_images_.size(),
         global_images_.size() + global_mask_images_.size());
#endif
    for (ImageMap::const_iterator it = images_.begin();
         it != images_.end(); ++it) {
      DLOG("!!! Image leak: %s", it->first.c_str());
      // Detach the leaked image.
      it->second->owner_ = NULL;
    }

    for (ImageMap::const_iterator it = mask_images_.begin();
         it != mask_images_.end(); ++it) {
      DLOG("!!! Mask image leak: %s", it->first.c_str());
      it->second->owner_ = NULL;
    }
  }

  ImageInterface *LoadImage(GraphicsInterface *gfx, FileManagerInterface *fm,
                            const char *filename, bool is_mask) {
    if (!gfx || !filename || !*filename)
      return NULL;

    std::string tag(filename);

    // Search in local cache first, then in global cache.
    ImageMap *image_map = is_mask ? &mask_images_ : &images_;
    ImageMap::iterator it = image_map->find(tag);
    if (it != image_map->end()) {
#ifdef _DEBUG
      num_shared_images_++;
#endif
      it->second->Ref();
      return it->second;
    }

    image_map = is_mask ? &global_mask_images_ : &global_images_;
    it = image_map->find(tag);
    if (it != image_map->end()) {
#ifdef _DEBUG
      global_num_shared_images_++;
#endif
      it->second->Ref();
      return it->second;
    }

    // The image was not loaded yet.
    ImageInterface *img = NULL;
    std::string data;
    FileManagerInterface *global_fm = GetGlobalFileManager();

    bool is_global = false;
    if (fm && fm->ReadFile(filename, &data)) {
      is_global = false;
    } else if (global_fm && global_fm->ReadFile(filename, &data)) {
      is_global = true;
    } else {
      // Failed to load the image.
      return NULL;
    }

    img = gfx->NewImage(filename, data, is_mask);
    if (img) {
      if (is_global) {
        image_map = is_mask ? &global_mask_images_ : &global_images_;
#ifdef _DEBUG
        global_num_new_images_++;
#endif
      } else {
        image_map = is_mask ? &mask_images_ : &images_;
#ifdef _DEBUG
        num_new_images_++;
#endif
      }

      SharedImage *shared_img = new SharedImage(image_map, img);
      (*image_map)[tag] = shared_img;
      return shared_img;
    }
    return NULL;
  }

 private:
  ImageMap images_;
  ImageMap mask_images_;

  static ImageMap global_images_;
  static ImageMap global_mask_images_;

#ifdef _DEBUG
  int num_new_images_;
  int num_shared_images_;
  static int global_num_new_images_;
  static int global_num_shared_images_;
#endif
};

ImageCache::Impl::ImageMap ImageCache::Impl::global_images_;
ImageCache::Impl::ImageMap ImageCache::Impl::global_mask_images_;
#ifdef _DEBUG
int ImageCache::Impl::global_num_new_images_ = 0;
int ImageCache::Impl::global_num_shared_images_ = 0;
#endif

ImageCache::ImageCache()
  : impl_(new Impl()) {
}

ImageCache::~ImageCache() {
  delete impl_;
  impl_ = NULL;
}

ImageInterface *ImageCache::LoadImage(GraphicsInterface *gfx, FileManagerInterface *fm,
                                      const char *filename, bool is_mask) {
  return impl_->LoadImage(gfx, fm, filename, is_mask);
}

} // namespace ggadget
