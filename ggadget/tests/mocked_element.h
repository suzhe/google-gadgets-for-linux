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

#ifndef GGADGET_TESTS_MOCKED_ELEMENT_H__
#define GGADGET_TESTS_MOCKED_ELEMENT_H__

#include <string>
#include "ggadget/element_interface.h"
#include "ggadget/elements.h"
#include "ggadget/math_utils.h"
#include "ggadget/scriptable_helper.h"
#include "ggadget/view_interface.h"
#include "ggadget/event.h"

class MockedElement :
    public ggadget::ScriptableHelper<ggadget::ElementInterface> {
 public:
  DEFINE_CLASS_ID(0x4d0e8e629a744384, ggadget::ElementInterface);

  MockedElement(ggadget::ElementInterface *parent,
                ggadget::ViewInterface *view,
                const char *name)
      : parent_(parent), view_(view) {
    if (name)
      name_ = name;
  }

  virtual ~MockedElement() {
  }

 public:
  virtual void Destroy() {
    delete this;
  }

  virtual ggadget::ViewInterface *GetView() {
    return view_;
  }

  virtual const ggadget::ViewInterface *GetView() const {
    return view_;
  }

  ElementInterface::HitTest GetHitTest() const {
    return ElementInterface::HT_DEFAULT;
  }

  void SetHitTest(ElementInterface::HitTest value) {
  }

  virtual const ggadget::Elements *GetChildren() const {
    return NULL;
  }

  virtual ggadget::Elements *GetChildren() {
    return NULL;
  }

  virtual ggadget::ElementInterface::CursorType GetCursor() const {
    return ggadget::ElementInterface::CURSOR_ARROW;
  }

  virtual void SetCursor(ggadget::ElementInterface::CursorType cursor) {
  }

  virtual bool IsDropTarget() const {
    return false;
  }

  virtual void SetDropTarget(bool drop_target) {
  }

  virtual bool IsEnabled() const {
    return false;
  }

  virtual void SetEnabled(bool enabled) {
  }

  virtual const char *GetName() const {
    return name_.c_str();
  }

  virtual const char *GetMask() const {
    return "";
  }

  virtual void SetMask(const char *mask) {
  }

  virtual double GetPixelWidth() const {
    return double(100);
  }

  virtual void SetPixelWidth(double width) {
  }

  virtual double GetPixelHeight() const {
    return double(100);
  }

  virtual void SetPixelHeight(double height) {
  }

  virtual double GetRelativeWidth() const {
    return 100;
  }

  virtual double GetRelativeHeight() const {
    return 100;
  }

  virtual double GetPixelX() const {
    return double(0);
  }

  virtual void SetPixelX(double x) {
  }

  virtual double GetPixelY() const {
    return double(0);
  }

  virtual void SetPixelY(double y) {
  }

  virtual double GetRelativeX() const {
    return 0;
  }

  virtual double GetRelativeY() const {
    return 0;
  }

  virtual double GetPixelPinX() const {
    return 0;
  }

  virtual void SetPixelPinX(double pin_x) {
  }

  virtual double GetPixelPinY() const {
    return 0;
  }

  virtual void SetPixelPinY(double pin_y) {
  }

  virtual double GetRotation() const {
    return 0.0;
  }

  virtual void SetRotation(double rotation) {
  }

  virtual double GetOpacity() const {
    return 0;
  }

  virtual void SetOpacity(double opacity) {
  }

  virtual bool IsVisible() const {
    return true;
  }

  virtual void SetVisible(bool visible) {
  }

  virtual ElementInterface *GetParentElement() {
    return parent_;
  }

  virtual const ElementInterface *GetParentElement() const {
    return parent_;
  }

  virtual const char *GetTooltip() const {
    return "";
  }

  virtual void SetTooltip(const char *tool_tip) {
  }

  virtual void Focus() {
  }

  virtual void KillFocus() {
  }

  virtual void SetRelativeWidth(double) {
  }

  virtual void SetRelativeHeight(double) {
  }

  virtual void SetRelativeX(double) {
  }

  virtual void SetRelativeY(double) {
  }

  virtual double GetRelativePinX() const {
    return 0.0;
  }

  virtual void SetRelativePinX(double) {
  }

  virtual double GetRelativePinY() const {
    return 0.0;
  }

  virtual void SetRelativePinY(double) {
  }

  virtual bool XIsRelative() const {
    return false;
  }

  virtual bool YIsRelative() const {
    return false;
  }

  virtual bool WidthIsRelative() const {
    return false;
  }

  virtual bool HeightIsRelative() const {
    return false;
  }

  virtual bool PinXIsRelative() const {
    return false;
  }

  virtual bool PinYIsRelative() const {
    return false;
  }

  virtual bool WidthIsSpecified() const {
    return false;
  }

  virtual bool HeightIsSpecified() const {
    return false;
  }

  virtual const ggadget::CanvasInterface *GetMaskCanvas() {
    return NULL;
  }

  virtual const ggadget::CanvasInterface *Draw(bool *changed) {
    return NULL;
  }

  virtual bool IsPositionChanged() const {
    return true;
  }

  virtual void ClearPositionChanged() {
  }

  virtual void OnParentWidthChange(double width) {
  }

  virtual void OnParentHeightChange(double height) {
  }

  virtual bool OnMouseEvent(ggadget::MouseEvent *event, bool direct,
                            ggadget::ElementInterface **fired_element) {
    *fired_element = this;
    return true;
  }

  virtual bool IsMouseEventIn(ggadget::MouseEvent *event) {
    return true;
  }

  virtual bool OnKeyEvent(ggadget::KeyboardEvent *event) {
    return true;
  }

  virtual bool OnOtherEvent(ggadget::Event *event) {
    return true;
  }

  virtual void SelfCoordToChildCoord(ElementInterface *child,
                                     double x, double y,
                                     double *child_x, double *child_y) {
    ggadget::ParentCoordToChildCoord(
        x, y, child->GetPixelX(), child->GetPixelY(),
        child->GetPixelPinX(), child->GetPixelPinY(),
        ggadget::DegreesToRadians(child->GetRotation()),
        child_x, child_y);
  }

 private:
  std::string name_;
  ggadget::ElementInterface *parent_;
  ggadget::ViewInterface *view_;
};

#endif // GGADGETS_TESTS_MOCKED_ELEMENT_H__
