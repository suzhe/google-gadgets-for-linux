/*
  Copyright 2008 Google Inc.

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

#include "scriptable_function.h"
#include "small_object.h"

namespace ggadget {

class ScriptableFunction::Impl : public SmallObject<> {
 public:
  Impl(Slot *slot) : slot_(slot) {
  }
  ~Impl() {
    // slot_ is destroyed in ScriptableHelper.
  }

  Slot *slot_;
};

ScriptableFunction::ScriptableFunction(Slot *slot)
    : impl_(new Impl(slot)) {
}

ScriptableFunction::~ScriptableFunction() {
  delete impl_;
}

void ScriptableFunction::DoRegister() {
  RegisterMethod("", impl_->slot_);
}

} // namespace ggadget
