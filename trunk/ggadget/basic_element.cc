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
#include "common.h"
#include "math_utils.h"
#include "element_factory_interface.h"
#include "element_interface.h"
#include "elements.h"
#include "graphics_interface.h"
#include "view_interface.h"

namespace ggadget {

class BasicElement::Impl {
 public:
  Impl(ElementInterface *parent,
       ViewInterface *view,
       const char *name,
       BasicElement *owner)
      : parent_(parent),
        children_(view->GetElementFactory(), owner, view),
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
        y_relative_(0.0),
        canvas_(NULL),
        mask_canvas_(NULL),
        visibility_changed_(true),
        changed_(true),
        position_changed_(true) {
    if (name)
      name_ = name;
    if (parent)
      ASSERT(parent->GetView() == view);
  }

  ~Impl() {
    if (canvas_) {
      canvas_->Destroy();
      canvas_ = NULL;
    }

    if (mask_canvas_) {
      mask_canvas_->Destroy();
      mask_canvas_ = NULL;  
    }  
  }

  void SetMask(const char *mask) {
    bool changed = false;
    if (mask && mask[0]) {
      changed = mask_ != mask;
      mask_ = mask;
    } else if (!mask_.empty()) {
      changed = true;
      mask_.clear();
    }

    if (changed) {
      if (mask_canvas_) {
        mask_canvas_->Destroy();
        mask_canvas_ = NULL;
      }
      view_->QueueDraw();
    }
  }

  const CanvasInterface *GetMaskCanvas() {
    // TODO: 
    return NULL;
  }

  void SetPixelWidth(double width) {
    if (width >= 0.0 && (width != width_ || width_relative_)) {
      width_ = width;
      width_relative_ = false;
      double p = GetParentWidth();
      if (p > 0.0)
        pwidth_ = width_ / p;
      WidthChanged();
    }
  }

  void SetPixelHeight(double height) {
    if (height >= 0.0 && (height != height_ || height_relative_)) {
      height_ = height;
      height_relative_ = false;
      double p = GetParentHeight();
      if (p > 0.0)
        pheight_ = height_ / p;
      HeightChanged();
    }
  }

  void SetRelativeWidth(double width, bool force) {
    if (width >= 0.0 && (force || width != pwidth_ || !width_relative_)) {
      pwidth_ = width;
      width_ = width * GetParentWidth();
      width_relative_ = true;
      WidthChanged();
    }
  }

  void SetRelativeHeight(double height, bool force) {
    if (height >= 0.0 && (force || height != pheight_ || !height_relative_)) {
      pheight_ = height;
      height_ = height * GetParentHeight();
      height_relative_ = true;
      HeightChanged();
    }
  }

  void SetPixelX(double x) {
    if (x != x_ || x_relative_) {
      x_ = x;
      double p = GetParentWidth();
      px_ = p > 0.0 ? x_ / p : 0.0;
      x_relative_ = false;
      view_->QueueDraw();
    }
  }

  void SetPixelY(double y) {
    if (y != y_ || y_relative_) {
      y_ = y;
      double p = GetParentHeight();
      py_ = p > 0.0 ? y_ / p : 0.0;
      y_relative_ = false;
      view_->QueueDraw();
    }
  }

  void SetRelativeX(double x, bool force) {
    if (force || x != px_ || !x_relative_) {
      px_ = x;
      x_ = x * GetParentWidth();
      x_relative_ = true;
      view_->QueueDraw();
    }
  }

  void SetRelativeY(double y, bool force) {
    if (force || y != py_ || !y_relative_) {
      py_ = y;
      y_ = y * GetParentHeight();
      y_relative_ = true;
      view_->QueueDraw();
    }
  }

  void SetPixelPinX(double pin_x) {
    if (pin_x != pin_x_ || pin_x_relative_) {
      pin_x_ = pin_x;
      ppin_x_ = width_ > 0.0 ? pin_x / width_ : 0.0;
      pin_x_relative_ = false;
      view_->QueueDraw();
    }
  }

  void SetPixelPinY(double pin_y) {
    if (pin_y != pin_y_ || pin_y_relative_) {
      pin_y_ = pin_y;
      ppin_y_ = height_ > 0.0 ? pin_y / height_ : 0.0;
      pin_y_relative_ = false;
      view_->QueueDraw();
    }
  }

