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

#ifndef GGADGETS_BUTTON_ELEMENT_H__
#define GGADGETS_BUTTON_ELEMENT_H__

#include <stdlib.h>
#include "basic_element.h"

namespace ggadget {

class ButtonElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0xb6fb01fd48134377, BasicElement);

  ButtonElement(ElementInterface *parent,
             ViewInterface *view,
             const char *name);
  virtual ~ButtonElement();

  virtual const CanvasInterface *Draw(bool *changed);
  virtual const char *GetTagName() const { return "button"; }

 public:
  /** Gets and sets the file name of default button image. */
  const char *GetImage() const;
  void SetImage(const char *img);

  /** Gets and sets the file name of disabled button image. */
  const char *GetDisabledImage() const;
  void SetDisabledImage(const char *img);
  
  /** Gets and sets the file name of mouse over button image. */
  const char *GetOverImage() const;
  void SetOverImage(const char *img);
  
  /** Gets and sets the file name of mouse down button image. */
  const char *GetDownImage() const;
  void SetDownImage(const char *img);

 public:
  static ElementInterface *CreateInstance(ElementInterface *parent,
                                          ViewInterface *view,
                                          const char *name);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ButtonElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGETS_BUTTON_ELEMENT_H__
