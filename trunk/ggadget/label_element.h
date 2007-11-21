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

#ifndef GGADGET_LABEL_ELEMENT_H__
#define GGADGET_LABEL_ELEMENT_H__

#include <stdlib.h>
#include <ggadget/basic_element.h>

namespace ggadget {

class TextFrame;

class LabelElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0x4b128d3ef8da40e6, BasicElement);

  LabelElement(ElementInterface *parent,
             ViewInterface *view,
             const char *name);
  virtual ~LabelElement();

  /** Gets the text frame containing the text content of this label. */
  TextFrame *GetTextFrame();

 public:
  static ElementInterface *CreateInstance(ElementInterface *parent,
                                          ViewInterface *view,
                                          const char *name);

 protected:
  virtual void DoDraw(CanvasInterface *canvas,
                      const CanvasInterface *children_canvas);
  virtual void GetDefaultSize(double *width, double *height) const;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(LabelElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_LABEL_ELEMENT_H__
