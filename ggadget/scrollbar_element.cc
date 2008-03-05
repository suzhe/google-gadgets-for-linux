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

#include "scrollbar_element.h"
#include "canvas_interface.h"
#include "gadget_consts.h"
#include "image_interface.h"
#include "logger.h"
#include "math_utils.h"
#include "scriptable_binary_data.h"
#include "scriptable_event.h"
#include "string_utils.h"
#include "view.h"

namespace ggadget {

enum DisplayState {
  STATE_NORMAL,
  STATE_DOWN,
  STATE_OVER,
  STATE_COUNT
};

enum ScrollBarComponent {
  COMPONENT_DOWNLEFT_BUTTON,
  COMPONENT_UPRIGHT_BUTTON,
  COMPONENT_DOWNLEFT_BAR,
  COMPONENT_UPRIGHT_BAR,
  COMPONENT_THUMB_BUTTON
};

static const char *kOrientationNames[] = {
  "vertical", "horizontal"
};

class ScrollBarElement::Impl {
 public:
  Impl(ScrollBarElement *owner, View *view)
      : owner_(owner),
        left_state_(STATE_NORMAL), right_state_(STATE_NORMAL),
        thumb_state_(STATE_NORMAL),
        // The values below are the default ones in Windows.
        min_(0), max_(100), value_(0), pagestep_(10), linestep_(1),
        accum_wheel_delta_(0), drag_delta_(0.),
        // Windows default to horizontal for orientation,
        // but puzzlingly use vertical images as default.
        orientation_(ORIENTATION_VERTICAL) {
    left_[STATE_NORMAL] =
        view->LoadImageFromGlobal(kScrollDefaultLeft, false);
    left_[STATE_DOWN] =
      view->LoadImageFromGlobal(kScrollDefaultLeftDown, false);
    left_[STATE_OVER] =
      view->LoadImageFromGlobal(kScrollDefaultLeftOver, false);

    right_[STATE_NORMAL] =
      view->LoadImageFromGlobal(kScrollDefaultRight, false);
    right_[STATE_DOWN] =
      view->LoadImageFromGlobal(kScrollDefaultRightDown, false);
    right_[STATE_OVER] =
      view->LoadImageFromGlobal(kScrollDefaultRightOver, false);

    thumb_[STATE_NORMAL] =
      view->LoadImageFromGlobal(kScrollDefaultThumb, false);
    thumb_[STATE_DOWN] =
      view->LoadImageFromGlobal(kScrollDefaultThumbDown, false);
    thumb_[STATE_OVER] =
      view->LoadImageFromGlobal(kScrollDefaultThumbOver, false);

    background_ = view->LoadImageFromGlobal(kScrollDefaultBackground, false);
  }
  ~Impl() {
    for (int i = STATE_NORMAL; i < STATE_COUNT; i++) {
      DestroyImage(left_[i]);
      DestroyImage(right_[i]);
      DestroyImage(thumb_[i]);
    }

    DestroyImage(background_);
  }

  void ClearDisplayStates() {
    left_state_ = STATE_NORMAL;
    right_state_ = STATE_NORMAL;
    thumb_state_ = STATE_NORMAL;
  }

  void GetButtonLocation(bool downleft,
                         double *x, double *y, double *w, double *h) {
    ImageInterface *img = downleft ? left_[left_state_] : right_[right_state_];
    double imgw =  img ? img->GetWidth() : 0;
    double imgh =  img ? img->GetHeight() : 0;
    if (orientation_ == ORIENTATION_HORIZONTAL) {
      *x = downleft ? 0 : owner_->GetPixelWidth() - imgw;
      *y = (owner_->GetPixelHeight() - imgh) / 2.;
    } else {
      *x = (owner_->GetPixelWidth() - imgw) / 2.;
      *y = downleft ? 0 : owner_->GetPixelHeight() - imgh;
    }
    *w = imgw;
    *h = imgh;
  }

