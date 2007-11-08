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

#ifndef GGADGET_SCRIPTABLE_MENU_H__
#define GGADGET_SCRIPTABLE_MENU_H__

#include <ggadget/scriptable_helper.h>

namespace ggadget {

class MenuInterface;

class ScriptableMenu : public ScriptableHelper<ScriptableInterface> {
 public:
  DEFINE_CLASS_ID(0x95432249155845d6, ScriptableInterface)

  explicit ScriptableMenu(MenuInterface *menu);
  virtual ~ScriptableMenu();

  /** This wrapper's ownership is transferrable, but it doesn't owns the menu */
  virtual OwnershipPolicy Attach() { return OWNERSHIP_TRANSFERRABLE; }

  MenuInterface *GetMenu() const { return menu_; }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableMenu);

  void ScriptAddItem(const char *item_text, int style, Slot *handler);
  ScriptableMenu *ScriptAddPopup(const char *popup_text);

  MenuInterface *menu_;
};

} // namespace ggadget

#endif  // GGADGET_SCRIPTABLE_MENU_H__
