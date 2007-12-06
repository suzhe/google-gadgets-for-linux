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

#ifndef GGADGET_LISTBOX_ELEMENT_H__
#define GGADGET_LISTBOX_ELEMENT_H__

#include <ggadget/div_element.h>

namespace ggadget {

class ItemElement;
class Texture;

class ListBoxElement : public DivElement {
 public:
  DEFINE_CLASS_ID(0x7ed919e76c7e400a, DivElement);

  ListBoxElement(BasicElement *parent, View *view,
                 const char *tag_name, const char *name);
  virtual ~ListBoxElement();

 public:
  virtual Connection *ConnectEvent(const char *event_name, 
                                   Slot0<void> *handler);
  Connection *ConnectOnChangeEvent(Slot0<void> *slot);
  Connection *ConnectOnRedrawEvent(Slot0<void> *slot);

  void FireOnChangeEvent();

  virtual EventResult HandleKeyEvent(const KeyboardEvent &event);

  virtual void QueueDraw(); 

 public:
  static BasicElement *CreateInstance(BasicElement *parent, View *view,
                                      const char *name);

  friend class ComboBoxElement;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ListBoxElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_LISTBOX_ELEMENT_H__