  void SetRelativePinX(double pin_x, bool force) {
    if (force || pin_x != ppin_x_ || !pin_x_relative_) {
      ppin_x_ = pin_x;
      pin_x_ = pin_x * width_;
      pin_x_relative_ = true;
      view_->QueueDraw();
    }
  }

  void SetRelativePinY(double pin_y, bool force) {
    if (force || pin_y != ppin_y_ || !pin_y_relative_) {
      ppin_y_ = pin_y;
      pin_y_ = pin_y * height_;
      pin_y_relative_ = true;
      view_->QueueDraw();
    }
  }

  void SetRotation(double rotation) {
    if (rotation != rotation_) {
      rotation_ = rotation;
      position_changed_ = true;
      view_->QueueDraw();
    }
  }

  void SetOpacity(double opacity) {
    if (opacity >= 0.0 && opacity <= 1.0 && opacity != opacity_) {
      opacity_ = opacity;
      view_->QueueDraw();
    }
  }

  void SetVisible(bool visible) {
    if (visible != visible_) {
      visible_ = visible;
      visibility_changed_ = true;
      view_->QueueDraw();
    }
  }

  double GetParentWidth() const {
    return parent_ ? parent_->GetPixelWidth() : view_->GetWidth();
  }

  double GetParentHeight() const {
    return parent_ ? parent_->GetPixelHeight() : view_->GetHeight();
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

  Variant GetWidth() const {
    return GetPixelOrRelative(width_relative_, width_, pwidth_);
  }

  void SetWidth(const Variant &width) {
    double v;
    switch (ParsePixelOrRelative(width, &v)) {
      case 0: SetPixelWidth(v); break;
      case 1: SetRelativeWidth(v, false); break;
      default: break;
    }
  }

  Variant GetHeight() const {
    return GetPixelOrRelative(height_relative_, height_, pheight_);
  }

  void SetHeight(const Variant &height) {
    double v;
    switch (ParsePixelOrRelative(height, &v)) {
      case 0: SetPixelHeight(v); break;
      case 1: SetRelativeHeight(v, false); break;
      default: break;
    }
  }

  Variant GetX() const {
    return GetPixelOrRelative(x_relative_, x_, px_);
  }

  void SetX(const Variant &x) {
    double v;
    switch (ParsePixelOrRelative(x, &v)) {
      case 0: SetPixelX(v); break;
      case 1: SetRelativeX(v, false); break;
      default: break;
    }
  }

  Variant GetY() const {
    return GetPixelOrRelative(y_relative_, y_, py_);
  }

  void SetY(const Variant &y) {
    double v;
    switch (ParsePixelOrRelative(y, &v)) {
      case 0: SetPixelY(v); break;
      case 1: SetRelativeY(v, false); break;
      default: break;
    }
  }

  Variant GetPinX() const {
    return GetPixelOrRelative(pin_x_relative_, pin_x_, ppin_x_);
  }

  void SetPinX(const Variant &pin_x) {
    double v;
    switch (ParsePixelOrRelative(pin_x, &v)) {
      case 0: SetPixelPinX(v); break;
      case 1: SetRelativePinX(v, false); break;
      default: break;
    }
  }

  Variant GetPinY() const {
    return GetPixelOrRelative(pin_y_relative_, pin_y_, ppin_y_);
  }

  void SetPinY(const Variant &pin_y) {
    double v;
    switch (ParsePixelOrRelative(pin_y, &v)) {
      case 0: SetPixelPinY(v); break;
      case 1: SetRelativePinY(v, false); break;
      default: break;
    }
  }

  void HostChanged() {
    children_.HostChanged();
  }

  const CanvasInterface *Draw(bool *changed) {
    CanvasInterface *canvas = NULL;
    const CanvasInterface *children_canvas = NULL;
    bool child_changed;
    bool change = visibility_changed_;

    visibility_changed_ = false;
    if (!visible_) { // if not visible, then return no matter what
      goto exit;
    }

    children_canvas = children_.Draw(&child_changed);
    if (child_changed) {
      change = true;
    }
    if (!children_canvas) { 
      // The default basic element has no self to draw, so just
      // exit if no children to draw.
      goto exit;
    }

    if (!canvas_ || changed_ || change) {
      // Need to redraw
      change = true;    
      SetUpCanvas();
      
      canvas_->MultiplyOpacity(opacity_);
      
      // PLACEHOLDER
      // This is where another element would draw itself.
      
      canvas_->DrawCanvas(0., 0., children_canvas);
    }

    canvas = canvas_;

  exit:  
    *changed = change;
    return canvas;
  }
    
  void SetUpCanvas() {
    if (!canvas_) {
      const GraphicsInterface *gfx = view_->GetGraphics();
      ASSERT(gfx);
      canvas_ = gfx->NewCanvas(static_cast<size_t>(width_) + 1, 
                               static_cast<size_t>(height_) + 1);
      if (!canvas_) {
        DLOG("Error: unable to create canvas.");
      }
    }
    else {
      // If not new canvas, we must remember to clear canvas before drawing.
      canvas_->ClearCanvas();
    }
    canvas_->IntersectRectClipRegion(0., 0., width_, height_);
  }
  
 public:
  void WidthChanged() {
    if (pin_x_relative_)
      SetRelativePinX(ppin_x_, true);
    children_.OnParentWidthChange(width_);
    if (canvas_) { // Changes to width and height require a new canvas.
      canvas_->Destroy();
      canvas_ = NULL;
    }
    view_->QueueDraw();
  }

  void HeightChanged() {
    if (pin_y_relative_)
      SetRelativePinY(ppin_y_, true);
    children_.OnParentHeightChange(height_);
    if (canvas_) { // Changes to width and height require a new canvas.
      canvas_->Destroy();
      canvas_ = NULL;
    }
    view_->QueueDraw();
  }

 public:
  ElementInterface *parent_;
  Elements children_;
  ViewInterface *view_;
  ElementInterface::HitTest hittest_;
  ElementInterface::CursorType cursor_;
  bool drop_target_;
  bool enabled_;
  std::string name_;
  double pin_x_;
  double pin_y_;
  double ppin_x_;
  double ppin_y_;
  bool pin_x_relative_;
  bool pin_y_relative_;
  double rotation_;
  double opacity_;
  bool visible_;
  std::string tooltip_;
  std::string mask_;
  double width_;
  double height_;
  double x_;
  double y_;
  double pwidth_;
  double pheight_;
  double px_;
  double py_;
  double width_relative_;
  double height_relative_;
  double x_relative_;
  double y_relative_;

  CanvasInterface *canvas_;
  CanvasInterface *mask_canvas_;
  bool visibility_changed_;
  bool changed_;
  bool position_changed_;

  EventSignal onclick_event_;
  EventSignal ondblclick_event_;
  EventSignal ondragdrop_event_;
  EventSignal ondragout_event_;
  EventSignal ondragover_event_;
  EventSignal onfocusin_event_;
  EventSignal onfocusout_event_;
  EventSignal onkeydown_event_;
  EventSignal onkeypress_event_;
  EventSignal onkeyup_event_;
  EventSignal onmousedown_event_;
  EventSignal onmousemove_event_;
  EventSignal onmouseout_event_;
  EventSignal onmouseover_event_;
  EventSignal onmouseup_event_;
  EventSignal onmousewheel_event_;
};

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
                           const char *name,
                           bool is_container)
    : impl_(new Impl(parent, view, name, this)) {
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
                   NewSlot(impl_, &Impl::GetHeight),
                   NewSlot(impl_, &Impl::SetHeight));
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
  RegisterConstant("parentElement", parent);
  RegisterProperty("pinX",
                   NewSlot(impl_, &Impl::GetPinX),
                   NewSlot(impl_, &Impl::SetPinX));
  RegisterProperty("pinY",
                   NewSlot(impl_, &Impl::GetPinY),
                   NewSlot(impl_, &Impl::SetPinY));
  RegisterProperty("rotation",
                   NewSlot(this, &BasicElement::GetRotation),
                   NewSlot(this, &BasicElement::SetRotation));
  RegisterProperty("tooltip",
                   NewSlot(this, &BasicElement::GetTooltip),
                   NewSlot(this, &BasicElement::SetTooltip));
  RegisterProperty("width",
                   NewSlot(impl_, &Impl::GetWidth),
                   NewSlot(impl_, &Impl::SetWidth));
  RegisterProperty("visible",
                   NewSlot(this, &BasicElement::IsVisible),
                   NewSlot(this, &BasicElement::SetVisible));
  RegisterProperty("x",
                   NewSlot(impl_, &Impl::GetX),
                   NewSlot(impl_, &Impl::SetX));
  RegisterProperty("y",
                   NewSlot(impl_, &Impl::GetY),
                   NewSlot(impl_, &Impl::SetY));

