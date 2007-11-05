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

#ifndef GGADGET_CHECKBOX_ELEMENT_H__
#define GGADGET_CHECKBOX_ELEMENT_H__

#include <stdlib.h>
#include "basic_element.h"

namespace ggadget {

class MouseEvent;

class CheckBoxElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0xe53dbec04fe34ea3, BasicElement);

  CheckBoxElement(ElementInterface *parent,
                ViewInterface *view,
                const char *name);
  virtual ~CheckBoxElement();

  virtual void DoDraw(CanvasInterface *canvas,
                      const CanvasInterface *children_canvas);

 public:
  /** Gets and sets the file name of default checkbox image. */
  const char *GetImage() const;
  void SetImage(const char *img);

  /** Gets and sets the file name of disabled checkbox image. */
  const char *GetDisabledImage() const;
  void SetDisabledImage(const char *img);

  /** Gets and sets the file name of mouse over checkbox image. */
  const char *GetOverImage() const;
  void SetOverImage(const char *img);

  /** Gets and sets the file name of mouse down checkbox image. */
  const char *GetDownImage() const;
  void SetDownImage(const char *img);
  
  /** Gets and sets the file name of default checkbox image. */
  const char *GetCheckedImage() const;
  void SetCheckedImage(const char *img);

  /** Gets and sets the file name of disabled checked checkbox image. */
  const char *GetCheckedDisabledImage() const;
  void SetCheckedDisabledImage(const char *img);

  /** Gets and sets the file name of mouse over checked checkbox image. */
  const char *GetCheckedOverImage() const;
  void SetCheckedOverImage(const char *img);

  /** Gets and sets the file name of mouse down checked checkbox image. */
  const char *GetCheckedDownImage() const;
  void SetCheckedDownImage(const char *img);
  
  /** Gets and sets whether the checkbox is checked. A checked state is true. */
  bool GetValue() const;
  void SetValue(bool value);
  
  /** Gets and sets whether the checkbox is on the right side. Undocumented. */
  bool IsCheckBoxOnRight() const;
  void SetCheckBoxOnRight(bool right);
  
  virtual bool OnMouseEvent(MouseEvent *event, bool direct,
                            ElementInterface **fired_element);

 public:
  static ElementInterface *CreateInstance(ElementInterface *parent,
                                          ViewInterface *view,
                                          const char *name);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(CheckBoxElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_CHECKBOX_ELEMENT_H__
