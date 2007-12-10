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

#ifndef GGADGET_SCROLLING_ELEMENT_H__
#define GGADGET_SCROLLING_ELEMENT_H__

#include <stdlib.h>
#include <ggadget/basic_element.h>

namespace ggadget {

class ScrollingElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0x17107e53044c40f2, BasicElement);

 protected:
  ScrollingElement(BasicElement *parent, View *view,
                   const char *tag_name, const char *name,
                   Elements *children);
  virtual ~ScrollingElement();

 public:
  /**
   * Gets and sets the autoscroll property.
   * @c true if the div automatically shows scrollbars if necessary; @c false
   * if it doesn't show scrollbars. Default is @c false.
   */
  bool IsAutoscroll() const;
  void SetAutoscroll(bool autoscroll);

  /** Scroll horizontally. */
  void ScrollX(int distance);
  /** Scroll vertically. */
  void ScrollY(int distance);
  /** Gets and Sets the absolute position. */
  int GetScrollXPosition() const;
  void SetScrollXPosition(int pos);
  int GetScrollYPosition() const;
  void SetScrollYPosition(int pos);

  virtual double GetClientWidth();
  virtual double GetClientHeight();
  virtual EventResult OnMouseEvent(const MouseEvent &event, bool direct,
                                   BasicElement **fired_element,
                                   BasicElement **in_element);

  /**
   * Overrides because this element supports scrolling.
   * @see ElementInterface::SelfCoordToChildCoord()
   */
  virtual void SelfCoordToChildCoord(const BasicElement *child,
                                     double x, double y,
                                     double *child_x, double *child_y) const;

 protected:
  /**
   * Draws scrollbar on the canvas. Subclasses must call this in their
   * @c DoDraw() method.
   */
  void DrawScrollbar(CanvasInterface *canvas);

  virtual void Layout();
  virtual EventResult HandleMouseEvent(const MouseEvent &event);

  /** Gets the size of the scrollable contents. */
  virtual void GetContentSize(double *width, double *height) = 0;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScrollingElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_SCROLLING_ELEMENT_H__
