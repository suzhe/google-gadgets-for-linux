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

#ifndef GGADGETS_TEST_MOCKED_ELEMENT_H__
#define GGADGETS_TEST_MOCKED_ELEMENT_H__

#include <string>
#include "ggadget/element_interface.h"
#include "ggadget/view_interface.h"

class MockedElement : public ggadget::ElementInterface {
 public:
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

  virtual const ggadget::ElementsInterface *GetChildren() const {
    return NULL;
  }

  virtual ggadget::ElementsInterface *GetChildren() {
    return NULL;
  }

  virtual const char *GetCursor() const {
    return "arrow";
  }

  virtual bool SetCursor(const char *cursor) {
    return false;
  }

  virtual bool IsDropTarget() const {
    return false;
  }

  virtual bool SetDropTarget(bool drop_target) {
    return false;
  }

  virtual bool IsEnabled() const {
    return false;
  }

  virtual bool SetEnabled(bool enabled) {
    return false;
  }

  virtual const char *GetName() const {
    return name_.c_str();
  }

  virtual const char *GetMask() const {
    return "";
  }

  virtual bool SetMask(const char *mask) const {
    return false;
  }

  virtual ggadget::Variant GetWidth() const {
    return ggadget::Variant(100);
  }

  virtual bool SetWidth(ggadget::Variant width) {
    return false;
  }

  virtual ggadget::Variant GetHeight() const {
    return ggadget::Variant(100);
  }

  virtual bool SetHeight(ggadget::Variant height) {
    return false;
  }

  virtual int GetOffsetWidth() const {
    return 100;
  }

  virtual int GetOffsetHeight() const {
    return 100;
  }

  virtual ggadget::Variant GetX() const {
    return ggadget::Variant(0);
  }

  virtual bool SetX(ggadget::Variant x) {
    return false;
  }

  virtual ggadget::Variant GetY() const {
    return ggadget::Variant(0);
  }

  virtual bool SetY(ggadget::Variant y) {
    return false;
  }

  virtual int GetOffsetX() const {
    return 0;
  }

  virtual int GetOffsetY() const {
    return 0;
  }

  virtual int GetPinX() const {
    return 0;
  }

  virtual bool SetPinX(int pin_x) {
    return false;
  }

  virtual int GetPinY() const {
    return 0;
  }

  virtual bool SetPinY(int pin_y) {
    return false;
  }

  virtual double GetRotation() const {
    return 0.0;
  }

  virtual bool SetRotation(double rotation) {
    return false;
  }

  virtual int GetOpacity() const {
    return 0;
  }

  virtual bool SetOpacity(int opacity) {
    return false;
  }

  virtual bool IsVisible() const {
    return true;
  }

  virtual bool SetVisible(bool visible) {
    return false;
  }

  virtual ElementInterface *GetParentElement() {
    return parent_;
  }

  virtual const ElementInterface *GetParentElement() const {
    return parent_;
  }

  virtual const char *GetToolTip() const {
    return "";
  }

  virtual bool SetToolTip(const char *tool_tip) {
    return false;
  }

  virtual ElementInterface *AppendElement(const char *tag_name) {
    return NULL;
  }

  virtual ElementInterface *InsertElement(const char *tag_name,
                                          const ElementInterface *before) {
    return NULL;
  }

  virtual bool RemoveElement(ElementInterface *child) {
    return false;
  }

  virtual void RemoveAllElements() {
  }

  virtual void Focus() {
  }

  virtual void KillFocus() {
  }

 private:
  std::string name_;
  ggadget::ElementInterface *parent_;
  ggadget::ViewInterface *view_;
};

#endif // GGADGETS_TEST_MOCKED_ELEMENT_H__
