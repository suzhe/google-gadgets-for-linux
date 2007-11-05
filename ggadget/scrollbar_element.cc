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
#include "image.h"
#include "string_utils.h"
#include "event.h"
#include "scriptable_event.h"
#include "view_interface.h"
#include "event.h"
#include "math_utils.h"
#include "gadget_consts.h"
#include "slot.h"

namespace ggadget {

static const char *const kOnChangeEvent = "onchange";

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
  Impl(ScrollBarElement *owner) : owner_(owner),
           left_state_(STATE_NORMAL), right_state_(STATE_NORMAL), 
           thumb_state_(STATE_NORMAL),
           // The values below are the default ones in Windows.
           min_(0), max_(100), value_(0), pagestep_(10), linestep_(1),
           accum_wheel_delta_(0), drag_delta_(0.), 
           // Windows default to horizontal for orientation, 
           // but puzzlingly use vertical images as default.
           orientation_(ORIENTATION_VERTICAL) {
    left_[STATE_NORMAL] = 
      owner->GetView()->LoadImageFromGlobal(kScrollDefaultLeft, false);
    left_[STATE_DOWN] = 
      owner->GetView()->LoadImageFromGlobal(kScrollDefaultLeftDown, false);
    left_[STATE_OVER] = 
      owner->GetView()->LoadImageFromGlobal(kScrollDefaultLeftOver, false);

    right_[STATE_NORMAL] = 
      owner->GetView()->LoadImageFromGlobal(kScrollDefaultRight, false);
    right_[STATE_DOWN] = 
      owner->GetView()->LoadImageFromGlobal(kScrollDefaultRightDown, false);
    right_[STATE_OVER] = 
      owner->GetView()->LoadImageFromGlobal(kScrollDefaultRightOver, false);  

    thumb_[STATE_NORMAL] = 
      owner->GetView()->LoadImageFromGlobal(kScrollDefaultThumb, false);
    thumb_[STATE_DOWN] = 
      owner->GetView()->LoadImageFromGlobal(kScrollDefaultThumbDown, false);
    thumb_[STATE_OVER] = 
      owner->GetView()->LoadImageFromGlobal(kScrollDefaultThumbOver, false);  

    background_ = owner->GetView()->LoadImageFromGlobal(kScrollDefaultBackground, 
                                                        false);
  }
  ~Impl() {
    for (int i = STATE_NORMAL; i < STATE_COUNT; i++) {
      delete left_[i];
      left_[i] = NULL;

      delete right_[i];
      right_[i] = NULL;

      delete thumb_[i];
      thumb_[i] = NULL;
    }

    delete background_;
    background_ = NULL;
  }

  ScrollBarElement *owner_;
  DisplayState left_state_, right_state_, thumb_state_;
  std::string left_src_[STATE_COUNT], right_src_[STATE_COUNT], 
              thumb_src_[STATE_COUNT], background_src_;
  Image *left_[STATE_COUNT], *right_[STATE_COUNT], *thumb_[STATE_COUNT], 
        *background_;
  int min_, max_, value_, pagestep_, linestep_;
  int accum_wheel_delta_;
  double drag_delta_;
  Orientation orientation_;
  EventSignal onchange_event_;

  void ClearDisplayStates() {
    left_state_ = STATE_NORMAL;
    right_state_ = STATE_NORMAL;
    thumb_state_ = STATE_NORMAL;
  }

