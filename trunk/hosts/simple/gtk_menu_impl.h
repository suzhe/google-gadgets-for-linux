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

#ifndef HOSTS_SIMPLE_GTK_MENU_IMPL_H__
#define HOSTS_SIMPLE_GTK_MENU_IMPL_H__

#include <map>
#include <vector>
#include <gtk/gtk.h>
#include <ggadget/ggadget.h>

/**
 * An implementation of @c MenuInterface for the simple gadget host.
 */
class GtkMenuImpl : public ggadget::MenuInterface {
 public:
  GtkMenuImpl(GtkMenu *menu);
  virtual ~GtkMenuImpl();

  virtual void AddItem(const char *item_text, int style,
                       ggadget::Slot1<void, const char *> *handler);
  virtual void SetItemStyle(const char *item_text, int style);
  virtual MenuInterface *AddPopup(const char *popup_text);

  GtkMenu *menu() { return menu_; }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(GtkMenuImpl);

  static void OnItemActivate(GtkMenuItem *menu_item, gpointer user_data);
  
  struct MenuItemInfo {
    std::string item_text;
    GtkWidget *menu_item;
    int style;
    ggadget::Slot1<void, const char *> *handler;
    GtkMenuImpl *submenu;
  };

  GtkMenu *menu_;
  typedef std::map<std::string, MenuItemInfo> ItemMap;
  ItemMap item_map_;
  static bool in_handler_;
};

#endif // HOSTS_SIMPLE_GTK_MENU_IMPL_H__
