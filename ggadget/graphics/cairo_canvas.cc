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
#include "../scoped_ptr.h"
#include <vector>
#include <algorithm>

namespace ggadget {

const char *CairoCanvas::kClassType = "CairoCanvas";

CairoCanvas::CairoCanvas(cairo_t *cr, size_t w, size_t h, bool is_mask) 
  : cr_(cr), width_(w), height_(h), is_mask_(is_mask), opacity_(1.) {
  cairo_reference(cr_);
  // Many CairoCanvas methods assume no existing path, so clear any 
  // existing paths on construction.
  cairo_new_path(cr_); 
}
 
CairoCanvas::~CairoCanvas() {
  cairo_destroy(cr_); 
}

void CairoCanvas::ClearSurface() {
  cairo_operator_t op = cairo_get_operator(cr_);
  cairo_set_operator(cr_, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr_);
  cairo_set_operator(cr_, op);
}

bool CairoCanvas::ClearCanvas() {
  ClearSurface();
  
  // Reset state.
  cairo_reset_clip(cr_);
  opacity_ = 1.;
  opacity_stack_ = std::stack<double>();
  return true;
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

bool CairoCanvas::DrawText(double x, double y, double width, double height,
                           const char *text, const FontInterface *f, 
                           const Color &c, Alignment align, VAlignment valign,
                           Trimming trimming,  TextFlag text_flag) {
  if (text == NULL || f == NULL || 
      strcmp(f->ClassType(), CairoFont::kClassType)) {
    return false;    
  }

  // If the text is blank, we need to do nothing.
  if (strlen(text) == 0) return true;

  const CairoFont *font = down_cast<const CairoFont*>(f); 
  PangoLayout *layout = pango_cairo_create_layout(cr_);
  cairo_set_source_rgba(cr_, c.red, c.green, c.blue, opacity_);
  pango_layout_set_text(layout, text, -1);
  pango_layout_set_font_description(layout, font->GetFontDescription());
  PangoAttrList *attr_list = pango_attr_list_new();
  PangoAttribute *underline_attr = NULL;
  PangoAttribute *strikeout_attr = NULL;
  // Pos is used to get glyph extents in pango.
  PangoRectangle pos;
  // real_x and real_y represent the real position of the layout.
  double real_x = x, real_y = y;
  const char *const kEllipsisText = "...";
  cairo_save(cr_);

  // Restrict the output area.
  cairo_rectangle(cr_, x, y, x + width, y + height);
  cairo_clip(cr_);
  
  // Set the underline attribute
  if (text_flag & TEXT_FLAGS_UNDERLINE) {
    underline_attr = pango_attr_underline_new(PANGO_UNDERLINE_SINGLE);
    // We want this attribute apply to all text.
    underline_attr->start_index = 0;
    underline_attr->end_index = 0xFFFFFFFF;
    pango_attr_list_insert(attr_list, underline_attr);
  } 
  // Set the strikeout attribute.
  if (text_flag & TEXT_FLAGS_STRIKEOUT) {
    strikeout_attr = pango_attr_strikethrough_new(true);
    // We want this attribute apply to all text.
    strikeout_attr->start_index = 0;
    strikeout_attr->end_index = 0xFFFFFFFF;
    pango_attr_list_insert(attr_list, strikeout_attr);
  }
  // Set the wordwrap attribute.
  if (text_flag & TEXT_FLAGS_WORDWRAP) {
    pango_layout_set_width(layout, static_cast<int>(width) * PANGO_SCALE);
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
  } else {
    // In pango, set width = -1 to set no wordwrap.
    pango_layout_set_width(layout, -1);
  }
  pango_layout_set_attributes(layout, attr_list);

  // Set alignment. This is only effective when wordwrap is set
  // because when wordwrap is unsert, the width have to be set
  // to -1, thus the alignment is useless.
  if (align == LEFT) 
    pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
  else if (align == CENTER)
    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
  else if (align == RIGHT)
    pango_layout_set_alignment(layout, PANGO_ALIGN_RIGHT);

  // Get the pixel extents(logical extents) of the layout.
  pango_layout_get_pixel_extents(layout, NULL, &pos);
  // Calculate number of all lines.
  int n_lines = pango_layout_get_line_count(layout);
  int line_height = pos.height / n_lines;
  // Calculate number of lines that could be displayed.
  // We should display one more line as long as there 
  // are 5 pixels of blank left. This is only effective
  // when trimming exists.
  int displayed_lines = (static_cast<int>(height) - 5) / line_height + 1;
  if (displayed_lines > n_lines) displayed_lines = n_lines;

  if (trimming == TRIMMING_NONE || (pos.width <= width && 
        pango_layout_get_line_count(layout) <= displayed_lines)) {
    // When there is no trimming, we can directly show the layout.

    // Set vertical alignment.
    if (valign == MIDDLE)
      real_y = y + (height - pos.height) / 2;
    else if (valign == BOTTOM)
      real_y = y + height - pos.height;

    // When wordwrap is unset, we also have to do the horizontal alignment.
    if ((text_flag & TEXT_FLAGS_WORDWRAP) == 0) {
      if (align == CENTER)
        real_x = x + (width - pos.width) / 2;
      else if (align == RIGHT)
        real_x = x + width - pos.width;
    }

    // Show pango layout when there is no trimming.
    cairo_move_to(cr_, real_x, real_y);
    pango_cairo_show_layout(cr_, layout);

  } else {     
    // We will use newtext as the content of the layout,
    // because we have to display the trimmed text.
    scoped_array<char> newtext(new char[strlen(text) + 4]);

    // Set vertical alignment.
    if (valign == MIDDLE)
      real_y = y + (height - line_height * displayed_lines) / 2;
    else if (valign == BOTTOM)
      real_y = y + height - line_height * displayed_lines;

    if (displayed_lines > 1) {
      // When there are multilines, we will show the above lines first, 
      // because trimming will only occurs in the last line.
      PangoLayoutLine *line = pango_layout_get_line(layout, 
                                                    displayed_lines - 2);
      int last_line_index = line->start_index + line->length;
      pango_layout_set_text(layout, text, last_line_index);
      cairo_move_to(cr_, real_x, real_y);
      pango_cairo_show_layout(cr_, layout);

      // The newtext contains the text that will be shown in the last line.
      strcpy(newtext.get(), text + last_line_index);
      real_y += line_height * (displayed_lines - 1);

    } else {
      // When there is only a single line, the newtext equals text.
      strcpy(newtext.get(), text);
    }
    // Set the newtext as the content of the layout.
    pango_layout_set_text(layout, newtext.get(), -1);

    // This record the width of the ellipsis text.
    int ellipsis_width = 0;

    if (trimming == TRIMMING_CHARACTER_ELLIPSIS) {
      // Pango has provided character-ellipsis trimming.
      // FIXME: when displaying arabic, the final layout width
      // may exceed the width we set before
      pango_layout_set_width(layout, static_cast<int>(width) * PANGO_SCALE);
      pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
      
    } else if (trimming == TRIMMING_PATH_ELLIPSIS) {
      // Pango has provided path-ellipsis trimming.
      // FIXME: when displaying arabic, the final layout width
      // may exceed the width we set before
      pango_layout_set_width(layout, static_cast<int>(width) * PANGO_SCALE);
      pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_MIDDLE);

    } else {
      // We have to do other type of trimming ourselves, including 
      // "character", "word" and "word-ellipsis".

      // We want every thing in a single line, so set no word wrap.
      pango_layout_set_width(layout, -1); 
      pango_layout_get_pixel_extents(layout, NULL, &pos);
      if (trimming == TRIMMING_WORD_ELLIPSIS) {
        // Only in this condition should we calculate the ellipsis width.
        pango_layout_set_text(layout, kEllipsisText, -1);
        pango_layout_get_pixel_extents(layout, NULL, &pos);
        ellipsis_width = pos.width;
        pango_layout_set_text(layout, newtext.get(), -1);
      }

      // Figure out how many characters can be displayed.
      std::vector<int> cluster_index;
      PangoLayoutIter *it = pango_layout_get_iter(layout);
      // A cluster is the smallest linguistic unit that can be shaped.
      do {
        cluster_index.push_back(pango_layout_iter_get_index(it));
      } while (pango_layout_iter_next_cluster(it));
      cluster_index.push_back(strlen(newtext.get()));
      std::sort(cluster_index.begin(), cluster_index.end());

      std::vector<int>::iterator cluster_it = cluster_index.begin();
      for (; cluster_it != cluster_index.end(); ++cluster_it) {
        pango_layout_set_text(layout, newtext.get(), *cluster_it);
        pango_layout_get_pixel_extents(layout, NULL, &pos);
        if (pos.width > width - ellipsis_width)
          break;
      }

      // Use conceal_index to represent the first byte that won't be displayed.
      assert(cluster_it != cluster_index.begin());
      int conceal_index = *(--cluster_it);

      // Get the text that will finally be displayed.
      if (trimming == TRIMMING_CHARACTER) {
        // In "character", just show the characters before the index.
        pango_layout_set_text(layout, newtext.get(), conceal_index);

      } else {  
        // In "word" or "word-ellipsis" trimming, we have to find out where
        // last word stops. If we can't find out a reasonable position, then 
        // just do trimming as in "character".
        PangoLogAttr *log_attrs;
        int n_attrs;
        pango_layout_get_log_attrs(layout, &log_attrs, &n_attrs);
        int off = g_utf8_pointer_to_offset(newtext.get(), 
                                           newtext.get() + conceal_index);
        while (off > 0 && !log_attrs[off].is_word_end &&
               !log_attrs[off].is_word_start)
          --off;
        if (off > 0)
          conceal_index = g_utf8_offset_to_pointer(newtext.get(), off) - 
                                                   newtext.get();
          newtext[conceal_index] = 0;

        // In word-ellipsis, we have to append the ellipsis manualy.
        if (trimming == TRIMMING_WORD_ELLIPSIS) 
          strcpy(newtext.get() + conceal_index, kEllipsisText);

        pango_layout_set_text(layout, newtext.get(), -1);
      }

      // We also have to do the horizontal alignment.
      pango_layout_get_pixel_extents(layout, NULL, &pos);
      if (align == CENTER)
        real_x = x + (width - pos.width) / 2;
      else if (align == RIGHT)
        real_x = x + width - pos.width;
    }

    // Show the trimmed text.
    cairo_move_to(cr_, real_x, real_y);
    pango_cairo_show_layout(cr_, layout);
    pango_layout_get_pixel_extents(layout, NULL, &pos);
    PangoLayoutIter *it = pango_layout_get_iter(layout);
    do {
      pango_layout_iter_get_char_extents(it, &pos);
    } while(pango_layout_iter_next_char(it));
  }

  g_object_unref(layout);
  // This will also free underline_attr and strikeout_attr.
  pango_attr_list_unref(attr_list);
  cairo_restore(cr_);

  return true;
}

} // namespace ggadget
