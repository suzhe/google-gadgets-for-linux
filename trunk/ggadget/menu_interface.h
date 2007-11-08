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

#ifndef GGADGET_MENU_INTERFACE_H__
#define GGADGET_MENU_INTERFACE_H__

namespace ggadget {

template <typename R, typename P1> class Slot1;

class MenuInterface {
 protected:
  virtual ~MenuInterface() { }

 public:
  enum MenuItemFlag {
    gddMenuItemFlagGrayed = 1,
    gddMenuItemFlagChecked = 8,
  };

  /**
   * Adds a single menu item.
   * @param item_text
   * @param style combination of <code>MenuItemFlag</code>s.
   * @param handler handles menu command.
   */
  virtual void AddItem(const char *item_text, int style,
                       Slot1<void, const char *> *handler) = 0;

  /**
   * Sets the style of the given menu item.
   * @param item_text
   * @param style combination of <code>MenuItemFlag</code>s.
   */
  virtual void SetItemStyle(const char *item_text, int style) = 0;

  /**
   * Adds a submenu/popup showing the given text.
   * @param popup_text
   * @return the menu object of the new popup menu.
   */
  virtual MenuInterface *AddPopup(const char *popup_text) = 0;
};

} // namespace ggadget

#endif  // GGADGET_MENU_INTERFACE_H__
