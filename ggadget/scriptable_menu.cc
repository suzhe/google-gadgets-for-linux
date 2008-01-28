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

#include "menu_interface.h"
#include "scriptable_menu.h"
#include "slot.h"

namespace ggadget {

ScriptableMenu::ScriptableMenu(MenuInterface *menu)
    : menu_(menu) {
}

void ScriptableMenu::DoRegister() {
  RegisterMethod("AddItem", NewSlot(this, &ScriptableMenu::ScriptAddItem));
  RegisterMethod("SetItemStyle", NewSlot(menu_, &MenuInterface::SetItemStyle));
  RegisterMethod("AddPopup", NewSlot(this, &ScriptableMenu::ScriptAddPopup));
}

ScriptableMenu::~ScriptableMenu() {
  for (std::vector<ScriptableMenu *>::iterator it = submenus_.begin();
       it != submenus_.end(); ++it) {
    delete *it;
  }
  submenus_.clear();
}

void ScriptableMenu::ScriptAddItem(const char *item_text, int style,
                                   Slot *handler) {
  menu_->AddItem(item_text, style,
                 handler ? new SlotProxy1<void, const char *>(handler) : NULL);
}

ScriptableMenu *ScriptableMenu::ScriptAddPopup(const char *popup_text) {
  ScriptableMenu *submenu = new ScriptableMenu(menu_->AddPopup(popup_text));
  submenus_.push_back(submenu);
  return submenu;
}

} // namespace ggadget
