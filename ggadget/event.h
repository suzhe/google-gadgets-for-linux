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

/** 
 * Class for holding event information. There are several subclasses for 
 * events features in common.
 */
class Event {
 public:
  enum Type {
    EVENT_MOUSE_DOWN,
    EVENT_MOUSE_UP,
    EVENT_MOUSE_CLICK,
    EVENT_MOUSE_DBLCLICK,
    EVENT_MOUSE_MOVE,
    EVENT_MOUSE_OUT,
    EVENT_MOUSE_OVER,
    EVENT_MOUSE_WHEEL,
    EVENT_FOCUS_IN,
    EVENT_FOCUS_OUT,
    EVENT_KEY_DOWN,
    EVENT_KEY_UP,
    EVENT_KEY_PRESS    
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
  MouseEvent(Type t, double x, double y) : Event(t), x_(x), y_(y) {};
 
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
  KeyboardEvent(Type t) : Event(t) {};  
};

} // namespace ggadget

#endif // GGADGET_EVENT_H__
