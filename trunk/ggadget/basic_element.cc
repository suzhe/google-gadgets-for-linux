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
#include <vector>
#include "basic_element.h"
#include "common.h"
#include "logger.h"
#include "math_utils.h"
#include "element_factory.h"
#include "elements.h"
#include "graphics_interface.h"
#include "math_utils.h"
#include "menu_interface.h"
#include "scriptable_event.h"
#include "string_utils.h"
#include "view.h"
#include "event.h"

namespace ggadget {

class BasicElement::Impl {
 public:
  Impl(BasicElement *parent, View *view, const char *tag_name, const char *name,
       bool children, BasicElement *owner)
      : parent_(parent),
        owner_(owner),
        children_(children ?
                  new Elements(view->GetElementFactory(), owner, view) :
                  NULL),
        view_(view),
        hittest_(BasicElement::HT_DEFAULT),
        cursor_(ViewInterface::CURSOR_ARROW),
        drop_target_(false),
        enabled_(false),
        width_(0.0), height_(0.0), pwidth_(0.0), pheight_(0.0),
        width_relative_(false), height_relative_(false),
        width_specified_(false), height_specified_(false),
        x_(0.0), y_(0.0), px_(0.0), py_(0.0),
        x_relative_(false), y_relative_(false),
        x_specified_(false), y_specified_(false),
        pin_x_(0.0), pin_y_(0.0), ppin_x_(0.0), ppin_y_(0.0),
        pin_x_relative_(false), pin_y_relative_(false),
        rotation_(0.0),
        opacity_(1.0),
        visible_(true),
        flip_(FLIP_NONE),
        implicit_(false),
        mask_image_(NULL),
        visibility_changed_(true),
        position_changed_(true),
        size_changed_(true),
        debug_color_index_(++total_debug_color_index_),
        debug_mode_(view->GetDebugMode()) {
    memset(old_view_rectangle_, 0, sizeof(old_view_rectangle_));
    if (name)
      name_ = name;
    if (tag_name)
      tag_name_ = tag_name;
    if (parent)
      ASSERT(parent->GetView() == view);
  }

  ~Impl() {
    DestroyImage(mask_image_);
    delete children_;
  }

  void SetMask(const Variant &mask) {
    DestroyImage(mask_image_);
    mask_image_ = view_->LoadImage(mask, true);
    QueueDraw();
  }

  const CanvasInterface *GetMaskCanvas() {
    return mask_image_ ? mask_image_->GetCanvas() : NULL;
  }

  void SetPixelWidth(double width) {
    if (width >= 0.0 && (width != width_ || width_relative_)) {
      width_ = width;
      width_relative_ = false;
      WidthChanged();
    }
  }

  void SetPixelHeight(double height) {
    if (height >= 0.0 && (height != height_ || height_relative_)) {
      height_ = height;
      height_relative_ = false;
      HeightChanged();
    }
  }

  void SetRelativeWidth(double width) {
    if (width >= 0.0 && (width != pwidth_ || !width_relative_)) {
      pwidth_ = width;
      width_relative_ = true;
      WidthChanged();
    }
  }

  void SetRelativeHeight(double height) {
    if (height >= 0.0 && (height != pheight_ || !height_relative_)) {
      pheight_ = height;
      height_relative_ = true;
      HeightChanged();
    }
  }

  void SetPixelX(double x) {
    if (x != x_ || x_relative_) {
      x_ = x;
      x_relative_ = false;
      PositionChanged();
    }
  }

  void SetPixelY(double y) {
    if (y != y_ || y_relative_) {
      y_ = y;
      y_relative_ = false;
      PositionChanged();
    }
  }

  void SetRelativeX(double x) {
    if (x != px_ || !x_relative_) {
      px_ = x;
      x_relative_ = true;
      PositionChanged();
    }
  }

  void SetRelativeY(double y) {
    if (y != py_ || !y_relative_) {
      py_ = y;
      y_relative_ = true;
      PositionChanged();
    }
  }

  void SetPixelPinX(double pin_x) {
    if (pin_x != pin_x_ || pin_x_relative_) {
      pin_x_ = pin_x;
      pin_x_relative_ = false;
      PositionChanged();
    }
  }

  void SetPixelPinY(double pin_y) {
    if (pin_y != pin_y_ || pin_y_relative_) {
      pin_y_ = pin_y;
      pin_y_relative_ = false;
      PositionChanged();
    }
  }

  void SetRelativePinX(double pin_x) {
    if (pin_x != ppin_x_ || !pin_x_relative_) {
      ppin_x_ = pin_x;
      pin_x_relative_ = true;
      PositionChanged();
    }
  }

  void SetRelativePinY(double pin_y) {
    if (pin_y != ppin_y_ || !pin_y_relative_) {
      ppin_y_ = pin_y;
      pin_y_relative_ = true;
      PositionChanged();
    }
  }

  void SetRotation(double rotation) {
    if (rotation != rotation_) {
      rotation_ = rotation;
      PositionChanged();
    }
  }

  void SetOpacity(double opacity) {
    if (opacity != opacity_) {
      opacity_ = opacity;
      QueueDraw();
    }
  }

