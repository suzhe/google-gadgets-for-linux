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

#include "ggadget/element_interface.h"

class MockedElement : public ggadget::ElementInterface {
 public:
  MockedElement(ggadget::ElementInterface *parent,
                const char *name) : name_(NULL) {
    if (name) {
      name_ = new char[strlen(name)];
      strcpy(name_, name);
    }
  }

  virtual ~MockedElement() {
    delete name_;
  }

 public:
  virtual void Release() {
    delete this;
  }

  virtual const ggadget::ElementsInterface *children() const {
    return NULL;
  }

  virtual ggadget::ElementsInterface *children() {
    return NULL;
  }

  virtual const char *cursor() const {
    return "arrow";
  }

  virtual bool set_cursor(const char *cursor) {
    return false;
  }

  virtual bool drop_target() const {
    return false;
  }

  virtual bool set_drop_target(bool drop_target) {
    return false;
  }

  virtual bool enabled() const {
    return false;
  }

  virtual bool set_enabled(bool enabled) {
    return false;
  }

  virtual const char *name() const {
    return name_;
  }

  virtual const char *mask() const {
    return "";
  }

  virtual bool set_mask(const char *mask) const {
    return false;
  }

  virtual ggadget::Variant width() const {
    return ggadget::Variant(100);
  }

  virtual bool set_width(ggadget::Variant width) {
    return false;
  }

  virtual ggadget::Variant height() const {
    return ggadget::Variant(100);
  }

  virtual bool set_height(ggadget::Variant height) {
    return false;
  }

  virtual int offset_width() const {
    return 100;
  }

  virtual int offset_height() const {
    return 100;
  }

  virtual ggadget::Variant x() const {
    return ggadget::Variant(0);
  }

  virtual bool set_x(ggadget::Variant x) {
    return false;
  }

  virtual ggadget::Variant y() const {
    return ggadget::Variant(0);
  }

  virtual bool set_y(ggadget::Variant y) {
    return false;
  }

  virtual int offset_x() const {
    return 0;
  }

  virtual int offset_y() const {
    return 0;
  }

  virtual int pin_x() const {
    return 0;
  }

  virtual bool set_pin_x(int pin_x) {
    return false;
  }

  virtual int pin_y() const {
    return 0;
  }

  virtual bool set_pin_y(int pin_y) {
    return false;
  }

  virtual double rotation() const {
    return 0.0;
  }

  virtual bool set_rotation(double rotation) {
    return false;
  }

  virtual int opacity() const {
    return 0;
  }

  virtual bool set_opacity(int opacity) {
    return false;
  }

  virtual bool visible() const {
    return true;
  }

  virtual bool set_visible(bool visible) {
    return false;
  }

  virtual ElementInterface *parent_element() {
    return NULL;
  }

  virtual const ElementInterface *parent_element() const {
    return NULL;
  }

  virtual const char *tool_tip() const {
    return "";
  }

  virtual bool set_tool_tip(const char *tool_tip) {
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
  char *name_;
};

#endif // GGADGETS_TEST_MOCKED_ELEMENT_H__
