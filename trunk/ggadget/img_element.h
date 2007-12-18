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

#ifndef GGADGET_IMG_ELEMENT_H__
#define GGADGET_IMG_ELEMENT_H__

#include <stdlib.h>
#include <ggadget/basic_element.h>

namespace ggadget {

class ImgElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0x95b5791e157d4373, BasicElement);

  ImgElement(BasicElement *parent, View *view, const char *name);
  virtual ~ImgElement();

  virtual bool IsPointIn(double x, double y);
 public:
  enum CropMaintainAspect {
    CROP_FALSE = 0,
    CROP_TRUE,
    CROP_PHOTO
  };

  /**
   * Gets and sets the source of image to display.
   * @see ViewInterface::LoadImage()
   */
  Variant GetSrc() const;
  void SetSrc(const Variant &src);

  /** Gets the original width of the image being displayed. */
  size_t GetSrcWidth() const;

  /** Gets the original height of the image being displayed. */
  size_t GetSrcHeight() const;

  /** Gets and sets the colorMultiply property of the image. */
  std::string GetColorMultiply() const;
  void SetColorMultiply(const char *color);

  /** Gets and sets the cropMaintainAspect property of the image. */
  CropMaintainAspect GetCropMaintainAspect() const;
  void SetCropMaintainAspect(CropMaintainAspect crop);

  /**
   * Resizes the image to specified @a width and @a height via reduced
   * resolution.  If the source image is larger than the display area,
   * using this method to resize the image to the output size will save
   * memory and improve rendering performance.
   */
  void SetSrcSize(size_t width, size_t height);

 public:
  static BasicElement *CreateInstance(BasicElement *parent, View *view,
                                      const char *name);

 protected:
  virtual void DoDraw(CanvasInterface *canvas,
                      const CanvasInterface *children_canvas);
  virtual void GetDefaultSize(double *width, double *height) const;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ImgElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_IMG_ELEMENT_H__