  RegisterMethod("focus", NewSlot(this, &BasicElement::Focus));
  RegisterMethod("killFocus", NewSlot(this, &BasicElement::KillFocus));

  if (is_container) {
    Elements *children = &impl_->children_;
    RegisterConstant("children", children);
    RegisterMethod("appendElement",
                   NewSlot(children, &Elements::AppendElementFromXML));
    RegisterMethod("insertElement",
                   NewSlot(children, &Elements::InsertElementFromXML));
    RegisterMethod("removeElement",
                   NewSlot(children, &Elements::RemoveElement));
    RegisterMethod("removeAllElements",
                   NewSlot(children, &Elements::RemoveAllElements));
  }

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
  return impl_->view_;
}

const ViewInterface *BasicElement::GetView() const {
  return impl_->view_;
}

ElementInterface::HitTest BasicElement::GetHitTest() const {
  return impl_->hittest_;
}

void BasicElement::SetHitTest(HitTest value) {
  impl_->hittest_ = value;
}

const Elements *BasicElement::GetChildren() const {
  return &impl_->children_;
}

Elements *BasicElement::GetChildren() {
  return &impl_->children_;
}

ElementInterface::CursorType BasicElement::GetCursor() const {
  return impl_->cursor_;
}

void BasicElement::SetCursor(ElementInterface::CursorType cursor) {
  impl_->cursor_ = cursor;
}

