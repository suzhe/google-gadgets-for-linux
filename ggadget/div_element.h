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

#ifndef GGADGET_DIV_ELEMENT_H__
#define GGADGET_DIV_ELEMENT_H__

#include <stdlib.h>
#include <ggadget/basic_element.h>

namespace ggadget {

class DivElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0xfca426268a584176, BasicElement);

  DivElement(BasicElement *parent, View *view, const char *name);
  virtual ~DivElement();

 public:
  /**
   * Gets and sets the autoscroll property.
   * @c true if the div automatically shows scrollbars if necessary; @c false
   * if it doesn't show scrollbars. Default is @c false.
   */
  bool IsAutoscroll() const;
  void SetAutoscroll(bool autoscroll);

  /**
   * Gets and sets the background color or image of the element. The image is
   * repeated if necessary, not stretched.
   */
  Variant GetBackground() const;
  void SetBackground(const Variant &background);

  virtual EventResult OnMouseEvent(const MouseEvent &event, bool direct,
                                   BasicElement **fired_element,
                                   BasicElement **in_element);
  virtual EventResult OnDragEvent(const DragEvent &event, bool direct,
                                  BasicElement **fired_element);

  /**
   * Overrides because this element supports scrolling.
   * @see ElementInterface::SelfCoordToChildCoord()
   */
  virtual void SelfCoordToChildCoord(const BasicElement *child,
                                     double x, double y,
                                     double *child_x, double *child_y) const;

 public:
  static BasicElement *CreateInstance(BasicElement *parent, View *view,
                                      const char *name);

 protected:
  /**
   * Used to subclass div elements.
   * No scriptable interfaces will be registered in this constructor.
   */
  DivElement(BasicElement *parent, View *view,
             const char *tag_name, const char *name, Elements *children);

  virtual void DoDraw(CanvasInterface *canvas,
                      const CanvasInterface *children_canvas);
  virtual EventResult HandleMouseEvent(const MouseEvent &event);
  virtual EventResult HandleKeyEvent(const KeyboardEvent &event);
  virtual void OnWidthChange();
  virtual void OnHeightChange();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(DivElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_DIV_ELEMENT_H__