  void GetButtonLocation(bool downleft, 
                         double *x, double *y, double *w, double *h) {
    Image *img = downleft ? left_[left_state_] : right_[right_state_];
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
    Image *img = thumb_[thumb_state_];
    double imgw =  img ? img->GetWidth() : 0;
    double imgh =  img ? img->GetHeight() : 0;

    double position = static_cast<double>(value_ - min_) / (max_ - min_);
    if (max_ == min_) { // prevent overflow
      position = 0;      
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

    Image *img = thumb_[thumb_state_];
    int delta = max_ - min_;
    double position, denominator;
    if (orientation_ == ORIENTATION_HORIZONTAL) {
      double imgw =  img ? img->GetWidth() : 0;      
      lx += lw;
      denominator = rx - imgw - lx;
      position = delta * (x - lx - drag_delta_) / denominator;
    } else {
      double imgh =  img ? img->GetHeight() : 0;
      ly += lh;
      denominator = ry - imgh - ly;
      position = delta * (y - ly - drag_delta_) / denominator;
    }
    if (denominator == 0) { // prevent overflow
      position = 0;
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
      Event event(Event::EVENT_CHANGE);
      ScriptableEvent s_event(&event, owner_, 0, 0);
      owner_->GetView()->FireEvent(&s_event, onchange_event_);
      owner_->QueueDraw();
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
};

ScrollBarElement::ScrollBarElement(ElementInterface *parent,
                       ViewInterface *view,
                       const char *name)
    : BasicElement(parent, view, "scrollbar", name, false),
      impl_(new Impl(this)) {
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
}

void ScrollBarElement::DoDraw(CanvasInterface *canvas,
                           const CanvasInterface *children_canvas) {
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

const char *ScrollBarElement::GetBackground() const {
  return impl_->background_src_.c_str();
}

void ScrollBarElement::SetBackground(const char *img) {
  if (AssignIfDiffer(img, &impl_->background_src_)) {
    delete impl_->background_;
    if (impl_->background_src_.empty()) {
      impl_->background_ = GetView()->LoadImageFromGlobal(kScrollDefaultBackground, 
                                                          false);
    } else {
      impl_->background_ = GetView()->LoadImage(img, false);
    }
    QueueDraw();
  }
}

const char *ScrollBarElement::GetLeftDownImage() const {
  return impl_->left_src_[STATE_DOWN].c_str();
}

void ScrollBarElement::SetLeftDownImage(const char *img) {
  if (AssignIfDiffer(img, &impl_->left_src_[STATE_DOWN])) {
    delete impl_->left_[STATE_DOWN];
    if (impl_->left_src_[STATE_DOWN].empty()) {
      impl_->left_[STATE_DOWN] = 
        GetView()->LoadImageFromGlobal(kScrollDefaultLeftDown, false);
    } else {    
      impl_->left_[STATE_DOWN] = GetView()->LoadImage(img, false);
    }
    if (impl_->left_state_ == STATE_DOWN) {
      QueueDraw();
    }
  }
}

const char *ScrollBarElement::GetLeftImage() const {
  return impl_->left_src_[STATE_NORMAL].c_str();
}

void ScrollBarElement::SetLeftImage(const char *img) {
  if (AssignIfDiffer(img, &impl_->left_src_[STATE_NORMAL])) {
    delete impl_->left_[STATE_NORMAL];
    if (impl_->left_src_[STATE_NORMAL].empty()) {
      impl_->left_[STATE_NORMAL] = 
        GetView()->LoadImageFromGlobal(kScrollDefaultLeft, false);
    } else {        
      impl_->left_[STATE_NORMAL] = GetView()->LoadImage(img, false);
    }
    if (impl_->left_state_ == STATE_NORMAL) {
      QueueDraw();
    }
  }
}

const char *ScrollBarElement::GetLeftOverImage() const {
  return impl_->left_src_[STATE_OVER].c_str();
}

void ScrollBarElement::SetLeftOverImage(const char *img) {
  if (AssignIfDiffer(img, &impl_->left_src_[STATE_OVER])) {
    delete impl_->left_[STATE_OVER];
    if (impl_->left_src_[STATE_OVER].empty()) {
      impl_->left_[STATE_OVER] = 
        GetView()->LoadImageFromGlobal(kScrollDefaultLeftOver, false);
    } else {        
      impl_->left_[STATE_OVER] = GetView()->LoadImage(img, false);
    }
    if (impl_->left_state_ == STATE_OVER) {
      QueueDraw();
    }
  }
}

const char *ScrollBarElement::GetRightDownImage() const {
  return impl_->right_src_[STATE_DOWN].c_str();
}

void ScrollBarElement::SetRightDownImage(const char *img) {
  if (AssignIfDiffer(img, &impl_->right_src_[STATE_DOWN])) {
    delete impl_->right_[STATE_DOWN];
    if (impl_->right_src_[STATE_DOWN].empty()) {
      impl_->right_[STATE_DOWN] = 
        GetView()->LoadImageFromGlobal(kScrollDefaultRightDown, false);
    } else {        
      impl_->right_[STATE_DOWN] = GetView()->LoadImage(img, false);
    }
    if (impl_->right_state_ == STATE_DOWN) {
      QueueDraw();
    }
  }
}

const char *ScrollBarElement::GetRightImage() const {
  return impl_->right_src_[STATE_NORMAL].c_str();
}

void ScrollBarElement::SetRightImage(const char *img) {
  if (AssignIfDiffer(img, &impl_->right_src_[STATE_NORMAL])) {
    delete impl_->right_[STATE_NORMAL];
    if (impl_->right_src_[STATE_NORMAL].empty()) {
      impl_->right_[STATE_NORMAL] = 
        GetView()->LoadImageFromGlobal(kScrollDefaultRight, false);
    } else {        
      impl_->right_[STATE_NORMAL] = GetView()->LoadImage(img, false);
    }
    if (impl_->right_state_ == STATE_NORMAL) {
      QueueDraw();
    }
  }
}

const char *ScrollBarElement::GetRightOverImage() const {
  return impl_->right_src_[STATE_OVER].c_str();
}

void ScrollBarElement::SetRightOverImage(const char *img) {
  if (AssignIfDiffer(img, &impl_->right_src_[STATE_OVER])) {
    delete impl_->right_[STATE_OVER];
    if (impl_->right_src_[STATE_OVER].empty()) {
      impl_->right_[STATE_OVER] = 
        GetView()->LoadImageFromGlobal(kScrollDefaultRightOver, false);  
    } else {        
      impl_->right_[STATE_OVER] = GetView()->LoadImage(img, false);
    }
    if (impl_->right_state_ == STATE_OVER) {
      QueueDraw();
    }
  }
}

const char *ScrollBarElement::GetThumbDownImage() const {
  return impl_->thumb_src_[STATE_DOWN].c_str();
}

void ScrollBarElement::SetThumbDownImage(const char *img) {
  if (AssignIfDiffer(img, &impl_->thumb_src_[STATE_DOWN])) {
    delete impl_->thumb_[STATE_DOWN];
    if (impl_->thumb_src_[STATE_DOWN].empty()) {     
      impl_->thumb_[STATE_DOWN] = 
        GetView()->LoadImageFromGlobal(kScrollDefaultThumbDown, false);
    } else {        
      impl_->thumb_[STATE_DOWN] = GetView()->LoadImage(img, false);
    }
    if (impl_->thumb_state_ == STATE_DOWN) {
      QueueDraw();
    }
  }
}

const char *ScrollBarElement::GetThumbImage() const {
  return impl_->thumb_src_[STATE_NORMAL].c_str();
}

void ScrollBarElement::SetThumbImage(const char *img) {
  if (AssignIfDiffer(img, &impl_->thumb_src_[STATE_NORMAL])) {
    delete impl_->thumb_[STATE_NORMAL];
    if (impl_->thumb_src_[STATE_NORMAL].empty()) {     
      impl_->thumb_[STATE_NORMAL] = 
        GetView()->LoadImageFromGlobal(kScrollDefaultThumb, false);
    } else {
      impl_->thumb_[STATE_NORMAL] = GetView()->LoadImage(img, false);
    }
    if (impl_->thumb_state_ == STATE_NORMAL) {
      QueueDraw();
    }
  }
}

const char *ScrollBarElement::GetThumbOverImage() const {
  return impl_->thumb_src_[STATE_OVER].c_str();
}

void ScrollBarElement::SetThumbOverImage(const char *img) {
  if (AssignIfDiffer(img, &impl_->thumb_src_[STATE_OVER])) {
    delete impl_->thumb_[STATE_OVER];
    if (impl_->thumb_src_[STATE_OVER].empty()) {
      impl_->thumb_[STATE_OVER] = 
        GetView()->LoadImageFromGlobal(kScrollDefaultThumbOver, false);  
    } else {    
      impl_->thumb_[STATE_OVER] = GetView()->LoadImage(img, false);
    }
    if (impl_->thumb_state_ == STATE_OVER) {
      QueueDraw();
    }
  }
}
ElementInterface *ScrollBarElement::CreateInstance(ElementInterface *parent,
                                             ViewInterface *view,
                                             const char *name) {
  return new ScrollBarElement(parent, view, name);
}

bool ScrollBarElement::OnMouseEvent(MouseEvent *event, bool direct,
                                    ElementInterface **fired_element) {
  bool result = BasicElement::OnMouseEvent(event, direct, fired_element);

  // Handle the event only when the event is fired and not canceled.
  if (*fired_element && result) {
    ASSERT(*fired_element == this);
    double compx = .0, compy = .0;
    ScrollBarComponent c = impl_->GetComponentFromPosition(event->GetX(), 
                                                           event->GetY(),
                                                           &compx, &compy);
    // Resolve in opposite order as drawn: thumb, right, left.
    switch (event->GetType()) {
      case Event::EVENT_MOUSE_DOWN: 
        {
          bool downleft = true, line = true;
          impl_->ClearDisplayStates();       
          if (c == COMPONENT_THUMB_BUTTON) {
            impl_->thumb_state_ = STATE_DOWN;
            if (impl_->orientation_ == ORIENTATION_HORIZONTAL) {
              impl_->drag_delta_ = event->GetX() - compx;
            } else {
              impl_->drag_delta_ = event->GetY() - compy;
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
        {
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
      case Event::EVENT_MOUSE_MOVE:
      case Event::EVENT_MOUSE_OUT:
      case Event::EVENT_MOUSE_OVER:
        {
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
            int v = impl_->GetValueFromLocation(event->GetX(), event->GetY());
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
        }
        break;
      case Event::EVENT_MOUSE_WHEEL:
        {
          impl_->accum_wheel_delta_ += event->GetWheelDelta();        
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
        }
        break;
      default:
        break;
    }; 
  }

  return result;
}

void ScrollBarElement::ConnectOnChangeEvent(Slot0<void> *slot) {
  impl_->onchange_event_.Connect(slot);
}

} // namespace ggadget