bool BasicElement::IsDropTarget() const {
  return impl_->drop_target_;
}

void BasicElement::SetDropTarget(bool drop_target) {
  impl_->drop_target_ = drop_target;
}

bool BasicElement::IsEnabled() const {
  return impl_->enabled_;
}

void BasicElement::SetEnabled(bool enabled) {
  impl_->enabled_ = enabled;
}

const char *BasicElement::GetName() const {
  return impl_->name_.c_str();
}

const char *BasicElement::GetMask() const {
  return impl_->mask_.c_str();
}

void BasicElement::SetMask(const char *mask) {
  impl_->SetMask(mask);
}

const CanvasInterface *BasicElement::GetMaskCanvas() {
  return impl_->GetMaskCanvas();
}

double BasicElement::GetPixelWidth() const {
  return impl_->width_;
}

void BasicElement::SetPixelWidth(double width) {
  impl_->SetPixelWidth(width);
}

double BasicElement::GetPixelHeight() const {
  return impl_->height_;
}

void BasicElement::SetPixelHeight(double height) {
  return impl_->SetPixelHeight(height);
}

double BasicElement::GetRelativeWidth() const {
  return impl_->pwidth_;
}

double BasicElement::GetRelativeHeight() const {
  return impl_->pheight_;
}

double BasicElement::GetPixelX() const {
  return impl_->x_;
}

void BasicElement::SetPixelX(double x) {
  impl_->SetPixelX(x);
}

double BasicElement::GetPixelY() const {
  return impl_->y_;
}

void BasicElement::SetPixelY(double y) {
  impl_->SetPixelY(y);
}

double BasicElement::GetRelativeX() const {
  return impl_->px_;
}

double BasicElement::GetRelativeY() const {
  return impl_->py_;
}

double BasicElement::GetPixelPinX() const {
  return impl_->pin_x_;
}

void BasicElement::SetPixelPinX(double pin_x) {
  impl_->SetPixelPinX(pin_x);
}

double BasicElement::GetPixelPinY() const {
  return impl_->pin_y_;
}

void BasicElement::SetPixelPinY(double pin_y) {
  impl_->SetPixelPinY(pin_y);
}

void BasicElement::SetRelativeWidth(double width) {
  impl_->SetRelativeWidth(width, false);
}

void BasicElement::SetRelativeHeight(double height) {
  impl_->SetRelativeHeight(height, false);
}

void BasicElement::SetRelativeX(double x) {
  impl_->SetRelativeX(x, false);
}

