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

#include "cairo_canvas.h"
#include "cairo_font.h"

namespace ggadget {

const char *CairoCanvas::kClassType = "CairoCanvas";

CairoCanvas::CairoCanvas(cairo_t *cr, size_t w, size_t h, bool is_mask) 
  : cr_(cr), width_(w), height_(h), is_mask_(is_mask), opacity_(1.) {
  cairo_reference(cr_);
  // many CairoCanvas methods assume no existing path, so clear any 
  // existing paths on construction
  cairo_new_path(cr_); 
}
 
CairoCanvas::~CairoCanvas() {
  cairo_destroy(cr_); 
}

bool CairoCanvas::PopState() {
  if (opacity_stack_.empty()) {
    return false;
  }
  
  opacity_ = opacity_stack_.top();
  opacity_stack_.pop();
  cairo_restore(cr_);
  return true;
}

bool CairoCanvas::PushState() {
  opacity_stack_.push(opacity_);
  cairo_save(cr_);
  return true;
}

bool CairoCanvas::MultiplyOpacity(double opacity) {
  if (opacity >= 0.0 && opacity <= 1.0) {
    opacity_ *= opacity;    
    return true;
  }
  return false; 
}

bool CairoCanvas::DrawLine(double x0, double y0, double x1, double y1, 
                             double width, const Color &c) {
  if (width < 0.0) {
    return false;
  }
  
  cairo_set_line_width(cr_, width);
  cairo_set_source_rgba(cr_, c.red, c.green, c.blue, opacity_);
  cairo_move_to(cr_, x0, y0);
  cairo_line_to(cr_, x1, y1);
  cairo_stroke(cr_);
  
  return true;
}

void CairoCanvas::RotateCoordinates(double radians) {
  cairo_rotate(cr_, radians);
}

void CairoCanvas::TranslateCoordinates(double dx, double dy) {
  cairo_translate(cr_, dx, dy);  
}

void CairoCanvas::ScaleCoordinates(double cx, double cy) {
  cairo_scale(cr_, cx, cy);  
}

bool CairoCanvas::DrawFilledRect(double x, double y, 
                                  double w, double h, const Color &c) {
  if (w < 0.0 || h < 0.0) {
    return false;    
  }

  cairo_set_source_rgba(cr_, c.red, c.green, c.blue, opacity_);
  cairo_rectangle(cr_, x, y, w, h);
  cairo_fill(cr_);  
  return true;
}

bool CairoCanvas::IntersectRectClipRegion(double x, double y, 
                                            double w, double h) {
  if (w < 0.0 || h < 0.0) {
    return false;    
  }
  
  cairo_rectangle(cr_, x, y, w, h);
  cairo_clip(cr_);
  return true;
}

bool CairoCanvas::DrawCanvas(double x, double y, const CanvasInterface *img) {
  // verify class type before downcasting
  if (!img || img->IsMask() ||
      strcmp(CairoCanvas::kClassType, img->ClassType())) {
    return false;
  }
  
  CairoCanvas *cimg = (CairoCanvas *)img;
  cairo_surface_t *s = cimg->GetSurface();
  int sheight = cairo_image_surface_get_height(s);
  int swidth = cairo_image_surface_get_width(s);
  size_t w = cimg->GetWidth();
  size_t h = cimg->GetHeight();
  if (size_t(sheight) == h && size_t(swidth) == w) {
    // no scaling needed
    cairo_set_source_surface(cr_, s, x, y);
    cairo_paint_with_alpha(cr_, opacity_);
  }
  else {
    // CairoGraphics supports only uniform scaling in both X, Y, but due to 
    // rounding differences, we need to compute the exact scaling individually
    // for X and Y.
    double cx = double(w) / swidth;
    double cy = double(h) / sheight;
    
    cairo_save(cr_);
    cairo_scale(cr_, cx, cy);
    cairo_set_source_surface(cr_, s, x / cx, y / cy);
    cairo_paint_with_alpha(cr_, opacity_);
    cairo_restore(cr_);
  }
    
  return true;
}

bool CairoCanvas::DrawCanvasWithMask(double x, double y, 
                                    const CanvasInterface *img,
                                    double mx, double my,
                                    const CanvasInterface *mask) {
  // verify class type before downcasting
  if (!img || img->IsMask() || !mask || !mask->IsMask() ||
      strcmp(CairoCanvas::kClassType, img->ClassType()) ||      
      strcmp(CairoCanvas::kClassType, mask->ClassType())) {
    return false;
  }

  CairoCanvas *cmask = (CairoCanvas *)mask;
  CairoCanvas *cimg = (CairoCanvas *)img;
  // In this implementation, only non-mask canvases may have surface dimensions
  // different from the canvas dimensions, so we only need to check img.
  // However, this also means that the zoom for the canvas needs to be scaled
  // independently from the mask in the zoomed scenario, which produces more
  // work for us to do in order to resize the two surfaces to the same
  // resolution.
  cairo_surface_t *simg = cimg->GetSurface();
  cairo_surface_t *smask = cmask->GetSurface();
  int sheight = cairo_image_surface_get_height(simg);
  int swidth = cairo_image_surface_get_width(simg);
  size_t w = cimg->GetWidth();
  size_t h = cimg->GetHeight();
  if (size_t(sheight) == h && size_t(swidth) == w) {
    // no scaling needed
    cairo_set_source_surface(cr_, simg, x, y);
    cairo_mask_surface(cr_, smask, mx, my);
  }
  else {  
    cairo_save(cr_);
    
    cairo_t *cr;
    cairo_surface_t *target;    
    double cx = double(w) / swidth;
    double cy = double(h) / sheight;
    // enlarge the lower resolution surface
    if (cx < 1.) { // only check cx since cx should be approx same as cy
      // img is higher res (zoom > 1), resize mask
      
      // this scaling is a bit off, but this type of errors are 
      // unavoidable when compositing images of different sizes
      size_t maskw = size_t(cmask->GetWidth() / cx);
      size_t maskh = size_t(cmask->GetHeight() / cy);  
      target = cairo_image_surface_create(CAIRO_FORMAT_A8, maskw, maskh);
      cr = cairo_create(target);
      cairo_scale(cr, 1. / cx, 1. / cy);
      cairo_set_source_surface(cr, smask, 0., 0.);
      cairo_paint(cr);
      smask = target;
      cairo_destroy(cr); // destroy before target is used
      cr = NULL;    
            
      cairo_scale(cr_, cx, cy);
      cairo_set_source_surface(cr_, simg, x / cx, y / cy);
      cairo_mask_surface(cr_, smask, mx / cx, my / cy);
    }
    else {
      // img is lower res (zoom < 1), resize img
      
      target = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
      cr = cairo_create(target);
      cairo_scale(cr, cx, cy);
      cairo_set_source_surface(cr, simg, 0., 0.);
      cairo_paint(cr);
      simg = target;
      cairo_destroy(cr); // destroy before target is used
      cr = NULL;    
            
      cairo_scale(cr_, 1. / cx, 1. / cy);
      cairo_set_source_surface(cr_, simg, x * cx, y * cy);
      cairo_mask_surface(cr_, smask, mx * cx, my * cy);
    }
    cairo_surface_destroy(target);    
    cairo_restore(cr_);
  }
     
  return true;
}

bool CairoCanvas::DrawText(double x, double y, const char *text, 
                           const FontInterface *f, const Color &c) {
  if (text == NULL || f == NULL || 
      strcmp(f->ClassType(), CairoFont::kClassType)) {
    return false;    
  }
  
  cairo_move_to(cr_, x, y);
  cairo_set_source_rgba(cr_, c.red, c.green, c.blue, opacity_);
  
  const CairoFont *font = (const CairoFont*)f; 
  PangoLayout *layout = pango_cairo_create_layout(cr_);
  pango_layout_set_text(layout, text, -1);
  pango_layout_set_font_description(layout, font->GetFontDescription());
  pango_cairo_show_layout(cr_, layout);
  g_object_unref(layout);
    
  return true;
}

} // namespace ggadget