  void SetVisible(bool visible) {
    if (visible != visible_) {
      visible_ = visible;
      visibility_changed_ = true;
      QueueDraw();
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
    return parent_ ? parent_->GetClientWidth() : view_->GetWidth();
  }

  double GetParentHeight() const {
    return parent_ ? parent_->GetClientHeight() : view_->GetHeight();
  }

  Variant GetWidth() const {
    return BasicElement::GetPixelOrRelative(width_relative_, width_specified_,
                                            width_, pwidth_);
  }

  void SetWidth(const Variant &width) {
    double v;
    switch (ParsePixelOrRelative(width, &v)) {
      case BasicElement::PR_PIXEL:
        width_specified_ = true;
        SetPixelWidth(v);
        break;
      case BasicElement::PR_RELATIVE:
        width_specified_ = true;
        SetRelativeWidth(v);
        break;
      case BasicElement::PR_UNSPECIFIED:
        ResetWidthToDefault();
        break;
      default:
        break;
    }
  }

  void ResetWidthToDefault() {
    if (width_specified_) {
      width_relative_ = width_specified_ = false;
      WidthChanged();
    }
  }

  Variant GetHeight() const {
    return BasicElement::GetPixelOrRelative(height_relative_, height_specified_,
                                            height_, pheight_);
  }

  void SetHeight(const Variant &height) {
    double v;
    switch (ParsePixelOrRelative(height, &v)) {
      case BasicElement::PR_PIXEL:
        height_specified_ = true;
        SetPixelHeight(v);
        break;
      case BasicElement::PR_RELATIVE:
        height_specified_ = true;
        SetRelativeHeight(v);
        break;
      case BasicElement::PR_UNSPECIFIED:
        ResetHeightToDefault();
        break;
      default:
        break;
    }
  }

  void ResetHeightToDefault() {
    if (height_specified_) {
      height_relative_ = height_specified_ = false;
      HeightChanged();
    }
  }

  Variant GetX() const {
    return BasicElement::GetPixelOrRelative(x_relative_, x_specified_, x_, px_);
  }

  void SetX(const Variant &x) {
    double v;
    switch (ParsePixelOrRelative(x, &v)) {
      case BasicElement::PR_PIXEL:
        x_specified_ = true;
        SetPixelX(v);
        break;
      case BasicElement::PR_RELATIVE:
        x_specified_ = true;
        SetRelativeX(v);
        break;
      case BasicElement::PR_UNSPECIFIED:
        ResetXToDefault();
        break;
      default:
        break;
    }
  }

  void ResetXToDefault() {
    if (x_specified_) {
      x_relative_ = x_specified_ = false;
      PositionChanged();
    }
  }

  Variant GetY() const {
    return BasicElement::GetPixelOrRelative(y_relative_, y_specified_, y_, py_);
  }

  void SetY(const Variant &y) {
    double v;
    switch (ParsePixelOrRelative(y, &v)) {
      case BasicElement::PR_PIXEL:
        y_specified_ = true;
        SetPixelY(v);
        break;
      case BasicElement::PR_RELATIVE:
        y_specified_ = true;
        SetRelativeY(v);
        break;
      case BasicElement::PR_UNSPECIFIED:
        ResetYToDefault();
        break;
      default:
        break;
    }
  }

  void ResetYToDefault() {
    if (y_specified_) {
      y_relative_ = y_specified_ = false;
      PositionChanged();
    }
  }

  Variant GetPinX() const {
    return BasicElement::GetPixelOrRelative(pin_x_relative_, true,
                                            pin_x_, ppin_x_);
  }

  void SetPinX(const Variant &pin_x) {
    double v;
    switch (ParsePixelOrRelative(pin_x, &v)) {
      case BasicElement::PR_PIXEL:
        SetPixelPinX(v);
        break;
      case BasicElement::PR_RELATIVE:
        SetRelativePinX(v);
        break;
      default:
        break;
    }
  }

  Variant GetPinY() const {
    return BasicElement::GetPixelOrRelative(pin_y_relative_, true,
                                            pin_y_, ppin_y_);
  }

  void SetPinY(const Variant &pin_y) {
    double v;
    switch (ParsePixelOrRelative(pin_y, &v)) {
      case BasicElement::PR_PIXEL:
        SetPixelPinY(v);
        break;
      case BasicElement::PR_RELATIVE:
        SetRelativePinY(v);
        break;
      default:
        break;
    }
  }

  void Layout() {
    if (!width_specified_ || !height_specified_) {
      double width, height;
      owner_->GetDefaultSize(&width, &height);
      if (!width_specified_)
        SetPixelWidth(width);
      if (!height_specified_)
        SetPixelHeight(height);
    }
    if (!x_specified_ || !y_specified_) {
      double x, y;
      owner_->GetDefaultPosition(&x, &y);
      if (!x_specified_)
        SetPixelX(x);
      if (!y_specified_)
        SetPixelY(y);
    }

    // Adjust relative size and positions.
    // Only values computed with parent width and height need change-checking
    // to react to parent size changes.
    // Other values have already triggered position_changed_ when they were set.
    double parent_width = GetParentWidth();
    if (x_relative_) {
      double new_x = px_ * parent_width;
      if (new_x != x_) {
        position_changed_ = true;
        x_ = new_x;
      }
    } else {
      px_ = parent_width > 0.0 ? x_ / parent_width : 0.0;
    }
    if (width_relative_) {
      double new_width = pwidth_ * parent_width;
      if (new_width != width_) {
        size_changed_ = true;
        width_ = new_width;
      }
    } else {
      pwidth_ = parent_width > 0.0 ? width_ / parent_width : 0.0;
    }

    double parent_height = GetParentHeight();
    if (y_relative_) {
      double new_y = py_ * parent_height;
      if (new_y != y_) {
        position_changed_ = true;
        y_ = new_y;
      }
    } else {
      py_ = parent_height > 0.0 ? y_ / parent_height : 0.0;
    }
    if (height_relative_) {
      double new_height = pheight_ * parent_height;
      if (new_height != height_) {
        size_changed_ = true;
        height_ = new_height;
      }
    } else {
      pheight_ = parent_height > 0.0 ? height_ / parent_height : 0.0;
    }

    if (pin_x_relative_) {
      pin_x_ = ppin_x_ * width_;
    } else {
      ppin_x_ = width_ > 0.0 ? pin_x_ / width_ : 0.0;
    }
    if (pin_y_relative_) {
      pin_y_ = ppin_y_ * height_;
    } else {
      ppin_y_ = height_ > 0.0 ? pin_y_ / height_ : 0.0;
    }

    if ((position_changed_ || size_changed_ || visibility_changed_) &&
        IsReallyVisible())
      view_->AddElementToClipRegion(owner_);

    if (size_changed_)
      PostSizeEvent();
    if (children_)
      children_->Layout();
  }

  void Draw(CanvasInterface *canvas) {
    // Only do draw if visible
    // Check for width_, height_ == 0 since IntersectRectClipRegion fails for
    // those cases.
    if (visible_ && opacity_ != 0 && width_ > 0 && height_ > 0) {
      const CanvasInterface *mask = GetMaskCanvas();
      CanvasInterface *target = canvas;

      // In order to get correct result, indirect draw is required for
      // following situations:
      // - The element has mask;
      // - The element's flips in x or y;
      // - Opacity of the element is not 1.0 and it has children.
      bool indirect_draw = mask || flip_ != FLIP_NONE ||
                          (opacity_ != 1.0 && children_ &&
                           children_->GetCount());
      if (indirect_draw) {
        target = view_->GetGraphics()->NewCanvas(size_t(ceil(width_)),
                                                 size_t(ceil(height_)));
      }

      canvas->PushState();
      canvas->IntersectRectClipRegion(0, 0, width_, height_);
      canvas->MultiplyOpacity(opacity_);
      owner_->DoDraw(target);

      if (indirect_draw) {
        double offset_x = 0, offset_y = 0;
        if (flip_ & FLIP_HORIZONTAL) {
          offset_x = -width_;
          canvas->ScaleCoordinates(-1, 1);
        }
        if (flip_ & FLIP_VERTICAL) {
          offset_y = -height_;
          canvas->ScaleCoordinates(1, -1);
        }
        if (mask)
          canvas->DrawCanvasWithMask(offset_x, offset_y, target,
                                     offset_x, offset_y, mask);
        else
          canvas->DrawCanvas(offset_x, offset_y, target);

        target->Destroy();
      }

      canvas->PopState();
      if (debug_mode_ >= 2) {
        DrawBoundingBox(canvas, width_, height_, debug_color_index_);
      }

#ifdef _DEBUG
      ++total_draw_count_;
      view_->IncreaseDrawCount();
      if ((total_draw_count_ % 1000) == 0)
        DLOG("BasicElement: %d draws, %d queues, %d%% q/d",
             total_draw_count_, total_queue_draw_count_,
             total_queue_draw_count_ * 100 / total_draw_count_);
#endif
    }

    GetViewCoord(owner_, old_view_rectangle_);
    visibility_changed_ = false;
    size_changed_ = false;
    position_changed_ = false;
  }

  void DrawChildren(CanvasInterface *canvas) {
    if (children_)
      children_->Draw(canvas);
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

 public:
  void QueueDraw() {
    if (visible_ || visibility_changed_) {
      view_->QueueDraw(owner_);
    }
#ifdef _DEBUG
    ++total_queue_draw_count_;
#endif
  }

  void PostSizeEvent() {
    if (onsize_event_.HasActiveConnections()) {
      Event *event = new SimpleEvent(Event::EVENT_SIZE);
      view_->PostEvent(new ScriptableEvent(event, owner_, NULL), onsize_event_);
    }
  }

  void PositionChanged() {
    position_changed_ = true;
    QueueDraw();
  }

  void WidthChanged() {
    size_changed_ = true;
    QueueDraw();
  }

  void HeightChanged() {
    size_changed_ = true;
    QueueDraw();
  }

  void MarkRedraw() {
    if (children_)
      children_->MarkRedraw();
  }

  bool IsReallyVisible() const {
    return visible_ && opacity_ != 0.0 &&
        (!parent_ ||
         (parent_->IsReallyVisible() && parent_->IsChildInVisibleArea(owner_)));
  }

 public:
  EventResult OnMouseEvent(const MouseEvent &event, bool direct,
                           BasicElement **fired_element,
                           BasicElement **in_element) {
    Event::Type type = event.GetType();
    ElementHolder this_element_holder(owner_);

    *fired_element = *in_element = NULL;

    if (!visible_ || opacity_ == 0) {
      return EVENT_RESULT_UNHANDLED;
    }

    if (!direct && children_) {
      // Send to the children first.
      EventResult result = children_->OnMouseEvent(event, fired_element,
                                                   in_element);
      if (!this_element_holder.Get() || *fired_element)
        return result;
    }

    if (!enabled_) {
      return EVENT_RESULT_UNHANDLED;
    }

    // Don't check mouse position, because the event may be out of this
    // element when this element is grabbing mouse.

    // Take this event, since no children took it, and we're enabled.
    ScriptableEvent scriptable_event(&event, owner_, NULL);
    if (type != Event::EVENT_MOUSE_MOVE) {
      DLOG("%s(%s|%s): x:%g y:%g dx:%d dy:%d b:%d m:%d",
           scriptable_event.GetName(),
           name_.c_str(), tag_name_.c_str(),
           event.GetX(), event.GetY(),
           event.GetWheelDeltaX(), event.GetWheelDeltaY(),
           event.GetButton(), event.GetModifier());
    }

    ElementHolder in_element_holder(*in_element);
    switch (type) {
      case Event::EVENT_MOUSE_MOVE: // put the high volume events near top
        view_->FireEvent(&scriptable_event, onmousemove_event_);
        break;
      case Event::EVENT_MOUSE_DOWN:
        view_->SetFocus(owner_);
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
      case Event::EVENT_MOUSE_RCLICK:
        view_->FireEvent(&scriptable_event, onrclick_event_);
        break;
      case Event::EVENT_MOUSE_RDBLCLICK:
        view_->FireEvent(&scriptable_event, onrdblclick_event_);
        break;
      case Event::EVENT_MOUSE_OUT:
        // TODO reset cursor
        view_->FireEvent(&scriptable_event, onmouseout_event_);
        break;
      case Event::EVENT_MOUSE_OVER:
        // TODO set cursor
        view_->FireEvent(&scriptable_event, onmouseover_event_);
        break;
      case Event::EVENT_MOUSE_WHEEL:
        view_->FireEvent(&scriptable_event, onmousewheel_event_);
        break;
      default:
        ASSERT(false);
    }

    EventResult result = scriptable_event.GetReturnValue();
    if (result != EVENT_RESULT_CANCELED && this_element_holder.Get())
      result = std::max(result, owner_->HandleMouseEvent(event));
    *fired_element = this_element_holder.Get();
    *in_element = in_element_holder.Get();
    return result;
  }

  EventResult OnDragEvent(const DragEvent &event, bool direct,
                          BasicElement **fired_element) {
    ElementHolder this_element_holder(owner_);

    *fired_element = NULL;
    if (!direct && children_) {
      // Send to the children first.
      EventResult result = children_->OnDragEvent(event, fired_element);
      if (!this_element_holder.Get() || *fired_element)
        return result;
    }

    if (!drop_target_ || !visible_ || opacity_ == 0)
      return EVENT_RESULT_UNHANDLED;

    // Take this event, since no children took it, and we're enabled.
    ScriptableEvent scriptable_event(&event, owner_, NULL);
    if (event.GetType() != Event::EVENT_DRAG_MOTION)
      DLOG("%s(%s|%s): %g %g file0=%s", scriptable_event.GetName(),
           name_.c_str(), tag_name_.c_str(),
           event.GetX(), event.GetY(), event.GetDragFiles()[0]);

    switch (event.GetType()) {
      case Event::EVENT_DRAG_MOTION: // put the high volume events near top
        // This event is only for drop target testing.
        break;
      case Event::EVENT_DRAG_OUT:
        view_->FireEvent(&scriptable_event, ondragout_event_);
        break;
      case Event::EVENT_DRAG_OVER:
        view_->FireEvent(&scriptable_event, ondragover_event_);
        break;
      case Event::EVENT_DRAG_DROP:
        view_->FireEvent(&scriptable_event, ondragdrop_event_);
        break;
      default:
        ASSERT(false);
    }

    EventResult result = scriptable_event.GetReturnValue();
    // For Drag events, we only return EVENT_RESULT_UNHANDLED if this element
    // is invisible or not a drag target. Some gadgets rely on this behavior.
    if (result == EVENT_RESULT_UNHANDLED)
      result = EVENT_RESULT_HANDLED;
    if (result != EVENT_RESULT_CANCELED && this_element_holder.Get())
      result = std::max(result, owner_->HandleDragEvent(event));
    *fired_element = this_element_holder.Get();
    return result;
  }

  EventResult OnKeyEvent(const KeyboardEvent &event) {
    if (!enabled_)
      return EVENT_RESULT_UNHANDLED;

    ElementHolder this_element_holder(owner_);
    ScriptableEvent scriptable_event(&event, owner_, NULL);
    DLOG("%s(%s|%s): %d", scriptable_event.GetName(),
         name_.c_str(), tag_name_.c_str(), event.GetKeyCode());

    switch (event.GetType()) {
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

    EventResult result = scriptable_event.GetReturnValue();
    if (result != EVENT_RESULT_CANCELED && this_element_holder.Get())
      result = std::max(result, owner_->HandleKeyEvent(event));
    return result;
  }

  EventResult OnOtherEvent(const Event &event) {
    if (!enabled_)
      return EVENT_RESULT_UNHANDLED;

    ElementHolder this_element_holder(owner_);
    ScriptableEvent scriptable_event(&event, owner_, NULL);
    DLOG("%s(%s|%s)", scriptable_event.GetName(),
         name_.c_str(), tag_name_.c_str());

    // TODO: focus logic.
    switch (event.GetType()) {
      case Event::EVENT_FOCUS_IN:
        view_->FireEvent(&scriptable_event, onfocusin_event_);
        break;
      case Event::EVENT_FOCUS_OUT:
        view_->FireEvent(&scriptable_event, onfocusout_event_);
        break;
      default:
        ASSERT(false);
    }
    EventResult result = scriptable_event.GetReturnValue();
    if (result != EVENT_RESULT_CANCELED && this_element_holder.Get())
      result = std::max(result, owner_->HandleOtherEvent(event));
    return result;
  }

 public:
  BasicElement *parent_;
  BasicElement *owner_;
  Elements *children_;
  View *view_;
  BasicElement::HitTest hittest_;
  ViewInterface::CursorType cursor_;
  bool drop_target_;
  bool enabled_;
  std::string tag_name_;
  std::string name_;
  double width_, height_, pwidth_, pheight_;
  bool width_relative_, height_relative_;
  bool width_specified_, height_specified_;
  double x_, y_, px_, py_;
  bool x_relative_, y_relative_;
  bool x_specified_, y_specified_;
  double pin_x_, pin_y_, ppin_x_, ppin_y_;
  bool pin_x_relative_, pin_y_relative_;
  double rotation_;
  double old_view_rectangle_[4];
  double opacity_;
  bool visible_;
  std::string tooltip_;
  FlipMode flip_;
  bool implicit_;

  ImageInterface *mask_image_;
  bool visibility_changed_;
  bool position_changed_;
  bool size_changed_;

  int debug_color_index_;
  static int total_debug_color_index_;
  int debug_mode_;

#ifdef _DEBUG
  static int total_draw_count_;
  static int total_queue_draw_count_;
#endif

  EventSignal onclick_event_;
  EventSignal ondblclick_event_;
  EventSignal onrclick_event_;
  EventSignal onrdblclick_event_;
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
  EventSignal onsize_event_;
};

int BasicElement::Impl::total_debug_color_index_ = 0;

#ifdef _DEBUG
int BasicElement::Impl::total_draw_count_ = 0;
int BasicElement::Impl::total_queue_draw_count_ = 0;
#endif

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

static const char *kFlipNames[] = {
  "none", "horizontal", "vertical", "both",
};

BasicElement::BasicElement(BasicElement *parent, View *view,
                           const char *tag_name, const char *name,
                           bool children)
    : impl_(new Impl(parent, view, tag_name, name, children, this)) {
}

void BasicElement::DoRegister() {
  RegisterProperty("x",
                   NewSlot(impl_, &Impl::GetX),
                   NewSlot(impl_, &Impl::SetX));
  RegisterProperty("y",
                   NewSlot(impl_, &Impl::GetY),
                   NewSlot(impl_, &Impl::SetY));
  RegisterProperty("width",
                   NewSlot(impl_, &Impl::GetWidth),
                   NewSlot(impl_, &Impl::SetWidth));
  RegisterProperty("height",
                   NewSlot(impl_, &Impl::GetHeight),
                   NewSlot(impl_, &Impl::SetHeight));
  RegisterConstant("name", impl_->name_);
  RegisterConstant("tagName", impl_->tag_name_);

  // TODO: Contentarea on Windows seems to support some of the following
  // properties.
  // if (GadgetStrCmp(tag_name, "contentarea") == 0)
    // The following properties and methods are not supported by contentarea.
  //  return;

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
  RegisterStringEnumProperty("hitTest",
                             NewSlot(this, &BasicElement::GetHitTest),
                             NewSlot(this, &BasicElement::SetHitTest),
                             kHitTestNames, arraysize(kHitTestNames));
  RegisterProperty("mask",
                   NewSlot(this, &BasicElement::GetMask),
                   NewSlot(this, &BasicElement::SetMask));
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
  RegisterConstant("parentElement", impl_->parent_);
  // Though we support relative pinX and pinY, this feature is not published
  // in the current public API, so pinX and pinY are still exposed to
  // script in pixels.
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
  RegisterProperty("visible",
                   NewSlot(this, &BasicElement::IsVisible),
                   NewSlot(this, &BasicElement::SetVisible));
  RegisterStringEnumProperty("flip",
                             NewSlot(this, &BasicElement::GetFlip),
                             NewSlot(this, &BasicElement::SetFlip),
                             kFlipNames, arraysize(kFlipNames));
  RegisterMethod("focus", NewSlot(this, &BasicElement::Focus));
  RegisterMethod("killFocus", NewSlot(this, &BasicElement::KillFocus));

  if (impl_->children_) {
    RegisterConstant("children", impl_->children_);
    RegisterMethod("appendElement",
                   NewSlot(impl_->children_, &Elements::AppendElementFromXML));
    RegisterMethod("insertElement",
                   NewSlot(impl_->children_, &Elements::InsertElementFromXML));
    RegisterMethod("removeElement",
                   NewSlot(impl_->children_, &Elements::RemoveElement));
    RegisterMethod("removeAllElements",
                   NewSlot(impl_->children_, &Elements::RemoveAllElements));
  }

  RegisterSignal(kOnClickEvent, &impl_->onclick_event_);
  RegisterSignal(kOnDblClickEvent, &impl_->ondblclick_event_);
  RegisterSignal(kOnRClickEvent, &impl_->onrclick_event_);
  RegisterSignal(kOnRDblClickEvent, &impl_->onrdblclick_event_);
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
  RegisterSignal(kOnSizeEvent, &impl_->onsize_event_);
}

BasicElement::~BasicElement() {
  DLOG("Basic element's dtor, this: %p", this);
  delete impl_;
}

std::string BasicElement::GetTagName() const {
  return impl_->tag_name_;
}

View *BasicElement::GetView() {
  return impl_->view_;
}

const View *BasicElement::GetView() const {
  return impl_->view_;
}

BasicElement::HitTest BasicElement::GetHitTest() const {
  return impl_->hittest_;
}

void BasicElement::SetHitTest(HitTest value) {
  impl_->hittest_ = value;
}

const Elements *BasicElement::GetChildren() const {
  return impl_->children_;
}

Elements *BasicElement::GetChildren() {
  return impl_->children_;
}

ViewInterface::CursorType BasicElement::GetCursor() const {
  return impl_->cursor_;
}

void BasicElement::SetCursor(ViewInterface::CursorType cursor) {
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

std::string BasicElement::GetName() const {
  return impl_->name_;
}

Variant BasicElement::GetMask() const {
  return Variant(GetImageTag(impl_->mask_image_));
}

void BasicElement::SetMask(const Variant &mask) {
  impl_->SetMask(mask);
}

const CanvasInterface *BasicElement::GetMaskCanvas() {
  return impl_->GetMaskCanvas();
}

double BasicElement::GetPixelWidth() const {
  return impl_->width_;
}

void BasicElement::SetPixelWidth(double width) {
  impl_->width_specified_ = true;
  impl_->SetPixelWidth(width);
}

double BasicElement::GetPixelHeight() const {
  return impl_->height_;
}

void BasicElement::SetPixelHeight(double height) {
  impl_->height_specified_ = true;
  impl_->SetPixelHeight(height);
}

void BasicElement::GetOldViewRectangle(double *rect) const {
  memcpy(rect, impl_->old_view_rectangle_, sizeof(impl_->old_view_rectangle_));
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
  impl_->x_specified_ = true;
  impl_->SetPixelX(x);
}

double BasicElement::GetPixelY() const {
  return impl_->y_;
}

void BasicElement::SetPixelY(double y) {
  impl_->y_specified_ = true;
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
  impl_->width_specified_ = true;
  impl_->SetRelativeWidth(width);
}

void BasicElement::SetRelativeHeight(double height) {
  impl_->height_specified_ = true;
  impl_->SetRelativeHeight(height);
}

void BasicElement::SetRelativeX(double x) {
  impl_->x_specified_ = true;
  impl_->SetRelativeX(x);
}

void BasicElement::SetRelativeY(double y) {
  impl_->y_specified_ = true;
  impl_->SetRelativeY(y);
}

double BasicElement::GetRelativePinX() const {
  return impl_->ppin_x_;
}

void BasicElement::SetRelativePinX(double x) {
  impl_->SetRelativePinX(x);
}

double BasicElement::GetRelativePinY() const {
  return impl_->ppin_y_;
}

void BasicElement::SetRelativePinY(double y) {
  return impl_->SetRelativePinY(y);
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

bool BasicElement::WidthIsSpecified() const {
  return impl_->width_specified_;
}

void BasicElement::ResetWidthToDefault() {
  impl_->ResetWidthToDefault();
}

bool BasicElement::HeightIsSpecified() const {
  return impl_->height_specified_;
}

void BasicElement::ResetHeightToDefault() {
  impl_->ResetHeightToDefault();
}

bool BasicElement::XIsSpecified() const {
  return impl_->x_specified_;
}

void BasicElement::ResetXToDefault() {
  impl_->ResetXToDefault();
}

bool BasicElement::YIsSpecified() const {
  return impl_->y_specified_;
}

void BasicElement::ResetYToDefault() {
  impl_->ResetYToDefault();
}

Variant BasicElement::GetX() const {
  return impl_->GetX();
}

void BasicElement::SetX(const Variant &x) {
  impl_->SetX(x);
}

Variant BasicElement::GetY() const {
  return impl_->GetY();
}

void BasicElement::SetY(const Variant &y) {
  impl_->SetY(y);
}

Variant BasicElement::GetWidth() const {
  return impl_->GetWidth();
}

void BasicElement::SetWidth(const Variant &width) {
  impl_->SetWidth(width);
}

Variant BasicElement::GetHeight() const {
  return impl_->GetHeight();
}

void BasicElement::SetHeight(const Variant &height) {
  impl_->SetHeight(height);
}

Variant BasicElement::GetPinX() const {
  return impl_->GetPinX();
}

void BasicElement::SetPinX(const Variant &pin_x) {
  impl_->SetPinX(pin_x);
}

Variant BasicElement::GetPinY() const {
  return impl_->GetPinY();
}

void BasicElement::SetPinY(const Variant &pin_y) {
  impl_->SetPinY(pin_y);
}

double BasicElement::GetClientWidth() const {
  return GetPixelWidth();
}

double BasicElement::GetClientHeight() const {
  return GetPixelHeight();
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

bool BasicElement::IsReallyVisible() const {
  return impl_->IsReallyVisible();
}

bool BasicElement::IsReallyEnabled() const {
  return impl_->enabled_ && IsReallyVisible();
}

bool BasicElement::IsFullyOpaque() const {
  if (!HasOpaqueBackground() || impl_->mask_image_ != NULL)
    return false;

  double opacity = GetOpacity();
  const BasicElement *elm = GetParentElement();
  for (; elm != NULL; elm = elm->GetParentElement())
    opacity *= elm->GetOpacity();

  return opacity == 1.0;
}

BasicElement *BasicElement::GetParentElement() {
  return impl_->parent_;
}

const BasicElement *BasicElement::GetParentElement() const {
  return impl_->parent_;
}

std::string BasicElement::GetTooltip() const {
  return impl_->tooltip_;
}

void BasicElement::SetTooltip(const char *tooltip) {
  if (tooltip)
    impl_->tooltip_ = tooltip;
  else
    impl_->tooltip_.clear();
}

BasicElement::FlipMode BasicElement::GetFlip() const {
  return impl_->flip_;
}

void BasicElement::SetFlip(FlipMode flip) {
  if (impl_->flip_ != flip) {
    impl_->flip_ = flip;
    QueueDraw();
  }
}

void BasicElement::Focus() {
  impl_->view_->SetFocus(this);
}

void BasicElement::KillFocus() {
  impl_->view_->SetFocus(NULL);
}

void BasicElement::Layout() {
  impl_->Layout();
}

void BasicElement::Draw(CanvasInterface *canvas) {
  impl_->Draw(canvas);
}

void BasicElement::DrawChildren(CanvasInterface *canvas) {
  impl_->DrawChildren(canvas);
}

void BasicElement::ClearPositionChanged() {
  impl_->position_changed_ = false;
}

bool BasicElement::IsPositionChanged() const {
  return impl_->position_changed_;
}

bool BasicElement::IsSizeChanged() const {
  return impl_->size_changed_;
}

bool BasicElement::SetChildrenScrollable(bool scrollable) {
  if (impl_->children_) {
    impl_->children_->SetScrollable(scrollable);
    return true;
  }
  return false;
}

bool BasicElement::GetChildrenExtents(double *width, double *height) {
  if (impl_->children_) {
    impl_->children_->GetChildrenExtents(width, height);
    return true;
  }
  return false;
}

void BasicElement::QueueDraw() {
  impl_->QueueDraw();
}

void BasicElement::MarkRedraw() {
  impl_->MarkRedraw();
}

EventResult BasicElement::OnMouseEvent(const MouseEvent &event, bool direct,
                                       BasicElement **fired_element,
                                       BasicElement **in_element) {
  return impl_->OnMouseEvent(event, direct, fired_element, in_element);
}

EventResult BasicElement::OnDragEvent(const DragEvent &event, bool direct,
                                      BasicElement **fired_element) {
  return impl_->OnDragEvent(event, direct, fired_element);
}

EventResult BasicElement::OnKeyEvent(const KeyboardEvent &event) {
  return impl_->OnKeyEvent(event);
}

EventResult BasicElement::OnOtherEvent(const Event &event) {
  return impl_->OnOtherEvent(event);
}

bool BasicElement::IsPointIn(double x, double y) {
  if (!IsPointInElement(x, y, impl_->width_, impl_->height_))
    return false;

  const CanvasInterface *mask = GetMaskCanvas();
  if (!mask) return true;

  double opacity;

  // If failed to get the value of the point, then just return false, assuming
  // the point is out of the mask.
  if (!mask->GetPointValue(x, y, NULL, &opacity))
    return false;

  return opacity > 0;
}

void BasicElement::SelfCoordToChildCoord(const BasicElement *child,
                                         double x, double y,
                                         double *child_x,
                                         double *child_y) const {
  ParentCoordToChildCoord(x, y, child->GetPixelX(), child->GetPixelY(),
                          child->GetPixelPinX(), child->GetPixelPinY(),
                          DegreesToRadians(child->GetRotation()),
                          child_x, child_y);
  FlipMode flip = child->GetFlip();
  if (flip & FLIP_HORIZONTAL)
    *child_x = child->GetPixelWidth() - *child_x;
  if (flip & FLIP_VERTICAL)
    *child_y = child->GetPixelHeight() - *child_y;
}

void BasicElement::ChildCoordToSelfCoord(const BasicElement *child,
                                         double x, double y,
                                         double *self_x,
                                         double *self_y) const {
  FlipMode flip = child->GetFlip();
  if (flip & FLIP_HORIZONTAL)
    x = child->GetPixelWidth() - x;
  if (flip & FLIP_VERTICAL)
    y = child->GetPixelHeight() - y;
  ChildCoordToParentCoord(x, y, child->GetPixelX(), child->GetPixelY(),
                          child->GetPixelPinX(), child->GetPixelPinY(),
                          DegreesToRadians(child->GetRotation()),
                          self_x, self_y);
}

void BasicElement::SelfCoordToParentCoord(double x, double y,
                                          double *parent_x,
                                          double *parent_y) const {
  const BasicElement *parent = GetParentElement();
  if (parent) {
    parent->ChildCoordToSelfCoord(this, x, y, parent_x, parent_y);
  } else {
    FlipMode flip = GetFlip();
    if (flip & FLIP_HORIZONTAL)
      x = GetPixelWidth() - x;
    if (flip & FLIP_VERTICAL)
      y = GetPixelHeight() - y;
    ChildCoordToParentCoord(x, y, GetPixelX(), GetPixelY(),
                            GetPixelPinX(), GetPixelPinY(),
                            DegreesToRadians(GetRotation()),
                            parent_x, parent_y);
  }
}

void BasicElement::ParentCoordToSelfCoord(double parent_x, double parent_y,
                                          double *x, double *y) const {
  const BasicElement *parent = GetParentElement();
  if (parent) {
    parent->SelfCoordToChildCoord(this, parent_x, parent_y, x, y);
  } else {
    ParentCoordToChildCoord(parent_x, parent_y,
                            GetPixelX(), GetPixelY(),
                            GetPixelPinX(), GetPixelPinY(),
                            DegreesToRadians(GetRotation()),
                            x, y);
    FlipMode flip = GetFlip();
    if (flip & FLIP_HORIZONTAL)
      *x = GetPixelWidth() - *x;
    if (flip & FLIP_VERTICAL)
      *y = GetPixelHeight() - *y;
  }
}

void BasicElement::SelfCoordToViewCoord(double x, double y,
                                        double *view_x, double *view_y) const {
  const BasicElement *elm = this;
  while (elm) {
    elm->SelfCoordToParentCoord(x, y, &x, &y);
    elm = elm->GetParentElement();
  }
  if (view_x) *view_x = x;
  if (view_y) *view_y = y;
}

void BasicElement::ViewCoordToSelfCoord(double view_x, double view_y,
                                        double *self_x, double *self_y) const {
  std::vector<const BasicElement *> elements;
  for (const BasicElement *e = this; e != NULL; e = e->GetParentElement())
    elements.push_back(e);

  std::vector<const BasicElement *>::reverse_iterator it = elements.rbegin();
  for (; it != elements.rend(); ++it)
    (*it)->ParentCoordToSelfCoord(view_x, view_y, &view_x, &view_y);

  if (self_x) *self_x = view_x;
  if (self_y) *self_y = view_y;
}

void BasicElement::GetDefaultSize(double *width, double *height) const {
  ASSERT(width);
  ASSERT(height);
  *width = *height = 0;
}

void BasicElement::GetDefaultPosition(double *x, double *y) const {
  ASSERT(x && y);
  *x = *y = 0;
}

// Returns when input is: pixel: 0; relative: 1; 2: unspecified; invalid: -1.
BasicElement::ParsePixelOrRelativeResult
BasicElement::ParsePixelOrRelative(const Variant &input, double *output) {
  ASSERT(output);
  *output = 0;
  std::string str;

  if (input.ConvertToString(&str) && str.empty())
    return PR_UNSPECIFIED;

  if (input.ConvertToDouble(output))
    return (std::isnan(*output) || std::isinf(*output)) ?
           PR_UNSPECIFIED : PR_PIXEL;

  if (!input.ConvertToString(&str) || str.empty())
    return PR_UNSPECIFIED;

  char *end_ptr;
  *output = strtod(str.c_str(), &end_ptr);
  if (*end_ptr == '%' && *(end_ptr + 1) == '\0') {
    *output /= 100.0;
    return PR_RELATIVE;
  }
  LOG("Invalid pixel or relative value: %s", input.Print().c_str());
  return PR_INVALID;
}

Variant BasicElement::GetPixelOrRelative(bool is_relative,
                                         bool is_specified,
                                         double pixel,
                                         double relative) {
  if (!is_specified)
    return Variant();

  if (is_relative) {
    char buf[20];
    snprintf(buf, sizeof(buf), "%d%%", static_cast<int>(relative * 100));
    return Variant(std::string(buf));
  } else {
    return Variant(static_cast<int>(round(pixel)));
  }
}

bool BasicElement::IsImplicit() const {
  return impl_->implicit_;
}

void BasicElement::SetImplicit(bool implicit) {
  ASSERT(!implicit || impl_->parent_);
  impl_->implicit_ = implicit;
}

void BasicElement::OnPopupOff() {
}

bool BasicElement::OnAddContextMenuItems(MenuInterface *menu) {
  // Let the default menu items shown by default.
  return true;
}

bool BasicElement::IsChildInVisibleArea(const BasicElement *child) const {
  return true;
}

bool BasicElement::HasOpaqueBackground() const {
  return false;
}

Connection *BasicElement::ConnectOnClickEvent(Slot0<void> *handler) {
  return impl_->onclick_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnDblClickEvent(Slot0<void> *handler) {
  return impl_->ondblclick_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnRClickEvent(Slot0<void> *handler) {
  return impl_->onrclick_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnRDblClickEvent(Slot0<void> *handler) {
  return impl_->onrdblclick_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnDragDropEvent(Slot0<void> *handler) {
  return impl_->ondragdrop_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnDragOutEvent(Slot0<void> *handler) {
  return impl_->ondragout_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnDragOverEvent(Slot0<void> *handler) {
  return impl_->ondragover_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnFocusInEvent(Slot0<void> *handler) {
  return impl_->onfocusin_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnFocusOutEvent(Slot0<void> *handler) {
  return impl_->onfocusout_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnKeyDownEvent(Slot0<void> *handler) {
  return impl_->onkeydown_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnKeyPressEvent(Slot0<void> *handler) {
  return impl_->onkeypress_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnKeyUpEvent(Slot0<void> *handler) {
  return impl_->onkeyup_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnMouseDownEvent(Slot0<void> *handler) {
  return impl_->onmousedown_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnMouseMoveEvent(Slot0<void> *handler) {
  return impl_->onmousemove_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnMouseOverEvent(Slot0<void> *handler) {
  return impl_->onmouseover_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnMouseOutEvent(Slot0<void> *handler) {
  return impl_->onmouseout_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnMouseUpEvent(Slot0<void> *handler) {
  return impl_->onmouseup_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnMouseWheelEvent(Slot0<void> *handler) {
  return impl_->onmousewheel_event_.Connect(handler);
}
Connection *BasicElement::ConnectOnSizeEvent(Slot0<void> *handler) {
  return impl_->onsize_event_.Connect(handler);
}

void GetViewCoord(const BasicElement *element, double rect[4]) {
  double r[8];
  element->SelfCoordToViewCoord(0, 0, &r[0], &r[1]);
  element->SelfCoordToViewCoord(0, element->GetPixelHeight(), &r[2], &r[3]);
  element->SelfCoordToViewCoord(element->GetPixelWidth(),
                                element->GetPixelHeight(), &r[4], &r[5]);
  element->SelfCoordToViewCoord(element->GetPixelWidth(), 0, &r[6], &r[7]);
  GetRectangleExtents(r, rect, rect + 1, rect + 2, rect + 3);
}

} // namespace ggadget
