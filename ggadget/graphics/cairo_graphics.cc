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

#include "cairo_graphics.h"
#include "cairo_canvas.h"
#include "cairo_font.h"

namespace ggadget {

CairoGraphics::CairoGraphics(double zoom) : zoom_(zoom) { 
  if (zoom_ <= .0) {
    zoom_ = 1.;
  }
  g_type_init();  
}

CanvasInterface *CairoGraphics::NewCanvas(size_t w, size_t h) const {
  CairoCanvas *c = NULL;
  cairo_t *cr = NULL;
  cairo_surface_t *surface = NULL;

  size_t width;
  size_t height;  
  if (zoom_ == 1.) {
    width = w;
    height = h;    
  }
  else {
    // compute new width and height based on zoom
    width = size_t(w * zoom_);
    height = size_t(h * zoom_);

    if (width == 0) width = 1;
    if (height == 0) height = 1;    
  }

  // create surface at native resolution after adjustment by scale
  surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  if (CAIRO_STATUS_SUCCESS != cairo_surface_status(surface)) {
    cairo_surface_destroy(surface);
    surface = NULL;
    goto exit;
  }

  cr = cairo_create(surface);
  if (zoom_ != 1.) {
    // divide again since x, y scale values may differ after rounding from the 
    // initial multiplication 
    cairo_scale(cr, width / (double)w, height / (double)h);
  }

  // clear canvas  
  c = new CairoCanvas(cr, w, h, false);
  c->ClearSurface();

exit:
  if (cr) {
    cairo_destroy(cr);
    cr = NULL;
  }

  if (surface) {
    cairo_surface_destroy(surface);
    surface = NULL; 
  }

  return c;
}

CanvasInterface *CairoGraphics::NewMask(const char *img_bytes, 
                                        size_t img_bytes_count) const {
  CairoCanvas *img = NULL;
  size_t h, w;
  cairo_surface_t *surface = NULL;
  cairo_t *cr = NULL;
  GdkPixbuf *pixbuf = NULL;

  if (!img_bytes || 0 == img_bytes_count) {
    goto exit;
  }

  pixbuf = LoadPixbufFromData(img_bytes, img_bytes_count);
  if (!pixbuf) {
    LOG("Error: unable to load PixBuf from data.");
    goto exit;
  }
  w = gdk_pixbuf_get_width(pixbuf);     
  h = gdk_pixbuf_get_height(pixbuf); 

  if (!gdk_pixbuf_get_has_alpha(pixbuf)) {
    // clone pixbuf with alpha channel and free the old one
    GdkPixbuf *a_pixbuf = gdk_pixbuf_add_alpha(pixbuf, false, 0, 0, 0);
    g_object_unref(pixbuf);
    pixbuf = a_pixbuf;
    a_pixbuf = NULL;    
  }

  // convert the pixels to mask specification as required by Cairo
  int rowstride, channels;
  guchar *pixels, *p;  
  rowstride = gdk_pixbuf_get_rowstride(pixbuf);
  channels = gdk_pixbuf_get_n_channels(pixbuf);
  pixels = gdk_pixbuf_get_pixels(pixbuf);
  if (GDK_COLORSPACE_RGB != gdk_pixbuf_get_colorspace(pixbuf) ||
      8 != gdk_pixbuf_get_bits_per_sample(pixbuf) ||
      4 != channels) {
    LOG("Error: unsupported PixBuf format.");
    goto exit;
  }  
  for (size_t x = 0; x < w; x++) {
    for (size_t y = 0; y < h; y++) {
      p = pixels + y * rowstride + x * channels;
      if (0 == p[0] && 0 == p[1] && 0 == p[2] && 255 == p[3]) {
        p[0] = p[1] = p[2] = p[3] = 0;
      }
      else {
        p[0] = p[1] = p[2] = 0;
        p[3] = 255;
      }
    }
  }

  // Now create the surface (eight-bit alpha channel) and Cairo context.
  // For some reason, A1 surface doesn't work (cairo bug?)
  surface = cairo_image_surface_create(CAIRO_FORMAT_A8, w, h);
  if (CAIRO_STATUS_SUCCESS != cairo_surface_status(surface)) {
    cairo_surface_destroy(surface);
    surface = NULL;
    goto exit;
  }

  cr = cairo_create(surface);
  img = new CairoCanvas(cr, w, h, true);
  img->ClearSurface();

  gdk_cairo_set_source_pixbuf(cr, pixbuf, 0., 0.);
  cairo_paint(cr);   

  cairo_set_source_rgba(cr, 0., 0., 0., 0.);

exit:
  if (cr) {
    cairo_destroy(cr);
    cr = NULL;
  }  

  if (surface) {
    cairo_surface_destroy(surface);
    surface = NULL; 
  }

  if (pixbuf) {
    g_object_unref(pixbuf);
    pixbuf = NULL;
  }

  return img;
}

CanvasInterface *CairoGraphics::NewImage(const char *img_bytes, 
                                         size_t img_bytes_count) const {
  CairoCanvas *img = NULL;
  size_t h, w;
  cairo_surface_t *surface = NULL;
  cairo_t *cr = NULL;
  GdkPixbuf *pixbuf = NULL;

  if (!img_bytes || 0 == img_bytes_count) {
    goto exit;
  }

  // for all other image formats, try gdk image loader
  pixbuf = LoadPixbufFromData(img_bytes, img_bytes_count);
  if (!pixbuf) {
    LOG("Error: unable to load PixBuf from data.");
    goto exit;
  }

  w = gdk_pixbuf_get_width(pixbuf);     
  h = gdk_pixbuf_get_height(pixbuf); 
  surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
  if (CAIRO_STATUS_SUCCESS != cairo_surface_status(surface)) {
    cairo_surface_destroy(surface);
    surface = NULL;
    goto exit;
  }

  cr = cairo_create(surface);
  img = new CairoCanvas(cr, w, h, false);  
  img->ClearSurface();

  gdk_cairo_set_source_pixbuf(cr, pixbuf, 0., 0.);
  cairo_paint(cr);         

  cairo_set_source_rgba(cr, 0., 0., 0., 0.);

exit:  
  if (cr) {
    cairo_destroy(cr);
    cr = NULL;
  }

  if (surface) {
    cairo_surface_destroy(surface);
    surface = NULL; 
  }

  if (pixbuf) {
    g_object_unref(pixbuf);
    pixbuf = NULL;
  }

  return img;
}

GdkPixbuf *CairoGraphics::LoadPixbufFromData(const char *img_bytes, 
                                             size_t img_bytes_count) {
  GdkPixbuf *pixbuf = NULL;
  GError *error = NULL;
  GdkPixbufLoader *loader = NULL;

  loader = gdk_pixbuf_loader_new();
  gboolean status = gdk_pixbuf_loader_write(loader, (const guchar*)img_bytes, 
                                            img_bytes_count, &error);
  if (!status) {
    goto exit;        
  }
  status = gdk_pixbuf_loader_close(loader, &error);
  if (!status) {
    goto exit;        
  }

  pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
  if (pixbuf) {
    gdk_pixbuf_ref(pixbuf);    
  }

exit:
  if (error) {
    g_error_free(error);
    error = NULL;
  }

  if (loader) {
    g_object_unref(loader);
    loader = NULL;
  }

  return pixbuf;
}

FontInterface *CairoGraphics::NewFont(const char *family, size_t pt_size, 
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

} // namespace ggadget
