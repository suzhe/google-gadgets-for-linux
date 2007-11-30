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

class ListBoxElement : public DivElement {
 public:
  DEFINE_CLASS_ID(0x7ed919e76c7e400a, DivElement);

  ListBoxElement(BasicElement *parent, View *view,
                 const char *tag_name, const char *name);
  virtual ~ListBoxElement();

 public:
  /** Gets and sets item width in pixels or percentage. */
  Variant GetItemWidth() const;
  void SetItemWidth(const Variant &width);

  /** Gets and sets item height in pixels or percentage. */
  Variant GetItemHeight() const;
  void SetItemHeight(const Variant &height);

  /** Gets or sets the background texture of the item under the mouse cursor. */
  Variant GetItemOverColor() const;
  void SetItemOverColor(const Variant &color);

  /** Gets or sets the background texture of the selected item. */
  Variant GetItemSelectedColor() const;
  void SetItemSelectedColor(const Variant &color);

  /** Gets and sets whether there are separator lines between the items. */
  bool HasItemSeparator() const;
  void SetItemSeparator(bool separator);

  /** Gets and sets whether the user can select multiple items. */
  bool IsMultiSelect() const;
  void SetMultiSelect(bool multiSelect);

  /**
   * Gets and sets the current selected index.
   * If multiple items are selected, selectedIndex is the index of the first
   * selected item.
   * -1 means no item is selected.
   */
  int GetSelectedIndex() const;
  void SetSelectedIndex(int index);

  /** Gets and sets the current selected item. */
  ItemElement *GetSelectedItem() const;
  void SetSelectedItem(ItemElement *item);

  /** Unselects all items in the listbox. */
  void ClearSelection();

 public:
  static BasicElement *CreateInstance(BasicElement *parent, View *view,
                                      const char *name);

 protected:
  virtual void DoDraw(CanvasInterface *canvas,
                      const CanvasInterface *children_canvas);
  virtual EventResult HandleMouseEvent(const MouseEvent &event);
  virtual EventResult HandleKeyEvent(const KeyboardEvent &event);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ListBoxElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_LISTBOX_ELEMENT_H__
