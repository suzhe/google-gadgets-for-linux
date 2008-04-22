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

#ifndef GGADGET_VIEW_ELEMENT_H__
#define GGADGET_VIEW_ELEMENT_H__

#include <ggadget/basic_element.h>

namespace ggadget {

/**
 * Internally used element used to allow a View to be a child of another View.
 * This element is not exposed in the API.
 */
class ViewElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0x3be02fb3f45b405b, BasicElement);

  ViewElement(BasicElement *parent, View *parent_view, View *child_view);
  virtual ~ViewElement();

  View *GetChildView() const;
  bool OnSizing(double *width, double *height);
  void SetSize(double width, double height);
 public:
  void SetChildView(View *child_view);

  void SetScale(double scale);

  virtual void Layout();
  virtual void MarkRedraw();
  virtual EventResult HandleMouseEvent(const MouseEvent &event);
  virtual EventResult HandleOtherEvent(const Event &event);

  virtual void GetDefaultSize(double *width, double *height) const;

 protected:
  virtual void DoDraw(CanvasInterface *canvas);
  virtual EventResult HandleDragEvent(const DragEvent &event);
  virtual EventResult HandleKeyEvent(const KeyboardEvent &event);

  // No CreateInstance() method since this class is internal.

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ViewElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_VIEW_ELEMENT_H__
