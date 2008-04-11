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
#include <gdk/gdkcairo.h>
#include <ggadget/logger.h>
#include <ggadget/signals.h>
#include "cairo_graphics.h"
#include "cairo_canvas.h"
#include "cairo_font.h"
#include "pixbuf_image.h"

#ifdef HAVE_RSVG_LIBRARY
#include "rsvg_image.h"
#endif

namespace ggadget {
namespace gtk {

class CairoGraphics::Impl {
 public:
  Impl(double zoom) : zoom_(zoom) {
    if (zoom_ <= 0) zoom_ = 1;
#ifdef _DEBUG
    num_new_images_ = num_shared_images_ = 0;
#endif
  }

  ~Impl() {
    ASSERT(image_map_.empty());
    ASSERT(mask_image_map_.empty());
#ifdef _DEBUG
    DLOG("CairoGraphics image statistics: new: %d/shared: %d",
         num_new_images_, num_shared_images_);
#endif
  }

  double zoom_;
  Signal1<void, double> on_zoom_signal_;
  typedef std::map<std::string, CairoImageBase *> ImageMap;
  ImageMap image_map_, mask_image_map_;

#ifdef _DEBUG
  int num_new_images_;
  int num_shared_images_;
#endif
};

CairoGraphics::CairoGraphics(double zoom) : impl_(new Impl(zoom)) {
}

CairoGraphics::~CairoGraphics() {
  delete impl_;
  impl_ = NULL;
}

GraphicsInterface *CairoGraphics::Clone() const {
  // Signals and ImageMaps are not cloned but zoom level is.
  CairoGraphics *gfx = new CairoGraphics(impl_->zoom_);
  return gfx;
}

double CairoGraphics::GetZoom() const {
  return impl_->zoom_;
}

void CairoGraphics::SetZoom(double zoom) {
  if (impl_->zoom_ != zoom) {
    impl_->zoom_ = (zoom > 0 ? zoom : 1);
    impl_->on_zoom_signal_(impl_->zoom_);
  }
}

Connection *CairoGraphics::ConnectOnZoom(Slot1<void, double> *slot) const {
  return impl_->on_zoom_signal_.Connect(slot);
}

CanvasInterface *CairoGraphics::NewCanvas(double w, double h) const {
  if (w <= 0 || h <= 0) return NULL;

  CairoCanvas *canvas = new CairoCanvas(this, w, h, CAIRO_FORMAT_ARGB32);
  if (!canvas->IsValid()) {
    delete canvas;
    canvas = NULL;
  }
  return canvas;
}

#ifdef HAVE_RSVG_LIBRARY
static bool IsSvg(const std::string &data) {
  //TODO: better detection method?
  return data.find("<?xml") != std::string::npos &&
         data.find("<svg") != std::string::npos;
}
#endif

ImageInterface *CairoGraphics::NewImage(const char *tag,
                                        const std::string &data,
                                        bool is_mask) const {
  if (data.empty())
    return NULL;

  std::string tag_str(tag ? tag : "");
  if (!tag_str.empty()) {
    Impl::ImageMap *image_map = is_mask ?
                                &impl_->mask_image_map_ : &impl_->image_map_;
    // Image with blank tag should not be cached, because it may not come
    // from a file.
    Impl::ImageMap::const_iterator it = image_map->find(tag_str);
    if (it != image_map->end()) {
#ifdef _DEBUG
      impl_->num_shared_images_++;
#endif
      it->second->Ref();
      return it->second;
    }
  }

  CairoImageBase *img = NULL;
#ifdef HAVE_RSVG_LIBRARY
  // Only use RsvgImage for ordinary svg image.
  if (IsSvg(data) && !is_mask) {
    img = new RsvgImage(this, tag, data, is_mask);
    if (!img->IsValid()) {
      img->Destroy();
      img = NULL;
    }
  } else {
#else
  if (1) {
#endif
    img = new PixbufImage(this, tag, data, is_mask);
    if (!img->IsValid()) {
      img->Destroy();
      img = NULL;
    }
  }
#ifdef _DEBUG
  impl_->num_new_images_++;
#endif

  if (img && !tag_str.empty()) {
    Impl::ImageMap *image_map = is_mask ?
                                &impl_->mask_image_map_ : &impl_->image_map_;
    (*image_map)[tag_str] = img;
  }
  return img;
}

void CairoGraphics::OnImageDelete(const std::string &tag, bool is_mask) const {
  Impl::ImageMap *image_map = is_mask ?
                              &impl_->mask_image_map_ : &impl_->image_map_;
  image_map->erase(tag);
}

FontInterface *CairoGraphics::NewFont(const char *family, double pt_size,
                                      FontInterface::Style style,
                                      FontInterface::Weight weight) const {
  PangoFontDescription *font = pango_font_description_new();

  pango_font_description_set_family(font, family);
  // Calculate pixel size based on the Windows DPI of 96 for compatibility
  // reasons.
  double px_size = pt_size * PANGO_SCALE * 96. / 72.;
  pango_font_description_set_absolute_size(font, px_size);

  if (weight == FontInterface::WEIGHT_BOLD) {
    pango_font_description_set_weight(font, PANGO_WEIGHT_BOLD);
  }

  if (style == FontInterface::STYLE_ITALIC) {
    pango_font_description_set_style(font, PANGO_STYLE_ITALIC);
  }

  return new CairoFont(font, pt_size, style, weight);
}

} // namespace gtk
} // namespace ggadget