void BasicElement::SetRelativeY(double y) {
  impl_->SetRelativeY(y, false);
}

double BasicElement::GetRelativePinX() const {
  return impl_->ppin_x_;
}

void BasicElement::SetRelativePinX(double x) {
  impl_->SetRelativePinX(x, false);
}

double BasicElement::GetRelativePinY() const {
  return impl_->ppin_y_;
}

void BasicElement::SetRelativePinY(double y) {
  return impl_->SetRelativePinY(y, false);
}

bool BasicElement::XIsRelative() const {
  return impl_->x_relative_;
}

bool BasicElement::YIsRelative() const {
  return impl_->y_relative_;
}

bool BasicElement::WidthIsRelative() const {
  return impl_->width_relative_;
}

bool BasicElement::HeightIsRelative() const {
  return impl_->height_relative_;
}

bool BasicElement::PinXIsRelative() const {
  return impl_->pin_x_relative_;
}

bool BasicElement::PinYIsRelative() const {
  return impl_->pin_y_relative_;
}

double BasicElement::GetRotation() const {
  return impl_->rotation_;
}

void BasicElement::SetRotation(double rotation) {
  impl_->SetRotation(rotation);
}

double BasicElement::GetOpacity() const {
  return impl_->opacity_;
}

void BasicElement::SetOpacity(double opacity) {
  impl_->SetOpacity(opacity);
}

bool BasicElement::IsVisible() const {
  return impl_->visible_;
}

void BasicElement::SetVisible(bool visible) {
  impl_->SetVisible(visible);
}

ElementInterface *BasicElement::GetParentElement() {
  return impl_->parent_;
}

const ElementInterface *BasicElement::GetParentElement() const {
  return impl_->parent_;
}

const char *BasicElement::GetTooltip() const {
  return impl_->tooltip_.c_str();
}

void BasicElement::SetTooltip(const char *tooltip) {
  if (tooltip)
    impl_->tooltip_ = tooltip;
  else
    impl_->tooltip_.clear();
}

void BasicElement::Focus() {
}

void BasicElement::KillFocus() {
}

const CanvasInterface *BasicElement::Draw(bool *changed) {
  return impl_->Draw(changed);  
}

void BasicElement::ClearVisibilityChanged() {
  impl_->visibility_changed_ = false;
}

bool BasicElement::IsVisibilityChanged() const { 
  return impl_->visibility_changed_;
}

void BasicElement::SetSelfChanged(bool changed) { 
  impl_->changed_ = changed;
}

bool BasicElement::IsSelfChanged() const {
  return impl_->changed_;
}

void BasicElement::ClearPositionChanged() {
  impl_->position_changed_ = false;
}

bool BasicElement::IsPositionChanged() const { 
  return impl_->position_changed_;
}

void BasicElement::SetUpCanvas() {
  impl_->SetUpCanvas();
}

CanvasInterface *BasicElement::GetCanvas() {
  return impl_->canvas_;
}

void BasicElement::DrawBoundingBox() {
  impl_->canvas_->DrawLine(0, 0, 0, impl_->height_, 1, Color(0, 0, 0));
  impl_->canvas_->DrawLine(0, 0, impl_->width_, 0, 1, Color(0, 0, 0));
  impl_->canvas_->DrawLine(impl_->width_, impl_->height_, 
                           0, impl_->height_, 1, Color(0, 0, 0));
  impl_->canvas_->DrawLine(impl_->width_, impl_->height_, 
                           impl_->width_, 0, 1, Color(0, 0, 0));
  impl_->canvas_->DrawLine(0, 0, impl_->width_, impl_->height_, 
                           1, Color(0, 0, 0));
  impl_->canvas_->DrawLine(impl_->width_, 0, 0, impl_->height_, 
                           1, Color(0, 0, 0));  
}

void BasicElement::OnParentWidthChange(double width) {
  if (impl_->x_relative_)
    impl_->SetRelativeX(impl_->px_, true);
  if (impl_->width_relative_)
    impl_->SetRelativeWidth(impl_->pwidth_, true);
}

void BasicElement::OnParentHeightChange(double height) {
  if (impl_->y_relative_)
    impl_->SetRelativeY(impl_->py_, true);
  if (impl_->height_relative_)
    impl_->SetRelativeHeight(impl_->pheight_, true);
}

} // namespace ggadget
