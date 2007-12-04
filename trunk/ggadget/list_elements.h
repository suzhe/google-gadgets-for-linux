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

#ifndef GGADGET_LIST_ELEMENTS_H__
#define GGADGET_LIST_ELEMENTS_H__

#include <ggadget/common.h>
#include <ggadget/event.h>
#include <ggadget/elements.h>
#include <ggadget/scriptable_helper.h>

namespace ggadget {

class ListBoxElement;
class ItemElement;
class Texture;

/**
 * ListElements is used for storing and managing items in a listbox or combobox.
 */
class ListElements : public Elements {
 public:
  DEFINE_CLASS_ID(0x32457b2f57414af6, Elements)

  ListElements(ElementFactoryInterface *factory, ListBoxElement *owner, 
               View *view);

  virtual ~ListElements();

 public: // ElementsInterface methods
   virtual ElementInterface *AppendElement(const char *tag_name,
                                           const char *name);
   virtual ElementInterface *InsertElement(const char *tag_name,
                                           const ElementInterface *before,
                                           const char *name);

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
  ItemElement *GetSelectedItem();
  const ItemElement *GetSelectedItem() const;
  void SetSelectedItem(ItemElement *item);

  /** 
   * AppendSelection differs from SetSelectedItem in that this method allows  
   * multiselect if it is enabled.
   * This method is not exposed to the script engine.
   */
  void AppendSelection(ItemElement *item);

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

  /** Gets or sets the background texture of the item under the mouse cursor. */
  Variant GetItemOverColor() const;
  const Texture *GetItemOverTexture() const;
  void SetItemOverColor(const Variant &color);

  /** Gets or sets the background texture of the selected item. */
  Variant GetItemSelectedColor() const;
  const Texture *GetItemSelectedTexture() const;
  void SetItemSelectedColor(const Variant &color);

  /** Gets and sets whether there are separator lines between the items. */
  bool HasItemSeparator() const;
  void SetItemSeparator(bool separator);

  /** Gets and sets whether the user can select multiple items. */
  bool IsMultiSelect() const;
  void SetMultiSelect(bool multiselect);

  virtual void OnParentWidthChange(double width);
  virtual void OnParentHeightChange(double height);

  virtual const CanvasInterface *Draw(bool *changed);

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(ListElements);
};

} // namespace ggadget

#endif // GGADGET_LIST_ELEMENTS_H__