  void GetThumbLocation(double leftx, double lefty,
                        double leftwidth, double leftheight,
                        double rightx, double righty,
                        double *x, double *y, double *w, double *h) {
    ImageInterface *img = thumb_[thumb_state_];
    double imgw =  img ? img->GetWidth() : 0;
    double imgh =  img ? img->GetHeight() : 0;

    double position;
    if (max_ == min_) { // prevent overflow
      position = 0;
    } else {
      position = static_cast<double>(value_ - min_) / (max_ - min_);
    }
    if (orientation_ == ORIENTATION_HORIZONTAL) {
      leftx += leftwidth;
      *x = leftx + (rightx - leftx - imgw) * position;
      *y = (owner_->GetPixelHeight() - imgh) / 2.;
    } else {
      *x = (owner_->GetPixelWidth() - imgw) / 2.;
      lefty += leftheight;
      *y = lefty + (righty - lefty - imgh) * position;
    }
    *w = imgw;
    *h = imgh;
  }

  /**
   * Utility function for getting the int value from a position on the
   * scrollbar. It does not check to make sure that the value is within range.
   */
  int GetValueFromLocation(double x, double y) {
    // cache these values?
    double lx, ly, lw, lh;
    GetButtonLocation(true, &lx, &ly, &lw, &lh);
    double rx, ry, unused;
    GetButtonLocation(false, &rx, &ry, &unused, &unused);

    ImageInterface *img = thumb_[thumb_state_];
    int delta = max_ - min_;
    double position, denominator;
    if (orientation_ == ORIENTATION_HORIZONTAL) {
      double imgw =  img ? img->GetWidth() : 0;
      lx += lw;
      denominator = rx - imgw - lx;
      if (denominator == 0) { // prevent overflow
        position = 0;
      } else {
        position = delta * (x - lx - drag_delta_) / denominator;
      }
    } else {
      double imgh =  img ? img->GetHeight() : 0;
      ly += lh;
      denominator = ry - imgh - ly;
      if (denominator == 0) { // prevent overflow
        position = 0;
      } else {
        position = delta * (y - ly - drag_delta_) / denominator;
      }
    }

    int answer = static_cast<int>(position);
    answer += min_;
    return answer;
  }

  void SetValue(int value) {
    if (value > max_) {
      value = max_;
    } else if (value < min_) {
      value = min_;
    }

    if (value != value_) {
      value_ = value;
      DLOG("scroll value: %d", value_);
      owner_->QueueDraw();
      SimpleEvent event(Event::EVENT_CHANGE);
      ScriptableEvent s_event(&event, owner_, NULL);
      owner_->GetView()->FireEvent(&s_event, onchange_event_);
    }
  }

  void Scroll(bool downleft, bool line) {
    int delta = line ? linestep_ : pagestep_;
    int v = value_ + (downleft ? -delta : delta);
    SetValue(v);
  }

  /**
   * Returns the scrollbar component that is under the (x, y) position.
   * For buttons, also return the top left coordinate of that component.
   */
  ScrollBarComponent GetComponentFromPosition(double x, double y,
                                              double *compx, double *compy) {
    double lx, ly, lw, lh;
    GetButtonLocation(true, &lx, &ly, &lw, &lh);
    double rx, ry, rw, rh;
    GetButtonLocation(false, &rx, &ry, &rw, &rh);
    double tx, ty, tw, th;
    GetThumbLocation(lx, ly, lw, lh, rx, ry, &tx, &ty, &tw, &th);

    // Check in reverse of drawn order: thumb, right, left.
    if (IsPointInElement(x - tx, y - ty, tw, th)) {
      *compx = tx;
      *compy = ty;
      return COMPONENT_THUMB_BUTTON;
    }

    if (IsPointInElement(x - rx, y - ry, rw, rh)) {
      *compx = rx;
      *compy = ry;
      return COMPONENT_UPRIGHT_BUTTON;
    }

    if (IsPointInElement(x - lx, y - ly, lw, lh)) {
      *compx = lx;
      *compy = ly;
      return COMPONENT_DOWNLEFT_BUTTON;
    }

    if ((orientation_ == ORIENTATION_HORIZONTAL) ? (x < tx) : (y < ty)) {
      return COMPONENT_DOWNLEFT_BAR;
    }

    return COMPONENT_UPRIGHT_BAR;
  }

