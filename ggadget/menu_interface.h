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
    MENU_ITEM_FLAG_GRAYED = 1,
    MENU_ITEM_FLAG_CHECKED = 8
  };

  enum MenuItemPriority {
    /** For menu items added by client code, like elements or javascript. */
    MENU_ITEM_PRI_CLIENT = 0,
    /** For menu items added by view decorator. */
    MENU_ITEM_PRI_DECORATOR = 10,
    /** For menu items added by host. */
    MENU_ITEM_PRI_HOST = 20,
    /** For menu items added by Gadget. */
    MENU_ITEM_PRI_GADGET = 30
  };

  /**
   * Adds a single menu item. If @a item_text is blank or NULL, a menu
   * separator will be added.
   *
   * @param item_text the text displayed in the menu item. '&'s act as hotkey
   *     indicator. If it's blank or NULL, style is automatically treated as
   *     @c MENU_ITEM_FLAG_SEPARATOR.
   * @param style combination of <code>MenuItemFlag</code>s.
   * @param handler handles menu command.
   * @param priority Priority of the menu item, item with smaller priority will
   *      be placed to higher position in the menu. Must be >= 0.
   *      0-9 is reserved for menu items added by Element and JavaScript.
   *      10-19 is reserved for menu items added by View Decorator.
   *      20-29 is reserved for menu items added by host.
   *      30-39 is reserved for menu items added by Gadget.
   */
  virtual void AddItem(const char *item_text, int style,
                       Slot1<void, const char *> *handler, int priority) = 0;

  /**
   * Sets the style of the given menu item.
   * @param item_text
   * @param style combination of <code>MenuItemFlag</code>s.
   */
  virtual void SetItemStyle(const char *item_text, int style) = 0;

  /**
   * Adds a submenu/popup showing the given text.
   * @param popup_text
   * @param style combination of <code>MenuItemFlag</code>s.
   * @param priority of the popup menu item.
   * @return the menu object of the new popup menu.
   */
  virtual MenuInterface *AddPopup(const char *popup_text, int priority) = 0;
};

} // namespace ggadget

#endif  // GGADGET_MENU_INTERFACE_H__
