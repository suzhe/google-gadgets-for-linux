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
#include "qt_menu.h"

namespace ggadget {
namespace qt {
#include "qt_menu.moc"

class QtMenu::Impl {
 public:
  Impl(QMenu *qmenu) : qt_menu_(qmenu) { }
  void AddItem(const char *item_text, int style,
      ggadget::Slot1<void, const char *> *handler) {
    QAction *action;
    if (style & MENU_ITEM_FLAG_SEPARATOR || !item_text || !*item_text) {
      action = qt_menu_->addSeparator();
    } else {
      action = qt_menu_->addAction(item_text);
      MenuItemInfo *info = new MenuItemInfo(item_text, handler, action);
      if (item_text) menu_items_[item_text] = info;
    }

    if (style & MENU_ITEM_FLAG_GRAYED)
      action->setDisabled(true);
    else
      action->setDisabled(false);

    if (style & MENU_ITEM_FLAG_CHECKED) {
      action->setCheckable(true);
      action->setChecked(true);
    } else {
      action->setChecked(false);
    }
  }

  void SetItemStyle(const char *item_text, int style) {
    std::map<std::string, MenuItemInfo*>::iterator iter;
    iter = menu_items_.find(item_text);
    if (iter == menu_items_.end()) return;
    QAction *action = iter->second->action_;

    if (style & MENU_ITEM_FLAG_GRAYED)
      action->setDisabled(true);
    else
      action->setDisabled(false);
    
    if (style & MENU_ITEM_FLAG_CHECKED)
      action->setChecked(true);
    else
      action->setChecked(false);
  }

  MenuInterface *AddPopup(const char *popup_text){
    std::string text_str(popup_text ? popup_text : "");
    QMenu *submenu = qt_menu_->addMenu(text_str.c_str());
    return new QtMenu(submenu);
  }

  QMenu *GetNativeMenu() { return qt_menu_; }
  QMenu *qt_menu_;
  std::map<std::string, MenuItemInfo*> menu_items_;
};
//bool QtMenu::setting_style_ = false;

QtMenu::QtMenu(QMenu *qmenu)
    : impl_(new Impl(qmenu)) {
}

QtMenu::~QtMenu() {
}

/*void QtMenu::SetMenuItemStyle(GtkMenuItem *menu_item, int style) {
  setting_style_ = true;
  gtk_widget_set_sensitive(GTK_WIDGET(menu_item),
      (style & ggadget::MenuInterface::MENU_ITEM_FLAG_GRAYED) == 0);
  if (GTK_IS_CHECK_MENU_ITEM(menu_item)) {
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
        (style & ggadget::MenuInterface::MENU_ITEM_FLAG_CHECKED) != 0);
  }
  setting_style_ = false; 
} */

// Windows version uses '&' as the mnemonic indicator, and this has been
// taken as the part of the Gadget API.
/* static std::string ConvertWindowsStyleMnemonics(const char *text) {
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
}*/

void QtMenu::AddItem(const char *item_text, int style,
                          ggadget::Slot1<void, const char *> *handler) {
  impl_->AddItem(item_text, style, handler);
}

void QtMenu::SetItemStyle(const char *item_text, int style) {
  impl_->SetItemStyle(item_text, style);
}

ggadget::MenuInterface *QtMenu::AddPopup(const char *popup_text) {
  return impl_->AddPopup(popup_text);
}

QMenu *QtMenu::GetNativeMenu() { 
  return impl_->GetNativeMenu();
}

} // namespace qt
} // namespace ggadget
