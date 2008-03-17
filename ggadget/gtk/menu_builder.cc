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

#include <ggadget/logger.h>
#include <ggadget/slot.h>
#include "menu_builder.h"

namespace ggadget {
namespace gtk {

static const char *kMenuItemTextTag = "menu-item-text";
static const char *kMenuItemCallbackTag = "menu-item-callback";
static const char *kMenuItemBuilderTag = "menu-item-builder";
static const char *kMenuItemNoCallbackTag = "menu-item-no-callback";

class MenuBuilder::Impl {
 public:
  Impl(GtkMenuShell *gtk_menu) : gtk_menu_(gtk_menu), item_added_(false) {
    ASSERT(GTK_IS_MENU_SHELL(gtk_menu_));
    g_object_ref(G_OBJECT(gtk_menu_));
  }
  ~Impl() {
    g_object_unref(G_OBJECT(gtk_menu_));
  }

  // Windows version uses '&' as the mnemonic indicator, and this has been
  // taken as the part of the Gadget API.
  static std::string ConvertWindowsStyleMnemonics(const char *text) {
    std::string result;
    if (text) {
      while (*text) {
        switch (*text) {
          case '&': result += '_'; break;
          case '_': result += "__";
          default: result += *text;
        }
        text++;
      }
    }
    return result;
  }

  void SetMenuItemStyle(GtkMenuItem *item, int style) {
    // Set a signature to disable callback, to avoid trigger handler when
    // setting checked state.
    g_object_set_data(G_OBJECT(item), kMenuItemNoCallbackTag,
                      static_cast<gpointer>(item));

    gtk_widget_set_sensitive(GTK_WIDGET(item),
        (style & ggadget::MenuInterface::MENU_ITEM_FLAG_GRAYED) == 0);

    if (GTK_IS_CHECK_MENU_ITEM(item)) {
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item),
          (style & ggadget::MenuInterface::MENU_ITEM_FLAG_CHECKED) != 0);
    }

    // Clear the signature.
    g_object_set_data(G_OBJECT(item), kMenuItemNoCallbackTag, NULL);
  }

  static void DestroyHandlerCallback(gpointer handler) {
    delete reinterpret_cast<ggadget::Slot1<void, const char *> *>(handler);
  }

  static void OnItemActivate(GtkMenuItem *item, gpointer data) {
    if (g_object_get_data(G_OBJECT(item), kMenuItemNoCallbackTag) != NULL)
      return;

    ggadget::Slot1<void, const char *> *handler =
        reinterpret_cast<ggadget::Slot1<void, const char *> *>(
            g_object_get_data(G_OBJECT(item), kMenuItemCallbackTag));
    const char *text =
        reinterpret_cast<const char *>(
            g_object_get_data(G_OBJECT(item), kMenuItemTextTag));

    if (handler) (*handler)(text);
  }

  void AddItem(const char *text, int style,
               ggadget::Slot1<void, const char *> *handler) {
    GtkMenuItem *item = NULL;

    if (style & MENU_ITEM_FLAG_SEPARATOR || !text || !*text) {
      item = GTK_MENU_ITEM(gtk_separator_menu_item_new());
    } else if (style & (MENU_ITEM_FLAG_CHECKABLE | MENU_ITEM_FLAG_CHECKED)) {
      item = GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic(
        ConvertWindowsStyleMnemonics(text).c_str()));
    } else {
      item = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(
        ConvertWindowsStyleMnemonics(text).c_str()));
    }

    gtk_widget_show(GTK_WIDGET(item));
    SetMenuItemStyle(item, style);

    if (text && *text)
      g_object_set_data_full(G_OBJECT(item), kMenuItemTextTag,
                             g_strdup(text), g_free);
    if (handler)
      g_object_set_data_full(G_OBJECT(item), kMenuItemCallbackTag,
                             handler, DestroyHandlerCallback);

    g_signal_connect(item, "activate", G_CALLBACK(OnItemActivate), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(gtk_menu_), GTK_WIDGET(item));
    item_added_ = true;
  }

  struct FindItemData {
    FindItemData(const char *item_text) : text(item_text), item(NULL) {}

    const char *text;
    GtkMenuItem *item;
  };

  static void FindItemCallback(GtkWidget *item, gpointer data) {
    const char *text = reinterpret_cast<const char *>(
        g_object_get_data(G_OBJECT(item), kMenuItemTextTag));
    FindItemData *item_data = reinterpret_cast<FindItemData *>(data);
    if (!item_data->item && text && strcmp(text, item_data->text) == 0)
      item_data->item = GTK_MENU_ITEM(item);
  }

  GtkMenuItem *FindItem(const char *item_text) {
    FindItemData data(item_text);
    gtk_container_foreach(GTK_CONTAINER(gtk_menu_), FindItemCallback,
                          reinterpret_cast<gpointer>(&data));
    return data.item;
  }

  void SetItemStyle(const char *text, int style) {
    GtkMenuItem *item = FindItem(text);
    if (item)
      SetMenuItemStyle(item, style);
  }

  static void DestroyMenuBuilderCallback(gpointer data) {
    delete reinterpret_cast<MenuBuilder *>(data);
  }

  MenuInterface *AddPopup(const char *text) {
    GtkMenuItem *item = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(
        ConvertWindowsStyleMnemonics(text).c_str()));
    gtk_widget_show(GTK_WIDGET(item));
    GtkMenu *popup = GTK_MENU(gtk_menu_new());
    gtk_widget_show(GTK_WIDGET(popup));
    MenuBuilder *submenu = new MenuBuilder(GTK_MENU_SHELL(popup));

    gtk_menu_item_set_submenu(item, GTK_WIDGET(popup));
    g_object_set_data_full(G_OBJECT(item), kMenuItemBuilderTag,
                           reinterpret_cast<gpointer>(submenu),
                           DestroyMenuBuilderCallback);
    if (text && *text)
      g_object_set_data_full(G_OBJECT(item), kMenuItemTextTag,
                             g_strdup(text), g_free);

    gtk_menu_shell_append(GTK_MENU_SHELL(gtk_menu_), GTK_WIDGET(item));
    item_added_ = true;
    return submenu;
  }

  GtkMenuShell *gtk_menu_;
  bool item_added_;
};

MenuBuilder::MenuBuilder(GtkMenuShell *gtk_menu)
    : impl_(new Impl(gtk_menu)) {
  DLOG("Create MenuBuilder.");
}

MenuBuilder::~MenuBuilder() {
  DLOG("Destroy MenuBuilder.");
  delete impl_;
  impl_ = NULL;
}

void MenuBuilder::AddItem(const char *item_text, int style,
                          ggadget::Slot1<void, const char *> *handler) {
  impl_->AddItem(item_text, style, handler);
}

void MenuBuilder::SetItemStyle(const char *item_text, int style) {
  impl_->SetItemStyle(item_text, style);
}

ggadget::MenuInterface *MenuBuilder::AddPopup(const char *popup_text) {
  return impl_->AddPopup(popup_text);
}

GtkMenuShell *MenuBuilder::GetGtkMenuShell() const {
  return impl_->gtk_menu_;
}

bool MenuBuilder::ItemAdded() const {
  return impl_->item_added_;
}

} // namespace gtk
} // namespace ggadget
