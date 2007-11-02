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
#include <iostream>
#include "basic_element.h"
#include "common.h"
#include "math_utils.h"
#include "element_factory_interface.h"
#include "element_interface.h"
#include "elements.h"
#include "graphics_interface.h"
#include "image.h"
#include "math_utils.h"
#include "scriptable_event.h"
#include "string_utils.h"
#include "view_interface.h"
#include "event.h"

namespace ggadget {

class BasicElement::Impl {
 public:
  Impl(ElementInterface *parent,
       ViewInterface *view,
       const char *tag_name,
       const char *name,
       BasicElement *owner)
      : parent_(parent),
        owner_(owner),
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
        mask_image_(NULL),
        visibility_changed_(true),
        changed_(true),
        position_changed_(true),
        debug_color_index_(++total_debug_color_index_),
        debug_mode_(view_->GetDebugMode()) {
    if (name)
      name_ = name;
    if (tag_name)
      tag_name_ = tag_name;
    if (parent)
      ASSERT(parent->GetView() == view);
  }

  ~Impl() {
    if (canvas_) {
      canvas_->Destroy();
      canvas_ = NULL;
    }

    delete mask_image_;
    mask_image_ = NULL;
  }

  void SetMask(const char *mask) {
    if (AssignIfDiffer(mask, &mask_)) {
      delete mask_image_;
      mask_image_ = view_->LoadImage(mask, true);
      view_->QueueDraw();
    }
  }

  const CanvasInterface *GetMaskCanvas() {
    return mask_image_ ? mask_image_->GetCanvas() : NULL;
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
      position_changed_ = true;
      view_->QueueDraw();
    }
  }

  void SetPixelY(double y) {
    if (y != y_ || y_relative_) {
      y_ = y;
      double p = GetParentHeight();
      py_ = p > 0.0 ? y_ / p : 0.0;
      y_relative_ = false;
      position_changed_ = true;
      view_->QueueDraw();
    }
  }

  void SetRelativeX(double x, bool force) {
    if (force || x != px_ || !x_relative_) {
      px_ = x;
      x_ = x * GetParentWidth();
      x_relative_ = true;
      position_changed_ = true;
      view_->QueueDraw();
    }
  }

  void SetRelativeY(double y, bool force) {
    if (force || y != py_ || !y_relative_) {
      py_ = y;
      y_ = y * GetParentHeight();
      y_relative_ = true;
      position_changed_ = true;
      view_->QueueDraw();
    }
  }

  void SetPixelPinX(double pin_x) {
    if (pin_x != pin_x_ || pin_x_relative_) {
      pin_x_ = pin_x;
      ppin_x_ = width_ > 0.0 ? pin_x / width_ : 0.0;
      pin_x_relative_ = false;
      position_changed_ = true;
      view_->QueueDraw();
    }
  }

  void SetPixelPinY(double pin_y) {
    if (pin_y != pin_y_ || pin_y_relative_) {
      pin_y_ = pin_y;
      ppin_y_ = height_ > 0.0 ? pin_y / height_ : 0.0;
      pin_y_relative_ = false;
      position_changed_ = true;
      view_->QueueDraw();
    }
  }

  void SetRelativePinX(double pin_x, bool force) {
    if (force || pin_x != ppin_x_ || !pin_x_relative_) {
      ppin_x_ = pin_x;
      pin_x_ = pin_x * width_;
      pin_x_relative_ = true;
      position_changed_ = true;
      view_->QueueDraw();
    }
  }

