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
    EVENT_RANGE_START = 0,
    EVENT_FOCUS_IN,
    EVENT_FOCUS_OUT,
    EVENT_RANGE_END,
    
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
    
    EVENT_TIMER_TICK = 30000
  };
      
  Event(Type t) : type_(t) {}; 
   
  Type GetType() const { return type_; };
  
 private:
  Type type_; 
};

/** 
 * Class representing a mouse event.
 */
class MouseEvent : public Event {
 public:
  MouseEvent(Type t, double x, double y) : Event(t), x_(x), y_(y) { 
    ASSERT(t > EVENT_MOUSE_RANGE_START && t < EVENT_MOUSE_RANGE_END);
  };
 
  double GetX() const { return x_; };
  double GetY() const { return y_; };
  
 private:
  double x_, y_;
};

/** 
 * Class representing a keyboard event.
 */
class KeyboardEvent : public Event {
 public:
  KeyboardEvent(Type t) : Event(t) { 
    ASSERT(t > EVENT_KEY_RANGE_START && t < EVENT_KEY_RANGE_END);
  };  
};

/** 
 * Class representing a timer event.
 */
class TimerEvent : public Event {
 public:
  TimerEvent(ElementInterface *target, void *data) 
      : Event(EVENT_TIMER_TICK), 
        target_(target), data_(data), receive_more_(true) { 
  };  
  
  /** Gets the target of this timer event. May be NULL if it is for the view. */
  ElementInterface *GetTarget() const { return target_; };
  
  void *GetData() const { return data_; };
  
  /** Sets that this timer should not be received anymore. */
  void StopReceivingMore() { receive_more_ = false; }
  /** Gets whether or not more events should be received. */
  bool GetReceiveMore() const { return receive_more_; };
  
 private:
  ElementInterface *target_;
  void *data_;
  bool receive_more_;
};

} // namespace ggadget

#endif // GGADGET_EVENT_H__
