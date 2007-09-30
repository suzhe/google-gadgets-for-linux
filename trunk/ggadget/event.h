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

#ifndef GGADGET_EVENT_H__
#define GGADGET_EVENT_H__

namespace ggadget {

class ElementInterface;

/** 
 * Class for holding event information. There are several subclasses for 
 * events features in common.
 */
class Event {
 public:
  enum Type {
    EVENT_SIMPLE_RANGE_START = 0,
    EVENT_CANCEL,
    EVENT_CLOSE,
    EVENT_DOCK,
    EVENT_MINIMIZE,
    EVENT_OK,
    EVENT_OPEN,
    EVENT_POPIN,
    EVENT_POPOUT,
    EVENT_RESTORE,
    EVENT_SIZE,
    EVENT_UNDOCK,
    EVENT_FOCUS_IN,
    EVENT_FOCUS_OUT,
    EVENT_SIMPLE_RANGE_END,

    EVENT_MOUSE_RANGE_START = 10000,
    EVENT_MOUSE_DOWN,
    EVENT_MOUSE_UP,
    EVENT_MOUSE_CLICK,
    EVENT_MOUSE_DBLCLICK,
    EVENT_MOUSE_MOVE,
    EVENT_MOUSE_OUT,
    EVENT_MOUSE_OVER,
    EVENT_MOUSE_WHEEL,
    EVENT_MOUSE_RANGE_END,
    
    EVENT_KEY_RANGE_START = 20000,
    EVENT_KEY_DOWN,
    EVENT_KEY_UP,
    EVENT_KEY_PRESS,
    EVENT_KEY_RANGE_END,
    
    EVENT_DRAG_RANGE_START = 30000,
    EVENT_DRAG_DROP,
    EVENT_DRAG_OUT,
    EVENT_DRAG_OVER,
    EVENT_DRAG_RANGE_END,

    EVENT_TIMER_TICK = 40000,
    EVENT_SIZING,
    EVENT_OPTION_CHANGED,
  };
      
  explicit Event(Type t) : type_(t) { ASSERT(IsSimpleEvent()); }
  Event(const Event &e) : type_(e.type_) { ASSERT(IsSimpleEvent()); }

  Type GetType() const { return type_; }

  bool IsSimpleEvent() const { return type_ > EVENT_SIMPLE_RANGE_START &&
                                      type_ < EVENT_SIMPLE_RANGE_END; }
  bool IsMouseEvent() const { return type_ > EVENT_MOUSE_RANGE_START &&
                                     type_ < EVENT_MOUSE_RANGE_END; }
  bool IsKeyboardEvent() const { return type_ > EVENT_KEY_RANGE_START &&
                                        type_ < EVENT_KEY_RANGE_END; }
  bool IsDragEvent() const { return type_ > EVENT_DRAG_RANGE_START &&
                                    type_ < EVENT_DRAG_RANGE_END; }

 protected:
  Event(Type t, int dummy) : type_(t) { }

 private:
  Type type_; 
};

/**
 * Class representing a mouse event.
 */
class MouseEvent : public Event {
 public:
  enum Button {
    BUTTON_NONE = 0,
    BUTTON_LEFT = 1,
    BUTTON_MIDDLE = 4,
    BUTTON_RIGHT = 2,
    BUTTON_ALL = BUTTON_LEFT | BUTTON_MIDDLE | BUTTON_RIGHT,
  };

  MouseEvent(Type t, double x, double y, Button button, int wheel_delta)
      : Event(t, 0), x_(x), y_(y), button_(button), wheel_delta_(wheel_delta) { 
    ASSERT(IsMouseEvent());
  }

  MouseEvent(const MouseEvent &e)
      : Event(e.GetType(), 0),
        x_(e.x_), y_(e.y_), button_(e.button_), wheel_delta_(e.wheel_delta_) {
    ASSERT(IsMouseEvent());
  }

  double GetX() const { return x_; }
  double GetY() const { return y_; }

  void SetX(double x) { x_ = x; }
  void SetY(double y) { y_ = y; }

