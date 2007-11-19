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

#include "gtk_menu_impl.h"

bool GtkMenuImpl::setting_style_ = false;

GtkMenuImpl::GtkMenuImpl(GtkMenu *menu) : menu_(menu) {
}

GtkMenuImpl::~GtkMenuImpl() {
  for (ItemMap::iterator it = item_map_.begin(); it != item_map_.end(); ++it) {
    delete it->second.handler;
    delete it->second.submenu;
  }
  item_map_.clear();
}

void GtkMenuImpl::SetMenuItemStyle(GtkMenuItem *menu_item, int style) {
  setting_style_ = true;
  gtk_widget_set_sensitive(GTK_WIDGET(menu_item),
      (style & ggadget::MenuInterface::MENU_ITEM_FLAG_GRAYED) == 0);
  if (GTK_IS_CHECK_MENU_ITEM(menu_item)) {
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
        (style & ggadget::MenuInterface::MENU_ITEM_FLAG_CHECKED) != 0);
  }
  setting_style_ = false;
}

void GtkMenuImpl::AddItem(const char *item_text, int style,
                          ggadget::Slot1<void, const char *> *handler) {
  GtkMenuItem *item;
  if (style & MENU_ITEM_FLAG_SEPARATOR)
    item = GTK_MENU_ITEM(gtk_separator_menu_item_new());
  else
    item = GTK_MENU_ITEM(gtk_check_menu_item_new_with_label(item_text));

  SetMenuItemStyle(item, style);
  std::string text_str(item_text);
  MenuItemInfo menu_item_info = { text_str, item, style, handler, NULL };
  // Here if there is already a submenu with the same name, the submenu leaks.
  item_map_[text_str] = menu_item_info;
  g_signal_connect(item, "activate", G_CALLBACK(OnItemActivate),
                   &item_map_[text_str]);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_), GTK_WIDGET(item));
}

void GtkMenuImpl::SetItemStyle(const char *item_text, int style) {
  ItemMap::const_iterator it = item_map_.find(item_text);
  if (it != item_map_.end())
    SetMenuItemStyle(it->second.menu_item, style);
}

ggadget::MenuInterface *GtkMenuImpl::AddPopup(const char *popup_text) {
  GtkMenuItem *item = GTK_MENU_ITEM(gtk_menu_item_new_with_label(popup_text));
  GtkMenu *popup = GTK_MENU(gtk_menu_new());
  gtk_menu_item_set_submenu(item, GTK_WIDGET(popup));
  GtkMenuImpl *submenu = new GtkMenuImpl(popup);

  std::string text_str(popup_text);
  struct MenuItemInfo menu_item_info = { text_str, item, 0, NULL, submenu };
  item_map_[text_str] = menu_item_info;
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_), GTK_WIDGET(item));
  return submenu;
}

void GtkMenuImpl::OnItemActivate(GtkMenuItem *menu_item, gpointer user_data) {
  // Don't handle events triggered by SetItemStyle.
  if (setting_style_)
    return;

  MenuItemInfo *menu_item_info = static_cast<MenuItemInfo *>(user_data);
  ASSERT(GTK_MENU_ITEM(menu_item_info->menu_item) == menu_item);
  if (menu_item_info->handler) {
    DLOG("Call menu item handler: %s", menu_item_info->item_text.c_str());
    (*menu_item_info->handler)(menu_item_info->item_text.c_str());
  }
}