  ScrollBarElement *owner_;
  DisplayState left_state_, right_state_, thumb_state_;
  ImageInterface *left_[STATE_COUNT], *right_[STATE_COUNT],
                 *thumb_[STATE_COUNT], *background_;
  int min_, max_, value_, pagestep_, linestep_;
  int accum_wheel_delta_;
  double drag_delta_;
  Orientation orientation_;
  EventSignal onchange_event_;
};

ScrollBarElement::ScrollBarElement(BasicElement *parent, View *view,
                                   const char *name)
    : BasicElement(parent, view, "scrollbar", name, false),
      impl_(new Impl(this, view)) {
}

void ScrollBarElement::DoRegister() {
  BasicElement::DoRegister();
  RegisterProperty("background",
                   NewSlot(this, &ScrollBarElement::GetBackground),
                   NewSlot(this, &ScrollBarElement::SetBackground));
  RegisterProperty("leftDownImage",
                   NewSlot(this, &ScrollBarElement::GetLeftDownImage),
                   NewSlot(this, &ScrollBarElement::SetLeftDownImage));
  RegisterProperty("leftImage",
                   NewSlot(this, &ScrollBarElement::GetLeftImage),
                   NewSlot(this, &ScrollBarElement::SetLeftImage));
  RegisterProperty("leftOverImage",
                   NewSlot(this, &ScrollBarElement::GetLeftOverImage),
                   NewSlot(this, &ScrollBarElement::SetLeftOverImage));
  RegisterProperty("lineStep",
                   NewSlot(this, &ScrollBarElement::GetLineStep),
                   NewSlot(this, &ScrollBarElement::SetLineStep));
  RegisterProperty("max",
                   NewSlot(this, &ScrollBarElement::GetMax),
                   NewSlot(this, &ScrollBarElement::SetMax));
  RegisterProperty("min",
                   NewSlot(this, &ScrollBarElement::GetMin),
                   NewSlot(this, &ScrollBarElement::SetMin));
  RegisterStringEnumProperty("orientation",
                   NewSlot(this, &ScrollBarElement::GetOrientation),
                   NewSlot(this, &ScrollBarElement::SetOrientation),
                   kOrientationNames, arraysize(kOrientationNames));
  RegisterProperty("pageStep",
                   NewSlot(this, &ScrollBarElement::GetPageStep),
                   NewSlot(this, &ScrollBarElement::SetPageStep));
  RegisterProperty("rightDownImage",
                   NewSlot(this, &ScrollBarElement::GetRightDownImage),
                   NewSlot(this, &ScrollBarElement::SetRightDownImage));
  RegisterProperty("rightImage",
                   NewSlot(this, &ScrollBarElement::GetRightImage),
                   NewSlot(this, &ScrollBarElement::SetRightImage));
  RegisterProperty("rightOverImage",
                   NewSlot(this, &ScrollBarElement::GetRightOverImage),
                   NewSlot(this, &ScrollBarElement::SetRightOverImage));
  RegisterProperty("thumbDownImage",
                   NewSlot(this, &ScrollBarElement::GetThumbDownImage),
                   NewSlot(this, &ScrollBarElement::SetThumbDownImage));
  RegisterProperty("thumbImage",
                   NewSlot(this, &ScrollBarElement::GetThumbImage),
                   NewSlot(this, &ScrollBarElement::SetThumbImage));
  RegisterProperty("thumbOverImage",
                   NewSlot(this, &ScrollBarElement::GetThumbOverImage),
                   NewSlot(this, &ScrollBarElement::SetThumbOverImage));
  RegisterProperty("value",
                   NewSlot(this, &ScrollBarElement::GetValue),
                   NewSlot(this, &ScrollBarElement::SetValue));

  RegisterSignal(kOnChangeEvent, &impl_->onchange_event_);
}

ScrollBarElement::~ScrollBarElement() {
  delete impl_;
  impl_ = NULL;
}

void ScrollBarElement::DoDraw(CanvasInterface *canvas) {
  double width = GetPixelWidth();
  double height = GetPixelHeight();
  double lx, ly, rx, ry, tx, ty, lw, lh, unused;
  impl_->GetButtonLocation(true, &lx, &ly, &lw, &lh);
  impl_->GetButtonLocation(false, &rx, &ry, &unused, &unused);
  impl_->GetThumbLocation(lx, ly, lw, lh, rx, ry,
                          &tx, &ty, &unused, &unused);
  /*DLOG("DRAW wh %f %f, l %f %f, r %f %f, t %f %f",
       width, height, lx, ly, rx, ry, tx, ty);*/

  // Drawing order: background, left, right, thumb.
  impl_->background_->StretchDraw(canvas, 0, 0, width, height);
  impl_->left_[impl_->left_state_]->Draw(canvas, lx, ly);
  impl_->right_[impl_->right_state_]->Draw(canvas, rx, ry);
  impl_->thumb_[impl_->thumb_state_]->Draw(canvas, tx, ty);
}

int ScrollBarElement::GetMax() const {
  return impl_->max_;
}

void ScrollBarElement::SetMax(int value) {
  if (value != impl_->max_) {
    impl_->max_ = value;
    if (impl_->value_ > value) {
      impl_->value_ = value;
    }
    QueueDraw();
  }
}

int ScrollBarElement::GetMin() const {
  return impl_->min_;
}

void ScrollBarElement::SetMin(int value) {
  if (value != impl_->min_) {
    impl_->min_ = value;
    if (impl_->value_ < value) {
      impl_->value_ = value;
    }
    QueueDraw();
  }
}

int ScrollBarElement::GetPageStep() const {
  return impl_->pagestep_;
}

void ScrollBarElement::SetPageStep(int value) {
  impl_->pagestep_ = value;
}

int ScrollBarElement::GetLineStep() const {
  return impl_->linestep_;
}

void ScrollBarElement::SetLineStep(int value) {
  impl_->linestep_ = value;
}

int ScrollBarElement::GetValue() const {
  return impl_->value_;
}

void ScrollBarElement::SetValue(int value) {
  impl_->SetValue(value);
}

ScrollBarElement::Orientation ScrollBarElement::GetOrientation() const {
  return impl_->orientation_;
}

void ScrollBarElement::SetOrientation(ScrollBarElement::Orientation o) {
  if (o != impl_->orientation_) {
    impl_->orientation_ = o;
    QueueDraw();
  }
}

static bool VariantIsEmpty(const Variant &img) {
  switch (img.type()) {
    case Variant::TYPE_VOID:
      return true;
    case Variant::TYPE_STRING:
      return VariantValue<std::string>()(img).empty();
    case Variant::TYPE_SCRIPTABLE:
      return VariantValue<ScriptableBinaryData *>()(img) == NULL;
    default:
      // Any values or types not recognized are treated as empty.
      return true;
  }
  return false;
}

static void LoadImage(View *view, const Variant &src,
                      const char *default_image_name, ImageInterface **image) {
  DestroyImage(*image);
  if (VariantIsEmpty(src)) {
    *image = view->LoadImageFromGlobal(default_image_name, false);
  } else {
    *image = view->LoadImage(src, false);
  }
}

Variant ScrollBarElement::GetBackground() const {
  return Variant(GetImageTag(impl_->background_));
}

void ScrollBarElement::SetBackground(const Variant &img) {
  LoadImage(GetView(), img, kScrollDefaultBackground, &impl_->background_);
  QueueDraw();
}

Variant ScrollBarElement::GetLeftDownImage() const {
  return Variant(GetImageTag(impl_->left_[STATE_DOWN]));
}

void ScrollBarElement::SetLeftDownImage(const Variant &img) {
  LoadImage(GetView(), img, kScrollDefaultLeftDown, &impl_->left_[STATE_DOWN]);
  if (impl_->left_state_ == STATE_DOWN) {
    QueueDraw();
  }
}

Variant ScrollBarElement::GetLeftImage() const {
  return Variant(GetImageTag(impl_->left_[STATE_NORMAL]));
}

void ScrollBarElement::SetLeftImage(const Variant &img) {
  LoadImage(GetView(), img, kScrollDefaultLeft, &impl_->left_[STATE_NORMAL]);
  if (impl_->left_state_ == STATE_NORMAL) {
    QueueDraw();
  }
}

Variant ScrollBarElement::GetLeftOverImage() const {
  return Variant(GetImageTag(impl_->left_[STATE_OVER]));
}

void ScrollBarElement::SetLeftOverImage(const Variant &img) {
  LoadImage(GetView(), img, kScrollDefaultLeftOver, &impl_->left_[STATE_OVER]);
  if (impl_->left_state_ == STATE_OVER) {
    QueueDraw();
  }
}

Variant ScrollBarElement::GetRightDownImage() const {
  return Variant(GetImageTag(impl_->right_[STATE_DOWN]));
}

void ScrollBarElement::SetRightDownImage(const Variant &img) {
  LoadImage(GetView(), img, kScrollDefaultRightDown,
            &impl_->right_[STATE_DOWN]);
  if (impl_->right_state_ == STATE_DOWN) {
    QueueDraw();
  }
}

Variant ScrollBarElement::GetRightImage() const {
  return Variant(GetImageTag(impl_->right_[STATE_NORMAL]));
}

void ScrollBarElement::SetRightImage(const Variant &img) {
  LoadImage(GetView(), img, kScrollDefaultRight, &impl_->right_[STATE_NORMAL]);
  if (impl_->right_state_ == STATE_NORMAL) {
    QueueDraw();
  }
}

Variant ScrollBarElement::GetRightOverImage() const {
  return Variant(GetImageTag(impl_->right_[STATE_OVER]));
}

void ScrollBarElement::SetRightOverImage(const Variant &img) {
  LoadImage(GetView(), img, kScrollDefaultRightOver,
            &impl_->right_[STATE_OVER]);
  if (impl_->right_state_ == STATE_OVER) {
    QueueDraw();
  }
}

Variant ScrollBarElement::GetThumbDownImage() const {
  return Variant(GetImageTag(impl_->thumb_[STATE_DOWN]));
}

void ScrollBarElement::SetThumbDownImage(const Variant &img) {
  LoadImage(GetView(), img, kScrollDefaultThumbDown,
            &impl_->thumb_[STATE_DOWN]);
  if (impl_->thumb_state_ == STATE_DOWN) {
    QueueDraw();
  }
}

Variant ScrollBarElement::GetThumbImage() const {
  return Variant(GetImageTag(impl_->thumb_[STATE_NORMAL]));
}

void ScrollBarElement::SetThumbImage(const Variant &img) {
  LoadImage(GetView(), img, kScrollDefaultThumb, &impl_->thumb_[STATE_NORMAL]);
  if (impl_->thumb_state_ == STATE_NORMAL) {
    QueueDraw();
  }
}

Variant ScrollBarElement::GetThumbOverImage() const {
  return Variant(GetImageTag(impl_->thumb_[STATE_OVER]));
}

void ScrollBarElement::SetThumbOverImage(const Variant &img) {
  LoadImage(GetView(), img, kScrollDefaultThumbOver,
            &impl_->thumb_[STATE_OVER]);
  if (impl_->thumb_state_ == STATE_OVER) {
    QueueDraw();
  }
}

BasicElement *ScrollBarElement::CreateInstance(BasicElement *parent, View *view,
                                               const char *name) {
  return new ScrollBarElement(parent, view, name);
}

EventResult ScrollBarElement::HandleMouseEvent(const MouseEvent &event) {
  EventResult result = EVENT_RESULT_HANDLED;
  double compx = .0, compy = .0;
  ScrollBarComponent c = impl_->GetComponentFromPosition(event.GetX(),
                                                         event.GetY(),
                                                         &compx, &compy);
  // Resolve in opposite order as drawn: thumb, right, left.
  switch (event.GetType()) {
    case Event::EVENT_MOUSE_MOVE:
    case Event::EVENT_MOUSE_OUT:
    case Event::EVENT_MOUSE_OVER: {
      DisplayState oldthumb = impl_->thumb_state_;
      DisplayState oldleft = impl_->left_state_;
      DisplayState oldright = impl_->right_state_;
      impl_->ClearDisplayStates();
      if (c == COMPONENT_THUMB_BUTTON) {
        impl_->thumb_state_ = STATE_OVER;
      } else if (c == COMPONENT_UPRIGHT_BUTTON) {
        impl_->right_state_ = STATE_OVER;
      } else if (c == COMPONENT_DOWNLEFT_BUTTON) {
        impl_->left_state_ = STATE_OVER;
      }

      // Restore the down states, overwriting the over states if necessary.
      if (oldthumb == STATE_DOWN) {
        impl_->thumb_state_ = STATE_DOWN;
        // Special case, need to scroll.
        int v = impl_->GetValueFromLocation(event.GetX(), event.GetY());
        SetValue(v);
        break;
      } else if (oldright == STATE_DOWN) {
        impl_->right_state_ = STATE_DOWN;
      } else if (oldleft == STATE_DOWN) {
        impl_->left_state_ = STATE_DOWN;
      }

      bool redraw = (impl_->left_state_ != oldleft ||
          impl_->right_state_ != oldright ||
          impl_->thumb_state_ != oldthumb);
      if (redraw) {
        QueueDraw();
      }
      break;
    }

    case Event::EVENT_MOUSE_DOWN:
     if (event.GetButton() & MouseEvent::BUTTON_LEFT) {
       bool downleft = true, line = true;
       impl_->ClearDisplayStates();
       if (c == COMPONENT_THUMB_BUTTON) {
         impl_->thumb_state_ = STATE_DOWN;
         if (impl_->orientation_ == ORIENTATION_HORIZONTAL) {
           impl_->drag_delta_ = event.GetX() - compx;
         } else {
           impl_->drag_delta_ = event.GetY() - compy;
         }
         QueueDraw();
         break; // don't scroll, early exit
       } else if (c == COMPONENT_UPRIGHT_BUTTON) {
         impl_->right_state_ = STATE_DOWN;
         downleft = false; line = true;
       } else if (c == COMPONENT_UPRIGHT_BAR) {
         downleft = line = false;
       } else if (c == COMPONENT_DOWNLEFT_BUTTON) {
         impl_->left_state_ = STATE_DOWN;
         downleft = line = true;
       } else if (c == COMPONENT_DOWNLEFT_BAR) {
         downleft = true; line = false;
       }
       impl_->Scroll(downleft, line);
     }
     break;
    case Event::EVENT_MOUSE_UP:
     if (event.GetButton() & MouseEvent::BUTTON_LEFT) {
       DisplayState oldthumb = impl_->thumb_state_;
       DisplayState oldleft = impl_->left_state_;
       DisplayState oldright = impl_->right_state_;
       impl_->ClearDisplayStates();
       if (c == COMPONENT_THUMB_BUTTON) {
         impl_->thumb_state_ = STATE_OVER;
       } else if (c == COMPONENT_UPRIGHT_BUTTON) {
         impl_->right_state_ = STATE_OVER;
       } else if (c == COMPONENT_DOWNLEFT_BUTTON) {
         impl_->left_state_ = STATE_OVER;
       }
       bool redraw = (impl_->left_state_ != oldleft ||
           impl_->right_state_ != oldright ||
           impl_->thumb_state_ != oldthumb);
       if (redraw) {
         QueueDraw();
       }
     }
     break;
    case Event::EVENT_MOUSE_WHEEL: {
      impl_->accum_wheel_delta_ += event.GetWheelDeltaY();
      bool downleft;
      int delta = impl_->accum_wheel_delta_;
      if (delta > 0 && delta >= MouseEvent::kWheelDelta) {
        impl_->accum_wheel_delta_ -= MouseEvent::kWheelDelta;
        downleft = true;
      } else if (delta < 0 && -delta >= MouseEvent::kWheelDelta) {
        impl_->accum_wheel_delta_ += MouseEvent::kWheelDelta;
        downleft = false;
      }
      else {
        break; // don't scroll in this case
      }
      impl_->Scroll(downleft, true);
      break;
    }

    default:
      result = EVENT_RESULT_UNHANDLED;
      break;
  }
  return result;
}

Connection *ScrollBarElement::ConnectOnChangeEvent(Slot0<void> *slot) {
  return impl_->onchange_event_.Connect(slot);
}

bool ScrollBarElement::HasOpaqueBackground() const {
  return impl_->background_ && impl_->background_->IsFullyOpaque();
}


} // namespace ggadget
