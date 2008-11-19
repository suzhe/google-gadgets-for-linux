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
#include "small_object.h"

#ifdef _DEBUG
// #define DEBUG_IMAGE_CACHE
#endif

namespace ggadget {

class ImageCache::Impl : public SmallObject<> {
  class SharedImage;
  typedef std::map<std::string, SharedImage *> ImageMap;

  class SharedImage : public ImageInterface {
   public:
    SharedImage(const std::string &key, const std::string &tag,
                ImageMap *owner, ImageInterface *image)
        : key_(key), tag_(tag), owner_(owner), image_(image), ref_(1) {
      ASSERT(owner_);
    }
    virtual ~SharedImage() {
#ifdef DEBUG_IMAGE_CACHE
      DLOG("Destroy image %s", key_.c_str());
#endif
      if (owner_)
        owner_->erase(key_);
      if (image_)
        image_->Destroy();
    }
    void Ref() {
      ASSERT(ref_ >= 0);
      ++ref_;
    }
    void Unref() {
      ASSERT(ref_ > 0);
      --ref_;
      if (ref_ == 0)
        delete this;
    }
    virtual void Destroy() {
      Unref();
    }
    virtual const CanvasInterface *GetCanvas() const {
      return image_ ? image_->GetCanvas() : NULL;
    }
    virtual void Draw(CanvasInterface *canvas, double x, double y) const {
      if (image_)
        image_->Draw(canvas, x, y);
    }
    virtual void StretchDraw(CanvasInterface *canvas,
                             double x, double y,
                             double width, double height) const {
      if (image_)
        image_->StretchDraw(canvas, x, y, width, height);
    }
    virtual double GetWidth() const {
      return image_ ? image_->GetWidth() : 0;
    }
    virtual double GetHeight() const {
      return image_ ? image_->GetHeight() : 0;
    }
    virtual ImageInterface *MultiplyColor(const Color &color) const {
      if (!image_)
        return NULL;
      // TODO: share images with color multiplied.

      // Reture self if the color is white.
      if (color == Color::kMiddleColor) {
        SharedImage *img = const_cast<SharedImage *>(this);
        img->Ref();
        return img;
      }

      return image_->MultiplyColor(color);
    }
    virtual bool GetPointValue(double x, double y,
                               Color *color, double *opacity) const {
      return image_ ? image_->GetPointValue(x, y, color, opacity) : false;
    }
    virtual std::string GetTag() const {
      return tag_;
    }
    virtual bool IsFullyOpaque() const {
      return image_ ? image_->IsFullyOpaque() : false;
    }
   public:
    std::string key_;
    std::string tag_;
    ImageMap *owner_;
    ImageInterface *image_;
    int ref_;
  };

 public:
  Impl() : ref_(0) {
#ifdef DEBUG_IMAGE_CACHE
    num_new_local_images_ = 0;
    num_shared_local_images_ = 0;
    num_new_global_images_ = 0;
    num_shared_global_images_ = 0;
#endif
  }

  ~Impl() {
#ifdef DEBUG_IMAGE_CACHE
    DLOG("Image statistics(new/shared): "
         "local: %d/%d, global: %d/%d, remained: %zd",
         num_new_local_images_, num_shared_local_images_,
         num_new_global_images_, num_shared_global_images_,
         images_.size() + mask_images_.size());
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
                            const std::string &filename, bool is_mask) {
    if (!gfx || filename.empty())
      return NULL;

    FileManagerInterface *global_fm = GetGlobalFileManager();
    ImageMap *image_map = is_mask ? &mask_images_ : &images_;
    ImageMap::const_iterator it;
    std::string local_key;
    std::string global_key;

    // Find image in cache first.
    if (fm) {
      local_key = fm->GetFullPath(filename.c_str());
      it = image_map->find(local_key);
      if (it != image_map->end()) {
#ifdef DEBUG_IMAGE_CACHE
        num_shared_local_images_++;
        DLOG("Local image %s found in cache.", local_key.c_str());
#endif
        it->second->Ref();
        return it->second;
      }
    }

    if (global_fm) {
      global_key = global_fm->GetFullPath(filename.c_str());
      it = image_map->find(global_key);
      if (it != image_map->end()) {
#ifdef DEBUG_IMAGE_CACHE
        num_shared_global_images_++;
        DLOG("Global image %s found in cache.", global_key.c_str());
#endif
        it->second->Ref();
        return it->second;
      }
    }

    // The image was not loaded yet.
    ImageInterface *img = NULL;
    std::string data;
    std::string key;
    if (fm && fm->ReadFile(filename.c_str(), &data)) {
      key = local_key;
      img = gfx->NewImage(filename, data, is_mask);
#ifdef DEBUG_IMAGE_CACHE
      DLOG("Local image %s loaded.", key.c_str());
      num_new_local_images_++;
#endif
    } else if (global_fm && global_fm->ReadFile(filename.c_str(), &data)) {
      key = global_key;
      img = gfx->NewImage(filename, data, is_mask);
#ifdef DEBUG_IMAGE_CACHE
      DLOG("Global image %s loaded.", key.c_str());
      num_new_global_images_++;
#endif
    } else {
      // Still return a SharedImage because the gadget wants the src of an
      // image even if the image can't be loaded.
      key = "Invalid:" + filename;
      DLOG("Failed to load image %s.", filename.c_str());
    }

    SharedImage *shared_img = new SharedImage(key, filename, image_map, img);
    (*image_map)[key] = shared_img;
    return shared_img;
  }

  void Ref() {
    ASSERT(ref_ >= 0);
    ASSERT(this == impl_);
    ++ref_;
  }

  void Unref() {
    ASSERT(ref_ > 0);
    ASSERT(this == impl_);
    --ref_;
    if (ref_ == 0) {
      impl_ = NULL;
      delete this;
    }
  }

  static Impl *Get() {
    if (!impl_)
      impl_ = new Impl();
    impl_->Ref();
    return impl_;
  }

 private:
  ImageMap images_;
  ImageMap mask_images_;
  int ref_;

#ifdef DEBUG_IMAGE_CACHE
  int num_new_local_images_;
  int num_shared_local_images_;
  int num_new_global_images_;
  int num_shared_global_images_;
#endif

 private:
  static Impl *impl_;
};

ImageCache::Impl *ImageCache::Impl::impl_ = NULL;

ImageCache::ImageCache() : impl_(Impl::Get()) {
}

ImageCache::~ImageCache() {
  impl_->Unref();
}

ImageInterface *ImageCache::LoadImage(GraphicsInterface *gfx,
                                      FileManagerInterface *fm,
                                      const std::string &filename,
                                      bool is_mask) {
  return impl_->LoadImage(gfx, fm, filename, is_mask);
}

} // namespace ggadget
