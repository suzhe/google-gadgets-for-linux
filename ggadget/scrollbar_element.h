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

#ifndef GGADGET_SCROLLBAR_ELEMENT_H__
#define GGADGET_SCROLLBAR_ELEMENT_H__

#include <cstdlib>
#include <ggadget/slot.h>
#include <ggadget/basic_element.h>

namespace ggadget {

class MouseEvent;

class ScrollBarElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0x789974a8a14a43c9, BasicElement);

  enum Orientation {
    ORIENTATION_VERTICAL,
    ORIENTATION_HORIZONTAL
  };

  ScrollBarElement(ElementInterface *parent,
                   ViewInterface *view,
                   const char *name);
  virtual ~ScrollBarElement();

  virtual void DoDraw(CanvasInterface *canvas,
                      const CanvasInterface *children_canvas);

 public:
  /** Gets and sets the file name of the background image. */
  Variant GetBackground() const;
  void SetBackground(const Variant &img);

  /** Gets and sets the file name of the left/up button down image. */
  Variant GetLeftDownImage() const;
  void SetLeftDownImage(const Variant &img);

  /** Gets and sets the file name of the left/up image. */
  Variant GetLeftImage() const;
  void SetLeftImage(const Variant &img);

  /** Gets and sets the file name of the left/up hover image. */
  Variant GetLeftOverImage() const;
  void SetLeftOverImage(const Variant &img);

  /** Gets and sets the file name of the right/down button down image. */
  Variant GetRightDownImage() const;
  void SetRightDownImage(const Variant &img);

  /** Gets and sets the file name of the right/down image. */
  Variant GetRightImage() const;
  void SetRightImage(const Variant &img);

  /** Gets and sets the file name of the right/down hover image. */
  Variant GetRightOverImage() const;
  void SetRightOverImage(const Variant &img);

  /** Gets and sets the file name of the thumb button down image. */
  Variant GetThumbDownImage() const;
  void SetThumbDownImage(const Variant &img);

  /** Gets and sets the file name of the thumb  image. */
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

  /** Gets and sets the page step value. */
  int GetPageStep() const;
  void SetPageStep(int value);

  /** Gets and sets the line step value. */
  int GetLineStep() const;
  void SetLineStep(int value);

  /** Gets and sets the scroll position of the thumb. */
  int GetValue() const;
  void SetValue(int value);

  virtual bool OnMouseEvent(MouseEvent *event, bool direct,
                            ElementInterface **fired_element);

  void ConnectOnChangeEvent(Slot0<void> *slot);

 public:
  static ElementInterface *CreateInstance(ElementInterface *parent,
                                          ViewInterface *view,
                                          const char *name);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScrollBarElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_SCROLLBAR_ELEMENT_H__
