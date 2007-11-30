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

#ifndef GGADGET_COMBOBOX_ELEMENT_H__
#define GGADGET_COMBOBOX_ELEMENT_H__

#include <ggadget/listbox_element.h>

namespace ggadget {

class ComboBoxElement : public ListBoxElement {
 public:
  DEFINE_CLASS_ID(0x848a2f5e84144915, ListBoxElement);

  ComboBoxElement(BasicElement *parent, View *view, const char *name);
  virtual ~ComboBoxElement();

 public:
  enum Type {
    /** The default, editable control. */
    DROPDOWN,
    /** Uneditable. */
    DROPLIST,
  };

  /** Gets and sets whether the dropdown list is visible. */
  bool IsDroplistVisible() const;
  void SetDroplistVisible(bool visible);

  /**
   * Gets and sets the maximum # of items to show before scrollbar is displayed.
   */
  int GetMaxDroplistItems() const;
  void SetMaxDroplistItems(int max_droplist_items);


  /** Gets and sets the type of this combobox. */
  Type GetType() const;
  void SetType(Type type);

  /** Gets and sets the value of the edit area, only for dropdown mode. */
  const char *GetValue() const;
  void SetValue(const char *value);

 public:
  static BasicElement *CreateInstance(BasicElement *parent, View *view,
                                      const char *name);

 protected:
  virtual void DoDraw(CanvasInterface *canvas,
                      const CanvasInterface *children_canvas);
  virtual EventResult HandleMouseEvent(const MouseEvent &event);
  virtual EventResult HandleKeyEvent(const KeyboardEvent &event);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ComboBoxElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_COMBOBOX_ELEMENT_H__
