/*
  Copyright 2008 Google Inc.

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

  ListBoxElement(View *view, const char *tag_name, const char *name);
  virtual ~ListBoxElement();

 protected:
  virtual void DoClassRegister();

 public:
  Connection *ConnectOnChangeEvent(Slot0<void> *slot);
  void ScrollToSelectedItem();
  virtual EventResult HandleKeyEvent(const KeyboardEvent &event);
  virtual void Layout();

 public:
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

  /**
   * AppendSelection differs from SetSelectedItem in that this method allows
   * multiselect if it is enabled.
   * This method is not exposed to the script engine.
   */
  void AppendSelection(ItemElement *item);

  /**
   * Selects all items in a range from the first selected item to the given
   * parameter if multiselect is enabled. Otherwise behaves like
   * SetSelectedItem.
   * This method is not exposed to the script engine.
   */
  void SelectRange(ItemElement *endpoint);

  /** Unselects all items in the listbox. */
  void ClearSelection();

  /** Gets and sets item width in pixels or percentage. */
  double GetItemPixelWidth() const;
  Variant GetItemWidth() const;
  void SetItemWidth(const Variant &width);

  /** Gets and sets item height in pixels or percentage. */
  double GetItemPixelHeight() const;
  Variant GetItemHeight() const;
  void SetItemHeight(const Variant &height);

  /**
   * Gets or sets the background texture of the item under the mouse cursor.
   */
  Variant GetItemOverColor() const;
  const Texture *GetItemOverTexture() const;
  void SetItemOverColor(const Variant &color);

  /** Gets or sets the background texture of the selected item. */
  Variant GetItemSelectedColor() const;
  const Texture *GetItemSelectedTexture() const;
  void SetItemSelectedColor(const Variant &color);

  /** Gets or sets the texture of the item separator. */
  Variant GetItemSeparatorColor() const;
  const Texture *GetItemSeparatorTexture() const;
  void SetItemSeparatorColor(const Variant &color);

  /** Gets and sets whether there are separator lines between the items. */
  bool HasItemSeparator() const;
  void SetItemSeparator(bool separator);

  /** Gets and sets whether the user can select multiple items. */
  bool IsMultiSelect() const;
  void SetMultiSelect(bool multiselect);

  /**
   * Creates an Item element with a single Label child with the specified text.
   * @return true on success, false otherwise.
   */
  bool AppendString(const char *str);

  /**
   * Creates an Item element with a single Label child with the specified text,
   * at the specified index.
   * @return true on success, false otherwise.
   */
  bool InsertStringAt(const char *str, size_t index);

  /**
   * Searches for the lowest-indexed Item element that has one Label child
   * with the specified text, and remove the element if found.
   */
  void RemoveString(const char *str);

  /**
   * Searches for the lowest-indexed Item element that has one Label child
   * with the specified text,
   */
  ItemElement *FindItemByString(const char *str);
  const ItemElement *FindItemByString(const char *str) const;

 public:
  static BasicElement *CreateInstance(View *view, const char *name);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ListBoxElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_LISTBOX_ELEMENT_H__
