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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <map>

#include <ggadget/color.h>
#include <ggadget/common.h>
#include <ggadget/logger.h>
#include "qt_graphics.h"
#include "qt_canvas.h"
#include "qt_image.h"
#include "qt_font.h"

namespace ggadget {
namespace qt {

typedef std::map<std::string, QtImage*> ImageMap;

class QtGraphics::Impl {
 public:
  Impl(double zoom) : zoom_(zoom) {
    if (zoom_ <= 0) zoom_ = 1;
  }

  double zoom_;
  Signal1<void, double> on_zoom_signal_;
  ImageMap image_map_, mask_image_map_;
};

QtGraphics::QtGraphics(double zoom) : impl_(new Impl(zoom)) {
}

QtGraphics::~QtGraphics() {
  delete impl_;
  impl_ = NULL;
}

double QtGraphics::GetZoom() const {
  return impl_->zoom_;
}

void QtGraphics::SetZoom(double zoom) {
  if (impl_->zoom_ != zoom) {
    impl_->zoom_ = (zoom > 0 ? zoom : 1);
    impl_->on_zoom_signal_(impl_->zoom_);
  }
}

Connection *QtGraphics::ConnectOnZoom(Slot1<void, double> *slot) const {
  return impl_->on_zoom_signal_.Connect(slot);
}

CanvasInterface *QtGraphics::NewCanvas(double w, double h) const {
  if (!w || !h) return NULL;

  QtCanvas *canvas = new QtCanvas(this, w, h);
  if (!canvas) return NULL;
  if (!canvas->IsValid()) {
    delete canvas;
    canvas = NULL;
  }
  return canvas;
}

ImageInterface *QtGraphics::NewImage(const char *tag,
                                     const std::string &data,
                                     bool is_mask) const {
  if (data.empty()) return NULL;

  std::string tag_str(tag ? tag : "");
 /* ImageMap *map = is_mask ?
      &impl_->mask_image_map_ : &impl_->image_map_;
  if (!tag_str.empty()) {
    // Image with blank tag should not be cached, because it may not come
    // from a file.
    ImageMap::const_iterator it = map->find(tag_str);
    if (it != map->end()) {
      it->second->Ref();
      return it->second;
    }
  }*/

  QtImage *img = new QtImage(NULL, tag, data, is_mask);
  if (!img) return NULL;
  if (!img->IsValid()) {
    img->Destroy();
    img = NULL;
  }

/*  if (img && !tag_str.empty()) {
    (*map)[tag_str] = img;
  }*/
  return img;
}

void QtGraphics::RemoveImageTag(const char *tag, bool is_mask) {
  std::string tag_str(tag ? tag : "");
  ImageMap *map = is_mask ?
      &impl_->mask_image_map_ : &impl_->image_map_;
  if (!tag_str.empty()) {
    map->erase(tag_str);
  }
}

FontInterface *QtGraphics::NewFont(const char *family, double pt_size,
                                   FontInterface::Style style,
                                   FontInterface::Weight weight) const {
  // TODO:How to get the most proper size
  double size = pt_size * 96./122.;
  return new QtFont(family, size, style, weight);
}

} // namespace qt
} // namespace ggadget
