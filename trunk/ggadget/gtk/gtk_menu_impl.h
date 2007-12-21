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

#ifndef GGADGET_GTK_GTK_MENU_IMPL_H__
#define GGADGET_GTK_GTK_MENU_IMPL_H__

#include <map>
#include <string>
#include <vector>
#include <gtk/gtk.h>
#include <ggadget/common.h>
#include <ggadget/menu_interface.h>

namespace ggadget {
namespace gtk {

/**
 * An implementation of @c MenuInterface for the simple gadget host.
 */
class GtkMenuImpl : public ggadget::MenuInterface {
 public:
  GtkMenuImpl(GtkMenu *gtk_menu);
  virtual ~GtkMenuImpl();

  virtual void AddItem(const char *item_text, int style,
                       ggadget::Slot1<void, const char *> *handler);
  virtual void SetItemStyle(const char *item_text, int style);
  virtual MenuInterface *AddPopup(const char *popup_text);

  GtkMenu *gtk_menu() { return gtk_menu_; }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(GtkMenuImpl);

  static void OnItemActivate(GtkMenuItem *menu_item, gpointer user_data);
  static void SetMenuItemStyle(GtkMenuItem *menu_item, int style);

  struct MenuItemInfo {
    std::string item_text;
    GtkMenuItem *gtk_menu_item;
    int style;
    ggadget::Slot1<void, const char *> *handler;
    GtkMenuImpl *submenu;
  };

  GtkMenu *gtk_menu_;
  typedef std::map<std::string, MenuItemInfo> ItemMap;
  ItemMap item_map_;
  static bool setting_style_;
};

} // namesapce gtk
} // namespace ggadget

#endif // GGADGET_GTK_GTK_MENU_IMPL_H__
