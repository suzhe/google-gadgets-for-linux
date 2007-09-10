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

#include "basic_element.h"
#include "basic_element_impl.h"
#include "view_interface.h"
#include "elements.h"
#include "element_interface.h"
#include "element_factory.h"
#include "common.h"

namespace ggadget {

namespace internal {

BasicElementImpl::BasicElementImpl(ElementInterface *parent,
                                   ViewInterface *view,
                                   const char *name,
                                   ElementInterface *owner)
    : parent_(parent),
      children_(new Elements(ElementFactory::GetInstance(), owner)),
      view_(view),
      hittest_(ElementInterface::HT_DEFAULT),
      cursor_(ElementInterface::CURSOR_ARROW),
      drop_target_(false),
      enabled_(false),
      pin_x_(0.0),
      pin_y_(0.0),
      ppin_x_(0.0),
      ppin_y_(0.0),
      pin_x_relative_(false),
      pin_y_relative_(false),
      rotation_(0.0),
      opacity_(1.0),
      visible_(true),
      width_(0.0),
      height_(0.0),
      x_(0.0),
      y_(0.0),
      pwidth_(0.0),
      pheight_(0.0),
      px_(0.0),
      py_(0.0),
      width_relative_(0.0),
      height_relative_(0.0),
      x_relative_(0.0),
      y_relative_(0.0) {
  if (name)
    name_ = name;
}

BasicElementImpl::~BasicElementImpl() {
  delete children_;
}

ViewInterface *BasicElementImpl::GetView() const {
  return view_;
}

ElementInterface::HitTest BasicElementImpl::GetHitTest() const {
  return hittest_;
}

void BasicElementImpl::SetHitTest(ElementInterface::HitTest value) {
  hittest_ = value;
}

ElementsInterface *BasicElementImpl::GetChildren() {
  return children_;
}

ElementInterface::CursorType BasicElementImpl::GetCursor() const {
  return cursor_;
}

void BasicElementImpl::SetCursor(ElementInterface::CursorType cursor) {
  cursor_ = cursor;
}

bool BasicElementImpl::IsDropTarget() const {
  return drop_target_;
}

void BasicElementImpl::SetDropTarget(bool drop_target) {
  drop_target_ = drop_target;
}

bool BasicElementImpl::IsEnabled() const {
  return enabled_;
}

void BasicElementImpl::SetEnabled(bool enabled) {
  enabled_ = enabled;
}

const char *BasicElementImpl::GetName() const {
  return name_.c_str();
}

const char *BasicElementImpl::GetMask() const {
  return mask_.c_str();
}

void BasicElementImpl::SetMask(const char *mask) {
  if (mask)
    mask_ = mask;
  else
    mask_.clear();
}

double BasicElementImpl::GetPixelWidth() const {
  return width_;
}

void BasicElementImpl::SetPixelWidth(double width) {
  if (width >= 0.0) {
    width_ = width;
    width_relative_ = false;
    double p = GetParentWidth();
    if (p > 0.0)
      pwidth_ = width_ / p;
    WidthChanged();
  }
}

double BasicElementImpl::GetPixelHeight() const {
  return height_;
}

void BasicElementImpl::SetPixelHeight(double height) {
  if (height >= 0.0) {
    height_ = height;
    height_relative_ = false;
    double p = GetParentHeight();
    if (p > 0.0)
      pheight_ = height_ / p;
    HeightChanged();
  }
}

double BasicElementImpl::GetRelativeWidth() const {
  return pwidth_;
}

double BasicElementImpl::GetRelativeHeight() const {
  return pheight_;
}

double BasicElementImpl::GetPixelX() const {
  return x_;
}

void BasicElementImpl::SetPixelX(double x) {
  x_ = x;
  double p = GetParentWidth();
  if (p > 0.0)
    px_ = x_ / p;
  x_relative_ = false;
}

double BasicElementImpl::GetPixelY() const {
  return y_;
}

void BasicElementImpl::SetPixelY(double y) {
  y_ = y;
  double p = GetParentHeight();
  if (p > 0.0)
    py_ = y_ / p;
  y_relative_ = false;
}

double BasicElementImpl::GetRelativeX() const {
  return px_;
}

double BasicElementImpl::GetRelativeY() const {
  return py_;
}

double BasicElementImpl::GetPixelPinX() const {
  return pin_x_;
}

void BasicElementImpl::SetPixelPinX(double pin_x) {
  pin_x_ = pin_x;
  if (width_ > 0.0)
    ppin_x_ = pin_x / width_;
  else
    ppin_x_ = 0.0;
  pin_x_relative_ = false;
}

double BasicElementImpl::GetPixelPinY() const {
  return pin_y_;
}

void BasicElementImpl::SetPixelPinY(double pin_y) {
  pin_y_ = pin_y;
  if (height_ > 0.0)
    ppin_y_ = pin_y / height_;
  else
    ppin_y_ = 0.0;
  pin_y_relative_ = false;
}

void BasicElementImpl::SetRelativeWidth(double width) {
  if (width >= 0.0) {
    pwidth_ = width;
    width_ = width * GetParentWidth();
    width_relative_ = true;
    WidthChanged();
  }
}

void BasicElementImpl::SetRelativeHeight(double height) {
  if (height >= 0.0) {
    pheight_ = height;
    height_ = height * GetParentHeight();
    height_relative_ = true;
    HeightChanged();
  }
}

void BasicElementImpl::SetRelativePinX(double pin_x) {
  ppin_x_ = pin_x;
  pin_x_ = pin_x * width_;
  pin_x_relative_ = true;
}

void BasicElementImpl::SetRelativePinY(double pin_y) {
  ppin_y_ = pin_y;
  pin_y_ = pin_y * height_;
  pin_y_relative_ = true;
}

double BasicElementImpl::GetRelativePinX() const {
  return ppin_x_;
}

double BasicElementImpl::GetRelativePinY() const {
  return ppin_y_;
}

void BasicElementImpl::SetRelativeX(double x) {
  px_ = x;
  x_ = x * GetParentWidth();
  x_relative_ = true;
}

void BasicElementImpl::SetRelativeY(double y) {
  py_ = y;
  y_ = y * GetParentHeight();
  y_relative_ = true;
}

double BasicElementImpl::GetRotation() const {
  return rotation_;
}

void BasicElementImpl::SetRotation(double rotation) {
  rotation_ = rotation;
}

double BasicElementImpl::GetOpacity() const {
  return opacity_;
}

void BasicElementImpl::SetOpacity(double opacity) {
  if (opacity >= 0.0 && opacity <= 1.0) {
    opacity_ = opacity;
  }
}

bool BasicElementImpl::IsVisible() const {
  return visible_;
}

void BasicElementImpl::SetVisible(bool visible) {
  visible_ = visible;
}

ElementInterface *BasicElementImpl::GetParentElement() const {
  return parent_;
}

const char *BasicElementImpl::GetToolTip() const {
  return tool_tip_.c_str();
}

void BasicElementImpl::SetToolTip(const char *tool_tip) {
  if (tool_tip)
    tool_tip_ = tool_tip;
  else
    tool_tip_.clear();
}

ElementInterface *BasicElementImpl::AppendElement(const char *tag_name,
                                                  const char *name) {
  ASSERT(children_);
  return children_->AppendElement(tag_name, name);
}

ElementInterface *BasicElementImpl::InsertElement(
    const char *tag_name, const ElementInterface *before, const char *name) {
  ASSERT(children_);
  return children_->InsertElement(tag_name, before, name);
}

bool BasicElementImpl::RemoveElement(ElementInterface *child) {
  return children_->RemoveElement(child);
}

void BasicElementImpl::RemoveAllElements() {
  return children_->RemoveAllElements();
}

void BasicElementImpl::Focus() {
}

void BasicElementImpl::KillFocus() {
}

double BasicElementImpl::GetParentWidth() const {
  const ElementInterface *p;
  const ViewInterface *v;
  if ((p = GetParentElement()))
    return p->GetPixelWidth();
  else if ((v = GetView()))
    return v->GetPixelWidth();
  return 0.0;
}

double BasicElementImpl::GetParentHeight() const {
  const ElementInterface *p;
  const ViewInterface *v;
  if ((p = GetParentElement()))
    return p->GetPixelHeight();
  else if ((v = GetView()))
    return v->GetPixelHeight();
  return 0.0;
}

void BasicElementImpl::WidthChanged() {
  if (pin_x_relative_)
    SetRelativePinX(GetRelativePinX());
  ElementsInterface *children = GetChildren();
  for (int i = 0; i < children->GetCount(); ++i) {
    ElementInterface *element = children->GetItemByIndex(i);
    if (element->XIsRelative())
      element->SetRelativeX(element->GetRelativeX());
    if (element->WidthIsRelative())
      element->SetRelativeWidth(element->GetRelativeWidth());
  }
}

void BasicElementImpl::HeightChanged() {
  if (pin_y_relative_)
    SetRelativePinY(GetRelativePinY());
  ElementsInterface *children = GetChildren();
  for (int i = 0; i < children->GetCount(); ++i) {
    ElementInterface *element = children->GetItemByIndex(i);
    if (element->YIsRelative())
      element->SetRelativeY(element->GetRelativeY());
    if (element->HeightIsRelative())
      element->SetRelativeHeight(element->GetRelativeHeight());
  }
}

bool BasicElementImpl::XIsRelative() const {
  return x_relative_;
}

bool BasicElementImpl::YIsRelative() const {
  return y_relative_;
}

bool BasicElementImpl::WidthIsRelative() const {
  return width_relative_;
}

bool BasicElementImpl::HeightIsRelative() const {
  return height_relative_;
}

bool BasicElementImpl::PinXIsRelative() const {
  return pin_x_relative_;
}

bool BasicElementImpl::PinYIsRelative() const {
  return pin_y_relative_;
}

} // namespace internal

BasicElement::BasicElement(ElementInterface *parent,
                           ViewInterface *view,
                           const char *name)
    : impl_(new internal::BasicElementImpl(parent, view, name, this)) {
}

BasicElement::~BasicElement() {
  delete impl_;
}

void BasicElement::Destroy() {
  delete this;
}

ViewInterface *BasicElement::GetView() {
  ASSERT(impl_);
  return impl_->GetView();
}

const ViewInterface *BasicElement::GetView() const {
  ASSERT(impl_);
  return impl_->GetView();
}

ElementInterface::HitTest BasicElement::GetHitTest() const {
  ASSERT(impl_);
  return impl_->GetHitTest();
}

void BasicElement::SetHitTest(HitTest value) {
  ASSERT(impl_);
  impl_->SetHitTest(value);
}

const ElementsInterface *BasicElement::GetChildren() const {
  ASSERT(impl_);
  return impl_->GetChildren();
}

ElementsInterface *BasicElement::GetChildren() {
  ASSERT(impl_);
  return impl_->GetChildren();
}

ElementInterface::CursorType BasicElement::GetCursor() const {
  ASSERT(impl_);
  return impl_->GetCursor();
}

void BasicElement::SetCursor(ElementInterface::CursorType cursor) {
  ASSERT(impl_);
  impl_->SetCursor(cursor);
}

bool BasicElement::IsDropTarget() const {
  ASSERT(impl_);
  return impl_->IsDropTarget();
}

void BasicElement::SetDropTarget(bool drop_target) {
  ASSERT(impl_);
  return impl_->SetDropTarget(drop_target);
}

bool BasicElement::IsEnabled() const {
  ASSERT(impl_);
  return impl_->IsEnabled();
}

void BasicElement::SetEnabled(bool enabled) {
  ASSERT(impl_);
  impl_->SetEnabled(enabled);
}

const char *BasicElement::GetName() const {
  ASSERT(impl_);
  return impl_->GetName();
}

const char *BasicElement::GetMask() const {
  ASSERT(impl_);
  return impl_->GetMask();
}

void BasicElement::SetMask(const char *mask) {
  ASSERT(impl_);
  return impl_->SetMask(mask);
}

double BasicElement::GetPixelWidth() const {
  ASSERT(impl_);
  return impl_->GetPixelWidth();
}

void BasicElement::SetPixelWidth(double width) {
  ASSERT(impl_);
  return impl_->SetPixelWidth(width);
}

double BasicElement::GetPixelHeight() const {
  ASSERT(impl_);
  return impl_->GetPixelHeight();
}

void BasicElement::SetPixelHeight(double height) {
  ASSERT(impl_);
  return impl_->SetPixelHeight(height);
}

double BasicElement::GetRelativeWidth() const {
  ASSERT(impl_);
  return impl_->GetRelativeWidth();
}

double BasicElement::GetRelativeHeight() const {
  ASSERT(impl_);
  return impl_->GetRelativeHeight();
}

double BasicElement::GetPixelX() const {
  ASSERT(impl_);
  return impl_->GetPixelX();
}

void BasicElement::SetPixelX(double x) {
  ASSERT(impl_);
  return impl_->SetPixelX(x);
}

double BasicElement::GetPixelY() const {
  ASSERT(impl_);
  return impl_->GetPixelY();
}

void BasicElement::SetPixelY(double y) {
  ASSERT(impl_);
  return impl_->SetPixelY(y);
}

double BasicElement::GetRelativeX() const {
  ASSERT(impl_);
  return impl_->GetRelativeX();
}

double BasicElement::GetRelativeY() const {
  ASSERT(impl_);
  return impl_->GetRelativeY();
}

double BasicElement::GetPixelPinX() const {
  ASSERT(impl_);
  return impl_->GetPixelPinX();
}

void BasicElement::SetPixelPinX(double pin_x) {
  ASSERT(impl_);
  return impl_->SetPixelPinX(pin_x);
}

double BasicElement::GetPixelPinY() const {
  ASSERT(impl_);
  return impl_->GetPixelPinY();
}

void BasicElement::SetPixelPinY(double pin_y) {
  ASSERT(impl_);
  impl_->SetPixelPinY(pin_y);
}

double BasicElement::GetRotation() const {
  ASSERT(impl_);
  return impl_->GetRotation();
}

void BasicElement::SetRotation(double rotation) {
  ASSERT(impl_);
  impl_->SetRotation(rotation);
}

double BasicElement::GetOpacity() const {
  ASSERT(impl_);
  return impl_->GetOpacity();
}

void BasicElement::SetOpacity(double opacity) {
  ASSERT(impl_);
  return impl_->SetOpacity(opacity);
}

bool BasicElement::IsVisible() const {
  ASSERT(impl_);
  return impl_->IsVisible();
}

void BasicElement::SetVisible(bool visible) {
  ASSERT(impl_);
  impl_->SetVisible(visible);
}

ElementInterface *BasicElement::GetParentElement() {
  ASSERT(impl_);
  return impl_->GetParentElement();
}

const ElementInterface *BasicElement::GetParentElement() const {
  ASSERT(impl_);
  return impl_->GetParentElement();
}

const char *BasicElement::GetToolTip() const {
  ASSERT(impl_);
  return impl_->GetToolTip();
}

void BasicElement::SetToolTip(const char *tool_tip) {
  ASSERT(impl_);
  return impl_->SetToolTip(tool_tip);
}

ElementInterface *BasicElement::AppendElement(const char *tag_name,
                                              const char *name) {
  ASSERT(impl_);
  return impl_->AppendElement(tag_name, name);
}

ElementInterface *BasicElement::InsertElement(const char *tag_name,
                                              const ElementInterface *before,
                                              const char *name) {
  ASSERT(impl_);
  return impl_->InsertElement(tag_name, before, name);
}

bool BasicElement::RemoveElement(ElementInterface *child) {
  ASSERT(impl_);
  return impl_->RemoveElement(child);
}

void BasicElement::RemoveAllElements() {
  ASSERT(impl_);
  impl_->RemoveAllElements();
}

void BasicElement::Focus() {
  ASSERT(impl_);
  impl_->Focus();
}

void BasicElement::KillFocus() {
  ASSERT(impl_);
  impl_->KillFocus();
}

void BasicElement::SetRelativeWidth(double width) {
  ASSERT(impl_);
  impl_->SetRelativeWidth(width);
}

void BasicElement::SetRelativeHeight(double height) {
  ASSERT(impl_);
  impl_->SetRelativeHeight(height);
}

void BasicElement::SetRelativeX(double x) {
  ASSERT(impl_);
  impl_->SetRelativeX(x);
}

void BasicElement::SetRelativeY(double y) {
  ASSERT(impl_);
  impl_->SetRelativeY(y);
}

double BasicElement::GetRelativePinX() const {
  ASSERT(impl_);
  return impl_->GetRelativePinX();
}

void BasicElement::SetRelativePinX(double x) {
  ASSERT(impl_);
  impl_->SetRelativePinX(x);
}

double BasicElement::GetRelativePinY() const {
  ASSERT(impl_);
  return impl_->GetRelativePinY();
}

void BasicElement::SetRelativePinY(double y) {
  ASSERT(impl_);
  return impl_->SetRelativePinY(y);
}

bool BasicElement::XIsRelative() const {
  ASSERT(impl_);
  return impl_->XIsRelative();
}

bool BasicElement::YIsRelative() const {
  ASSERT(impl_);
  return impl_->YIsRelative();
}

bool BasicElement::WidthIsRelative() const {
  ASSERT(impl_);
  return impl_->WidthIsRelative();
}

bool BasicElement::HeightIsRelative() const {
  ASSERT(impl_);
  return impl_->HeightIsRelative();
}

bool BasicElement::PinXIsRelative() const {
  ASSERT(impl_);
  return impl_->PinXIsRelative();
}

bool BasicElement::PinYIsRelative() const {
  ASSERT(impl_);
  return impl_->PinYIsRelative();
}

DELEGATE_SCRIPTABLE_INTERFACE_IMPL(BasicElement, impl_->static_scriptable_)

} // namespace ggadget
