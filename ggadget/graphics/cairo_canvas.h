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

#ifndef GGADGET_GRAPHICS_CAIRO_CANVAS_H__
#define GGADGET_GRAPHICS_CAIRO_CANVAS_H__

#include <cairo.h>
#include <stack> 

#include "ggadget/common.h"
#include "ggadget/canvas_interface.h"

namespace ggadget {

/**
 * This class realizes the CanvasInterface using the Cairo graphics library.
 * Internally, the graphics state is represented by a Cairo context object.
 * The owner of this object should set any necessary Cairo properties before
 * passing the cairo_t to the constructor. This may include operator, clipping, 
 * set initial matrix settings, and clear the drawing surface.
 */
class CairoCanvas : public CanvasInterface {
 public:
  /**
   * Constructs a CairoCanvas object from a Cairo context. CairoCanvas
   * will retain a reference to the cairo_t so it is safe to destroy the Cairo
   * context after constructing this object.
   */
  CairoCanvas(cairo_t *cr, size_t w, size_t h, bool is_mask);
  virtual ~CairoCanvas();

  virtual void Destroy() { delete this; };

  virtual size_t GetWidth() const { return width_; };
  virtual size_t GetHeight() const { return height_; };  

  virtual bool IsMask() const { return is_mask_; };  

  virtual bool PushState();
  virtual bool PopState();

  virtual bool MultiplyOpacity(double opacity);
  virtual void RotateCoordinates(double radians);
  virtual void TranslateCoordinates(double dx, double dy);
  virtual void ScaleCoordinates(double cx, double cy);

  /** Clears the entire surface to be empty. */
  void ClearSurface();
  virtual bool ClearCanvas();

  virtual bool DrawLine(double x0, double y0, double x1, double y1, 
                        double width, const Color &c);
  virtual bool DrawFilledRect(double x, double y, 
                              double w, double h, const Color &c);  

  virtual bool DrawCanvas(double x, double y, const CanvasInterface *img);
  virtual bool DrawFilledRectWithCanvas(double x, double y, 
                                        double w, double h,
                                        const CanvasInterface *img);
  virtual bool DrawCanvasWithMask(double x, double y,
                                  const CanvasInterface *img,
                                  double mx, double my,
                                  const CanvasInterface *mask);

  virtual bool DrawText(double x, double y, double width, double height,
                        const char *text, const FontInterface *f, 
                        const Color &c, Alignment align, VAlignment valign,
                        Trimming trimming, TextFlag text_flag);
  virtual bool DrawTextWithTexture(double x, double y, double width, 
                                   double height, const char *text, 
                                   const FontInterface *f, 
                                   const CanvasInterface *texture, 
                                   Alignment align, VAlignment valign,
                                   Trimming trimming, TextFlag text_flag);
  
  virtual bool IntersectRectClipRegion(double x, double y, 
                                       double w, double h);

  /**
   * Get the surface contained within this class for use elsewhere. 
   * Will flush the surface before returning so it is ready to be read.
   */
  cairo_surface_t *GetSurface() const { 
    cairo_surface_t *s = cairo_get_target(cr_);
    cairo_surface_flush(s);
    return s;
  };   

 private:
   cairo_t *cr_;
   size_t width_, height_;
   bool is_mask_;
   double opacity_;
   std::stack<double> opacity_stack_;

   bool DrawTextInternal(double x, double y, double width, 
                         double height, const char *text, 
                         const FontInterface *f, 
                         Alignment align, VAlignment valign,
                         Trimming trimming, TextFlag text_flag);
   
   DISALLOW_EVIL_CONSTRUCTORS(CairoCanvas);
};

}

#endif // GGADGET_GRAPHICS_CAIRO_CANVAS_H__