  Button GetButton() const { return button_; }
  void SetButton(Button button) { button_ = button; }

  int GetWheelDelta() const { return wheel_delta_; }
  void SetWheelDelta(int wheel_delta) { wheel_delta_ = wheel_delta; }

 private:
  double x_, y_;
  Button button_;
  int wheel_delta_;
};

/** 
 * Class representing a keyboard event.
 */
class KeyboardEvent : public Event {
 public:
  KeyboardEvent(Type t, int key_code) : Event(t, 0), key_code_(key_code) { 
    ASSERT(IsKeyboardEvent());
  };

  KeyboardEvent(const KeyboardEvent &e)
      : Event(e.GetType(), 0), key_code_(e.key_code_) {
    ASSERT(IsKeyboardEvent());
  }

  int GetKeyCode() const { return key_code_; }
  void SetKeyCode(int key_code) { key_code_ = key_code; }

 private:
  int key_code_;
};

/**
 * Class representing a drag & drop event.
 */
class DragEvent : public Event {
 public:
  DragEvent(Type t, double x, double y, const char **files)
      : Event(t, 0), x_(x), y_(y), files_(files) { 
    ASSERT(IsDragEvent());
  }

  DragEvent(const DragEvent &e)
      : Event(e.GetType(), 0), x_(e.x_), y_(e.y_), files_(e.files_) {
    ASSERT(IsDragEvent());
  }

  double GetX() const { return x_; }
  double GetY() const { return y_; }

  void SetX(double x) { x_ = x; }
  void SetY(double y) { y_ = y; }

  const char **GetFiles() const { return files_; }
  void GetFiles(const char **files) { files_ = files; }

 private:
  double x_, y_;
  const char **files_;
};

/** 
 * Class representing a timer event.
 */
class TimerEvent : public Event {
 public:
  TimerEvent(ElementInterface *target, void *data, uint64_t time) 
      : Event(EVENT_TIMER_TICK, 0), 
        target_(target), data_(data), time_(time), receive_more_(true) { 
  };

  /** Gets the target of this timer event. May be NULL if it is for the view. */
  ElementInterface *GetTarget() const { return target_; };
  void *GetData() const { return data_; };

  /** 
   * Returns the time of the event in microsecond units. This should only 
   * be used for relative time comparisons, to compute elapsed time.
   */
  uint64_t GetTimeStamp() const { return time_; };

  /** Sets that this timer should not be received anymore. */
  void StopReceivingMore() { receive_more_ = false; }
  /** Gets whether or not more events should be received. */
  bool GetReceiveMore() const { return receive_more_; };
  
 private:
  ElementInterface *target_;
  void *data_;
  uint64_t time_;
  bool receive_more_;
};

/**
 * Class representing a sizing event.
 */
class SizingEvent : public Event {
 public:
  SizingEvent(double width, double height)
      : Event(EVENT_SIZING, 0), width_(width), height_(height) { 
  }

  SizingEvent(const SizingEvent &e)
      : Event(EVENT_SIZING, 0), width_(e.width_), height_(e.height_) {
    ASSERT(e.GetType() == EVENT_SIZING);
  }

  double GetWidth() const { return width_; }
  double GetHeight() const { return height_; }

  void SetWidth(double width) { width_ = width; }
  void SetHeight(double height) { height_ = height; }

 private:
  double width_, height_;
};

/**
 * Class representing a sizing event.
 */
class OptionChangedEvent : public Event {
 public:
  OptionChangedEvent(const char *property_name)
      : Event(EVENT_OPTION_CHANGED, 0), property_name_(property_name) { 
  }

  OptionChangedEvent(const OptionChangedEvent &e)
      : Event(EVENT_OPTION_CHANGED, 0), property_name_(e.property_name_) {
    ASSERT(e.GetType() == EVENT_OPTION_CHANGED);
  }

  const char *GetPropertyName() const { return property_name_.c_str(); }
  void SetPropertyname(
      const char *property_name) { property_name_ = property_name; }

 private:
  std::string property_name_;
};

} // namespace ggadget

#endif // GGADGET_EVENT_H__
