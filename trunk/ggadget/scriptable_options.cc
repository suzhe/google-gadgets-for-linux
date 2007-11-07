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

class ScriptableOptions::Impl {
 public:
  Impl(OptionsInterface *options) : options_(options) {
  }

  void Add(const char *name, const JSONString &value) {
    options_->Add(name, Variant(value));
  }

  JSONString GetDefaultValue(const char *name) {
    Variant value = options_->GetDefaultValue(name);
    return value.type() == Variant::TYPE_JSON ?
           VariantValue<JSONString>()(value) : JSONString("");
  }

  void PutDefaultValue(const char *name, const JSONString &value) {
    options_->PutDefaultValue(name, Variant(value));
  }

  JSONString GetValue(const char *name) {
    Variant value = options_->GetValue(name);
    return value.type() == Variant::TYPE_JSON ?
           VariantValue<JSONString>()(value) : JSONString("");
  }

  void PutValue(const char *name, const JSONString &value) {
    options_->PutValue(name, Variant(value));
  }

  OptionsInterface *options_;
};

ScriptableOptions::ScriptableOptions(OptionsInterface *options)
    : impl_(new Impl(options)) {
  RegisterProperty("count",
                   NewSlot(options, &OptionsInterface::GetCount), NULL);
  // Partly support the deprecated "item" property.
  RegisterMethod("item", NewSlot(impl_, &Impl::GetValue));
  // Partly support the deprecated "defaultValue" property.
  RegisterMethod("defaultValue",
                 NewSlot(impl_, &Impl::GetDefaultValue));
  RegisterMethod("add", NewSlot(impl_, &Impl::Add));
  RegisterMethod("exists", NewSlot(options, &OptionsInterface::Exists));
  RegisterMethod("getDefaultValue",
                 NewSlot(options, &OptionsInterface::GetDefaultValue));
  RegisterMethod("getValue", NewSlot(impl_, &Impl::GetValue));
  RegisterMethod("putDefaultValue",
                 NewSlot(impl_, &Impl::PutDefaultValue));
  RegisterMethod("putValue", NewSlot(impl_, &Impl::PutValue));
  RegisterMethod("remove", NewSlot(options, &OptionsInterface::Remove));
  RegisterMethod("removeAll", NewSlot(options, &OptionsInterface::RemoveAll));

  // Register the "default" method, allowing this object be called directly
  // as a function.
  RegisterMethod("", NewSlot(impl_, &Impl::GetValue));

  // Disable the following for now, because it's not in the public API document.
  // SetDynamicPropertyHandler(NewSlot(impl_, &Impl::GetValue),
  //                           NewSlot(impl_, &Impl::PutValue));
}

ScriptableOptions::~ScriptableOptions() {
  delete impl_;
  impl_ = NULL;
}

} // namespace ggadget