  void SetRelativePinY(double pin_y, bool force) {
    if (force || pin_y != ppin_y_ || !pin_y_relative_) {
      ppin_y_ = pin_y;
      pin_y_ = pin_y * height_;
      pin_y_relative_ = true;
      position_changed_ = true;
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
    if (opacity != opacity_) {
      opacity_ = opacity;
      changed_ = true;
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

  // Retrieves the opacity of the element.
  int GetIntOpacity() const {
    return static_cast<int>(round(opacity_ * 255));
  }

  // Sets the opacity of the element.
  void SetIntOpacity(int opacity) {
    if (0 <= opacity && 255 >= opacity) {
      SetOpacity(opacity / 255.);
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
      // The input is an integer pixel value.
      case Variant::TYPE_INT64:
        *output = VariantValue<int>()(input);
        return 0;
      // The input is a double pixel value.
      case Variant::TYPE_DOUBLE:
        *output = round(VariantValue<double>()(input));
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
      return Variant(static_cast<int>(round(pixel)));
    }
  }

  Variant GetWidth() const {
    return GetPixelOrRelative(width_relative_, width_, pwidth_);
  }

  void SetWidth(const Variant &width) {
    double v;
    switch (ParsePixelOrRelative(width, &v)) {
      case 0: owner_->SetPixelWidth(v); break;
      case 1: owner_->SetRelativeWidth(v); break;
      default: break;
    }
  }

  Variant GetHeight() const {
    return GetPixelOrRelative(height_relative_, height_, pheight_);
  }

  void SetHeight(const Variant &height) {
    double v;
    switch (ParsePixelOrRelative(height, &v)) {
      case 0: owner_->SetPixelHeight(v); break;
      case 1: owner_->SetRelativeHeight(v); break;
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

  const CanvasInterface *Draw(bool *changed) {
    const CanvasInterface *canvas = NULL;
    const CanvasInterface *children_canvas = NULL;
    bool child_changed;
    bool change = visibility_changed_;

    visibility_changed_ = false;
    if (!visible_) { // if not visible, then return no matter what
      goto exit;
    }

    children_canvas = children_.Draw(&child_changed);
    change = change || child_changed | changed_ || !canvas_;
    changed_ = false;

    if (change) {
      // Need to redraw.
      SetUpCanvas();
      canvas_->MultiplyOpacity(opacity_);
      owner_->DoDraw(canvas_, children_canvas);

      if (debug_mode_ == 1) {
        // TODO: draw box around children_canvas only.
      } else if (debug_mode_ == 2) {
        DrawBoundingBox(canvas_, width_, height_, debug_color_index_);
      }
    }

    canvas = canvas_;

  exit:
    *changed = change;
    return canvas;
  }

  static void DrawBoundingBox(CanvasInterface *canvas,
                              double w, double h,
                              int color_index) {
    Color color(((color_index >> 4) & 3) / 3.5,
                ((color_index >> 2) & 3) / 3.5,
                (color_index & 3) / 3.5);
    canvas->DrawLine(0, 0, 0, h, 1, color);
    canvas->DrawLine(0, 0, w, 0, 1, color);
    canvas->DrawLine(w, h, 0, h, 1, color);
    canvas->DrawLine(w, h, w, 0, 1, color);
    canvas->DrawLine(0, 0, w, h, 1, color);
    canvas->DrawLine(w, 0, 0, h, 1, color);
  }

  CanvasInterface *SetUpCanvas() {
    if (!canvas_) {
      const GraphicsInterface *gfx = view_->GetGraphics();
      ASSERT(gfx);
      canvas_ = gfx->NewCanvas(static_cast<size_t>(ceil(width_)),
                               static_cast<size_t>(ceil(height_)));
      if (!canvas_) {
        DLOG("Error: unable to create canvas.");
      }
    }
    else {
      // If not new canvas, we must remember to clear canvas before drawing.
      canvas_->ClearCanvas();
    }
    canvas_->IntersectRectClipRegion(0., 0., width_, height_);
    return canvas_;
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
  bool OnMouseEvent(MouseEvent *event, bool direct,
                    ElementInterface **fired_element) {
    *fired_element = NULL;
    if (!direct) {
      // Send to the children first.
      bool result = children_.OnMouseEvent(event, fired_element);
      if (*fired_element)
        return result;
    }

    if (!enabled_)
      return true;

    // Don't check mouse position, because the event may be out of this
    // element when this element is grabbing mouse.

    // Take this event, since no children took it, and we're enabled.
    ScriptableEvent scriptable_event(event, owner_, 0, 0);
    if (event->GetType() != Event::EVENT_MOUSE_MOVE)
      DLOG("%s(%s|%s): %g %g %d %d", scriptable_event.GetName(),
           name_.c_str(), tag_name_.c_str(),
           event->GetX(), event->GetY(),
           event->GetButton(), event->GetWheelDelta());

    switch (event->GetType()) {
      case Event::EVENT_MOUSE_MOVE: // put the high volume events near top
        view_->FireEvent(&scriptable_event, onmousemove_event_);
        break;
      case Event::EVENT_MOUSE_DOWN:
        view_->FireEvent(&scriptable_event, onmousedown_event_);
        break;
      case Event::EVENT_MOUSE_UP:
        view_->FireEvent(&scriptable_event, onmouseup_event_);
        break;
      case Event::EVENT_MOUSE_CLICK:
        view_->FireEvent(&scriptable_event, onclick_event_);
        break;
      case Event::EVENT_MOUSE_DBLCLICK:
        view_->FireEvent(&scriptable_event, ondblclick_event_);
        break;
      case Event::EVENT_MOUSE_OUT:
        view_->FireEvent(&scriptable_event, onmouseout_event_);
        break;
      case Event::EVENT_MOUSE_OVER:
        view_->FireEvent(&scriptable_event, onmouseover_event_);
        break;
      case Event::EVENT_MOUSE_WHEEL:
        view_->FireEvent(&scriptable_event, onmousewheel_event_);
        break;
      default:
        ASSERT(false);
    }

    *fired_element = owner_;
    return scriptable_event.GetReturnValue();
  }

  bool OnKeyEvent(KeyboardEvent *event) {
    if (!enabled_)
      return true;

    ScriptableEvent scriptable_event(event, owner_, 0, 0);
    DLOG("%s(%s|%s): %d", scriptable_event.GetName(),
         name_.c_str(), tag_name_.c_str(), event->GetKeyCode());

    switch (event->GetType()) {
      case Event::EVENT_KEY_DOWN:
        view_->FireEvent(&scriptable_event, onkeydown_event_);
        break;
      case Event::EVENT_KEY_UP:
        view_->FireEvent(&scriptable_event, onkeyup_event_);
        break;
      case Event::EVENT_KEY_PRESS:
        view_->FireEvent(&scriptable_event, onkeypress_event_);
        break;
      default:
        ASSERT(false);
    }
    return scriptable_event.GetReturnValue();
  }

  bool OnOtherEvent(Event *event) {
    if (!enabled_)
      return true;

    ScriptableEvent scriptable_event(event, owner_, 0, 0);
    DLOG("%s(%s|%s)", scriptable_event.GetName(),
         name_.c_str(), tag_name_.c_str());

    // TODO: focus logic.
    switch (event->GetType()) {
      case Event::EVENT_FOCUS_IN:
        view_->FireEvent(&scriptable_event, onfocusin_event_);
        break;
      case Event::EVENT_FOCUS_OUT:
        view_->FireEvent(&scriptable_event, onfocusout_event_);
        break;
      default:
        ASSERT(false);
    }
    return scriptable_event.GetReturnValue();
  }

 public:
  ElementInterface *parent_;
  BasicElement *owner_;
  Elements children_;
  ViewInterface *view_;
  ElementInterface::HitTest hittest_;
  ElementInterface::CursorType cursor_;
  bool drop_target_;
  bool enabled_;
  std::string tag_name_;
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
  Image *mask_image_;
  bool visibility_changed_;
  bool changed_;
  bool position_changed_;

  int debug_color_index_;
  static int total_debug_color_index_;
  int debug_mode_;

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

int BasicElement::Impl::total_debug_color_index_ = 0;

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
                           const char *tag_name,
                           const char *name,
                           bool is_container)
    : impl_(new Impl(parent, view, tag_name, name, this)) {
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
  RegisterConstant("name", impl_->name_);
  RegisterProperty("offsetHeight",
                   NewSlot(this, &BasicElement::GetPixelHeight), NULL);
  RegisterProperty("offsetWidth",
                   NewSlot(this, &BasicElement::GetPixelWidth), NULL);
  RegisterProperty("offsetX",
                   NewSlot(this, &BasicElement::GetPixelX), NULL);
  RegisterProperty("offsetY",
                   NewSlot(this, &BasicElement::GetPixelY), NULL);
  RegisterProperty("opacity",
                   NewSlot(impl_, &Impl::GetIntOpacity),
                   NewSlot(impl_, &Impl::SetIntOpacity));
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
  RegisterConstant("tagname", impl_->tag_name_);
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

  RegisterSignal(kOnClickEvent, &impl_->onclick_event_);
  RegisterSignal(kOnDblClickEvent, &impl_->ondblclick_event_);
  RegisterSignal(kOnDragDropEvent, &impl_->ondragdrop_event_);
  RegisterSignal(kOnDragOutEvent, &impl_->ondragout_event_);
  RegisterSignal(kOnDragOverEvent, &impl_->ondragover_event_);
  RegisterSignal(kOnFocusInEvent, &impl_->onfocusin_event_);
  RegisterSignal(kOnFocusOutEvent, &impl_->onfocusout_event_);
  RegisterSignal(kOnKeyDownEvent, &impl_->onkeydown_event_);
  RegisterSignal(kOnKeyPressEvent, &impl_->onkeypress_event_);
  RegisterSignal(kOnKeyUpEvent, &impl_->onkeyup_event_);
  RegisterSignal(kOnMouseDownEvent, &impl_->onmousedown_event_);
  RegisterSignal(kOnMouseMoveEvent, &impl_->onmousemove_event_);
  RegisterSignal(kOnMouseOutEvent, &impl_->onmouseout_event_);
  RegisterSignal(kOnMouseOverEvent, &impl_->onmouseover_event_);
  RegisterSignal(kOnMouseUpEvent, &impl_->onmouseup_event_);
  RegisterSignal(kOnMouseWheelEvent, &impl_->onmousewheel_event_);
}

BasicElement::~BasicElement() {
  delete impl_;
}

void BasicElement::Destroy() {
  delete this;
}

const char *BasicElement::GetTagName() const {
  return impl_->tag_name_.c_str();
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
  if (opacity >= 0.0 && opacity <= 1.0) {
    impl_->SetOpacity(opacity);
  }
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

void BasicElement::ClearPositionChanged() {
  impl_->position_changed_ = false;
}

bool BasicElement::IsPositionChanged() const {
  return impl_->position_changed_;
}

#if 0 // TODO: ensure if they are needed.
void BasicElement::ClearVisibilityChanged() {
  impl_->visibility_changed_ = false;
}

bool BasicElement::IsVisibilityChanged() const {
  return impl_->visibility_changed_;
}

CanvasInterface *BasicElement::SetUpCanvas() {
  return impl_->SetUpCanvas();
}

CanvasInterface *BasicElement::GetCanvas() {
  return impl_->canvas_;
}

void BasicElement::SetSelfChanged(bool changed) { 
  impl_->changed_ = changed;
  impl_->view_->QueueDraw();
}

bool BasicElement::IsSelfChanged() const {
  return impl_->changed_;
}
#endif

void BasicElement::QueueDraw() {
  impl_->changed_ = true;
  if (impl_->visible_) {
    impl_->view_->QueueDraw();
  }
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

bool BasicElement::OnMouseEvent(MouseEvent *event, bool direct,
                                ElementInterface **fired_element) {
  return impl_->OnMouseEvent(event, direct, fired_element);
}

bool BasicElement::OnKeyEvent(KeyboardEvent *event) {
  return impl_->OnKeyEvent(event);
}

bool BasicElement::OnOtherEvent(Event *event) {
  return impl_->OnOtherEvent(event);
}

bool BasicElement::IsMouseEventIn(MouseEvent *event) {
  return IsPointInElement(event->GetX(), event->GetY(),
                          impl_->width_, impl_->height_);
}

void BasicElement::SelfCoordToChildCoord(ElementInterface *child,
                                         double x, double y,
                                         double *child_x, double *child_y) {
  ParentCoordToChildCoord(x, y, child->GetPixelX(), child->GetPixelY(),
                          child->GetPixelPinX(), child->GetPixelPinY(),
                          DegreesToRadians(child->GetRotation()),
                          child_x, child_y);
}

} // namespace ggadget
