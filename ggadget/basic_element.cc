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
#include "common.h"
#include "element_factory.h"
#include "element_interface.h"
#include "view_interface.h"

namespace ggadget {

namespace internal {

BasicElementImpl::BasicElementImpl(ElementInterface *parent,
                                   ViewInterface *view,
                                   const char *name,
                                   ElementInterface *owner)
    : parent_(parent),
      children_(ElementFactory::GetInstance(), owner, view),
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

Elements *BasicElementImpl::GetChildren() {
  return &children_;
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

const char *BasicElementImpl::GetTooltip() const {
  return tooltip_.c_str();
}

void BasicElementImpl::SetTooltip(const char *tooltip) {
  if (tooltip)
    tooltip_ = tooltip;
  else
    tooltip_.clear();
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
    return v->GetWidth();
  return 0.0;
}

double BasicElementImpl::GetParentHeight() const {
  const ElementInterface *p;
  const ViewInterface *v;
  if ((p = GetParentElement()))
    return p->GetPixelHeight();
  else if ((v = GetView()))
    return v->GetHeight();
  return 0.0;
}

void BasicElementImpl::WidthChanged() {
  if (pin_x_relative_)
    SetRelativePinX(GetRelativePinX());
  Elements *children = GetChildren();
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
  Elements *children = GetChildren();
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

static int ParsePixelOrRelative(const Variant &input, double *output) {
  switch (input.type()) {
    // The input is a pixel value.
    case Variant::TYPE_INT64:
      *output = VariantValue<int>()(input);
      return 0;
    // The input is a relative percent value.
    case Variant::TYPE_STRING: {
      const char *str_value = VariantValue<const char *>()(input);
      char *end_ptr;
      *output = strtol(str_value, &end_ptr, 10) / 100.0;
      if (*end_ptr == '%' && *(end_ptr + 1) == '\0')
        return 1;
      LOG("Invalid relative value: %s", input.ToString().c_str());
      return -1;
    }
    default:
      LOG("Invalid pixel or relative value: %s", input.ToString().c_str());
      return -1;
  }
}

#define SET_PIXEL_OR_RELATIVE(value, set_pixel, set_relative) \
  double v;                                                   \
  switch (ParsePixelOrRelative(value, &v)) {                  \
    case 0: set_pixel(v); break;                              \
    case 1: set_relative(v); break;                           \
    default: break;                                           \
  }

static Variant GetPixelOrRelative(bool is_relative,
                                  double pixel,
                                  double relative) {
  if (is_relative) {
    char buf[20];
    snprintf(buf, sizeof(buf), "%d%%", static_cast<int>(relative * 100));
    return Variant(std::string(buf));
  } else {
    return Variant(static_cast<int>(pixel));
  }
}

Variant BasicElementImpl::GetWidth() const {
  return GetPixelOrRelative(WidthIsRelative(),
                            GetPixelWidth(),
                            GetRelativeWidth());
}
void BasicElementImpl::SetWidth(const Variant &width) {
  SET_PIXEL_OR_RELATIVE(width, SetPixelWidth, SetRelativeWidth);
}

Variant BasicElementImpl::GetHeight() const {
  return GetPixelOrRelative(HeightIsRelative(),
                            GetPixelHeight(),
                            GetRelativeHeight());
}
void BasicElementImpl::SetHeight(const Variant &height) {
  SET_PIXEL_OR_RELATIVE(height, SetPixelHeight, SetRelativeHeight);
}

Variant BasicElementImpl::GetX() const {
  return GetPixelOrRelative(XIsRelative(),
                            GetPixelX(),
                            GetRelativeX());
}
void BasicElementImpl::SetX(const Variant &x) {
  SET_PIXEL_OR_RELATIVE(x, SetPixelX, SetRelativeX);
}

Variant BasicElementImpl::GetY() const {
  return GetPixelOrRelative(YIsRelative(),
                            GetPixelY(),
                            GetRelativeY());
}
void BasicElementImpl::SetY(const Variant &y) {
  SET_PIXEL_OR_RELATIVE(y, SetPixelY, SetRelativeY);
}

Variant BasicElementImpl::GetPinX() const {
  return GetPixelOrRelative(PinXIsRelative(),
                            GetPixelPinX(),
                            GetRelativePinX());
}
void BasicElementImpl::SetPinX(const Variant &pin_x) {
  SET_PIXEL_OR_RELATIVE(pin_x, SetPixelPinX, SetRelativePinX);
}

Variant BasicElementImpl::GetPinY() const {
  return GetPixelOrRelative(PinYIsRelative(),
                            GetPixelPinY(),
                            GetRelativePinY());
}
void BasicElementImpl::SetPinY(const Variant &pin_y) {
  SET_PIXEL_OR_RELATIVE(pin_y, SetPixelPinY, SetRelativePinY);
}

} // namespace internal

static const char *kCursorTypeNames[] = {
  "arrow", "ibeam", "wait", "cross", "uparrow",
  "size", "sizenwse", "sizenesw", "sizewe", "sizens", "sizeall",
  "no", "hand", "busy", "help",
};

static const char *kHitTestNames[] = {
  "httransparent", "htnowhere", "htclient", "htcaption", " htsysmenu",
  "htsize", "htmenu", "hthscroll", "htvscroll", "htminbutton", "htmaxbutton",
  "htleft", "htright", "httop", "httopleft", "httopright",
  "htbottom", "htbottomleft", "htbottomright", "htborder",
  "htobject", "htclose", "hthelp",
};

BasicElement::BasicElement(ElementInterface *parent,
                           ViewInterface *view,
                           const char *name)
    : impl_(new internal::BasicElementImpl(parent, view, name, this)) {

  RegisterConstant("children", GetChildren());
  RegisterStringEnumProperty("cursor",
                             NewSlot(this, &BasicElement::GetCursor),
                             NewSlot(this, &BasicElement::SetCursor),
                             kCursorTypeNames, arraysize(kCursorTypeNames));
  RegisterProperty("dropTarget",
                   NewSlot(this, &BasicElement::IsDropTarget),
                   NewSlot(this, &BasicElement::SetDropTarget));
  RegisterProperty("enabled",
                   NewSlot(this, &BasicElement::IsEnabled),
                   NewSlot(this, &BasicElement::SetEnabled));
  RegisterProperty("height",
                   NewSlot(impl_, &internal::BasicElementImpl::GetHeight),
                   NewSlot(impl_, &internal::BasicElementImpl::SetHeight));
  RegisterStringEnumProperty("hitTest",
                             NewSlot(this, &BasicElement::GetHitTest),
                             NewSlot(this, &BasicElement::SetHitTest),
                             kHitTestNames, arraysize(kHitTestNames));
  RegisterProperty("mask",
                   NewSlot(this, &BasicElement::GetMask),
                   NewSlot(this, &BasicElement::SetMask));
  RegisterProperty("name", NewSlot(this, &BasicElement::GetName), NULL);
  RegisterProperty("offsetHeight",
                   NewSlot(this, &BasicElement::GetPixelHeight), NULL);
  RegisterProperty("offsetWidth",
                   NewSlot(this, &BasicElement::GetPixelWidth), NULL);
  RegisterProperty("offsetX",
                   NewSlot(this, &BasicElement::GetPixelX), NULL);
  RegisterProperty("offsetY",
                   NewSlot(this, &BasicElement::GetPixelY), NULL);
  RegisterProperty("parentElement",
                   NewSlot(impl_,
                           &internal::BasicElementImpl::GetParentElement),
                   NULL);
  RegisterProperty("pinX",
                   NewSlot(impl_, &internal::BasicElementImpl::GetPinX),
                   NewSlot(impl_, &internal::BasicElementImpl::SetPinX));
  RegisterProperty("pinY",
                   NewSlot(impl_, &internal::BasicElementImpl::GetPinY),
                   NewSlot(impl_, &internal::BasicElementImpl::SetPinY));
  RegisterProperty("rotation",
                   NewSlot(this, &BasicElement::GetRotation),
                   NewSlot(this, &BasicElement::SetRotation));
  RegisterProperty("tooltip",
                   NewSlot(this, &BasicElement::GetTooltip),
                   NewSlot(this, &BasicElement::SetTooltip));
  RegisterProperty("width",
                   NewSlot(impl_, &internal::BasicElementImpl::GetWidth),
                   NewSlot(impl_, &internal::BasicElementImpl::SetWidth));
  RegisterProperty("visible",
                   NewSlot(this, &BasicElement::IsVisible),
                   NewSlot(this, &BasicElement::SetVisible));
  RegisterProperty("x",
                   NewSlot(impl_, &internal::BasicElementImpl::GetX),
                   NewSlot(impl_, &internal::BasicElementImpl::SetX));
  RegisterProperty("y",
                   NewSlot(impl_, &internal::BasicElementImpl::GetY),
                   NewSlot(impl_, &internal::BasicElementImpl::SetY));

  RegisterMethod("focus", NewSlot(this, &BasicElement::Focus));
  RegisterMethod("killFocus", NewSlot(this, &BasicElement::KillFocus));
  RegisterMethod("appendElement",
                 NewSlot(GetChildren(), &Elements::AppendElementFromXML));
  RegisterMethod("insertElement",
                 NewSlot(GetChildren(), &Elements::InsertElementFromXML));
  RegisterMethod("removeElement",
                 NewSlot(GetChildren(), &Elements::RemoveElement));
  RegisterMethod("removeAllElements",
                 NewSlot(GetChildren(), &Elements::RemoveAllElements));

  RegisterSignal("onclick", &impl_->onclick_event_);
  RegisterSignal("ondblclick", &impl_->ondblclick_event_);
  RegisterSignal("ondragdrop", &impl_->ondragdrop_event_);
  RegisterSignal("ondragout", &impl_->ondragout_event_);
  RegisterSignal("ondragover", &impl_->ondragover_event_);
  RegisterSignal("onfocusin", &impl_->onfocusin_event_);
  RegisterSignal("onfocusout", &impl_->onfocusout_event_);
  RegisterSignal("onkeydown", &impl_->onkeydown_event_);
  RegisterSignal("onkeypress", &impl_->onkeypress_event_);
  RegisterSignal("onkeyup", &impl_->onkeyup_event_);
  RegisterSignal("onmousedown", &impl_->onmousedown_event_);
  RegisterSignal("onmousemove", &impl_->onmousemove_event_);
  RegisterSignal("onmouseout", &impl_->onmouseout_event_);
  RegisterSignal("onmouseover", &impl_->onmouseover_event_);
  RegisterSignal("onmouseup", &impl_->onmouseup_event_);
  RegisterSignal("onmousewheel", &impl_->onmousewheel_event_);
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

const Elements *BasicElement::GetChildren() const {
  ASSERT(impl_);
  return impl_->GetChildren();
}

Elements *BasicElement::GetChildren() {
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

const char *BasicElement::GetTooltip() const {
  ASSERT(impl_);
  return impl_->GetTooltip();
}

void BasicElement::SetTooltip(const char *tooltip) {
  ASSERT(impl_);
  return impl_->SetTooltip(tooltip);
}

ElementInterface *BasicElement::AppendElement(const char *tag_name,
                                              const char *name) {
  ASSERT(impl_);
  return impl_->children_.AppendElement(tag_name, name);
}

ElementInterface *BasicElement::InsertElement(const char *tag_name,
                                              const ElementInterface *before,
                                              const char *name) {
  ASSERT(impl_);
  return impl_->children_.InsertElement(tag_name, before, name);
}

bool BasicElement::RemoveElement(ElementInterface *child) {
  ASSERT(impl_);
  return impl_->children_.RemoveElement(child);
}

void BasicElement::RemoveAllElements() {
  ASSERT(impl_);
  impl_->children_.RemoveAllElements();
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

} // namespace ggadget
