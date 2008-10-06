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

#ifndef GGADGET_PROGRESSBAR_ELEMENT_H__
#define GGADGET_PROGRESSBAR_ELEMENT_H__

#include <cstdlib>
#include <ggadget/basic_element.h>

namespace ggadget {

class MouseEvent;

class ProgressBarElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0x2808145fd57747c0, BasicElement);

  enum Orientation {
    ORIENTATION_VERTICAL,
    ORIENTATION_HORIZONTAL
  };

  ProgressBarElement(View *view, const char *name);
  virtual ~ProgressBarElement();

 public:
  /** Gets and sets the file name of the image when the slider is empty. */
  Variant GetEmptyImage() const;
  void SetEmptyImage(const Variant &img);

  /** Gets and sets the file name of the image when the slider is full. */
  Variant GetFullImage() const;
  void SetFullImage(const Variant &img);

  /** Gets and sets the file name of the thumb disabled image. */
  Variant GetThumbDisabledImage() const;
  void SetThumbDisabledImage(const Variant &img);

  /** Gets and sets the file name of the thumb  down image. */
  Variant GetThumbDownImage() const;
  void SetThumbDownImage(const Variant &img);

  /** Gets and sets the file name of the thumb image. */
  Variant GetThumbImage() const;
  void SetThumbImage(const Variant &img);

  /** Gets and sets the file name of the thumb hover image. */
  Variant GetThumbOverImage() const;
  void SetThumbOverImage(const Variant &img);

   /** Gets and sets the scrollbar orientation (horizontal, vertical). */
  Orientation GetOrientation() const;
  void SetOrientation(Orientation o);

  /** Gets and sets the max scrollbar value. */
  int GetMax() const;
  void SetMax(int value);

  /** Gets and sets the min scrollbar value. */
  int GetMin() const;
  void SetMin(int value);

  /** Gets and sets the scroll position of the thumb. */
  int GetValue() const;
  void SetValue(int value);

  /** Gets and sets if the button should be rendered with default images. */
  bool IsDefaultRendering() const;
  void SetDefaultRendering(bool default_rendering);

  Connection *ConnectOnChangeEvent(Slot0<void> *handler);

 protected:
  virtual void DoClassRegister();
  virtual void DoDraw(CanvasInterface *canvas);
  virtual EventResult HandleMouseEvent(const MouseEvent &event);
  virtual void GetDefaultSize(double *width, double *height) const;

 public:
  virtual bool HasOpaqueBackground() const;

 public:
  static BasicElement *CreateInstance(View *view, const char *name);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ProgressBarElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_PROGRESSBAR_ELEMENT_H__
