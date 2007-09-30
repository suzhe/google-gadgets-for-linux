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

#include "scriptable_options.h"
#include "options_interface.h"

namespace ggadget {

ScriptableOptions::ScriptableOptions(OptionsInterface *options)
    : options_(options) {
  RegisterProperty("count",
                   NewSlot(options, &OptionsInterface::GetCount), NULL);
  // Partly support the deprecated "item" property.
  RegisterMethod("item", NewSlot(options, &OptionsInterface::GetValue));
  // Partly support the deprecated "defaultValue" property.
  RegisterMethod("defaultValue",
                 NewSlot(options, &OptionsInterface::GetDefaultValue));
  RegisterMethod("add", NewSlot(options, &OptionsInterface::Add));
  RegisterMethod("exists", NewSlot(options, &OptionsInterface::Exists));
  RegisterMethod("getDefaultValue",
                 NewSlot(options, &OptionsInterface::GetDefaultValue));
  RegisterMethod("getValue", NewSlot(options, &OptionsInterface::GetValue));
  RegisterMethod("putDefaultValue",
                 NewSlot(options, &OptionsInterface::PutDefaultValue));
  RegisterMethod("putValue", NewSlot(options, &OptionsInterface::PutValue));
  RegisterMethod("remove", NewSlot(options, &OptionsInterface::Remove));
  RegisterMethod("removeAll", NewSlot(options, &OptionsInterface::RemoveAll));
  SetDynamicPropertyHandler(NewSlot(options, &OptionsInterface::GetValue),
                            NewSlot(options, &OptionsInterface::PutValue));
}

ScriptableOptions::~ScriptableOptions() {
}

} // namespace ggadget
