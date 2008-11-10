/*
  Copyright 2008 Google Inc.

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
class TextFrame;

class CheckBoxElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0xe53dbec04fe34ea3, BasicElement);

  CheckBoxElement(View *view, const char *name, bool is_checkbox);
  virtual ~CheckBoxElement();

 protected:
  virtual void DoClassRegister();

 public:
  /** Gets and sets the file name of default checkbox image. */
  Variant GetImage() const;
  void SetImage(const Variant &img);

  /** Gets and sets the file name of disabled checkbox image. */
  Variant GetDisabledImage() const;
  void SetDisabledImage(const Variant &img);

  /** Gets and sets the file name of mouse over checkbox image. */
  Variant GetOverImage() const;
  void SetOverImage(const Variant &img);

  /** Gets and sets the file name of mouse down checkbox image. */
  Variant GetDownImage() const;
  void SetDownImage(const Variant &img);

  /** Gets and sets the file name of default checkbox image. */
  Variant GetCheckedImage() const;
  void SetCheckedImage(const Variant &img);

  /** Gets and sets the file name of disabled checked checkbox image. */
  Variant GetCheckedDisabledImage() const;
  void SetCheckedDisabledImage(const Variant &img);

  /** Gets and sets the file name of mouse over checked checkbox image. */
  Variant GetCheckedOverImage() const;
  void SetCheckedOverImage(const Variant &img);

  /** Gets and sets the file name of mouse down checked checkbox image. */
  Variant GetCheckedDownImage() const;
  void SetCheckedDownImage(const Variant &img);

  /** Gets and sets whether the checkbox is checked. A checked state is true. */
  bool GetValue() const;
  void SetValue(bool value);

  /** Gets and sets whether the checkbox is on the right side. Undocumented. */
  bool IsCheckBoxOnRight() const;
  void SetCheckBoxOnRight(bool right);

  /** Gets the text frame containing the caption of this checkbox. */
  TextFrame *GetTextFrame();
  const TextFrame *GetTextFrame() const;

  /** Gets and sets if the button should be rendered with default images. */
  bool IsDefaultRendering() const;
  void SetDefaultRendering(bool default_rendering);

  bool IsCheckBox() const;

  Connection *ConnectOnChangeEvent(Slot0<void> *handler);

 public:
  static BasicElement *CreateCheckBoxInstance(View *view, const char *name);
  static BasicElement *CreateRadioInstance(View *view, const char *name);

 protected:
  virtual void DoDraw(CanvasInterface *canvas);
  virtual EventResult HandleMouseEvent(const MouseEvent &event);
  virtual void GetDefaultSize(double *width, double *height) const;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(CheckBoxElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_CHECKBOX_ELEMENT_H__
