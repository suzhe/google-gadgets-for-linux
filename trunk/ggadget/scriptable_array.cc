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

#include "scriptable_array.h"

namespace ggadget {

class ScriptableArray::Impl {
 public:
  Impl(ScriptableArray *owner, Variant *array, size_t count, bool native_owned)
      : owner_(owner),
        array_(array), count_(count),
        native_owned_(native_owned),
        delete_from_script_(false) {
  }

  ScriptableArray *ToArray() { return owner_; }

  ScriptableArray *owner_;
  Variant *array_;
  size_t count_;
  bool native_owned_;
  bool delete_from_script_;
};

ScriptableArray::ScriptableArray(Variant *array, size_t count,
                                 bool native_owned)
    : impl_(new Impl(this, array, count, native_owned)) {
  RegisterConstant("count", count);
  RegisterMethod("item", NewSlot(this, &ScriptableArray::GetItem));
  // Simulates JavaScript array.
  RegisterConstant("length", count);
  SetArrayHandler(NewSlot(this, &ScriptableArray::GetItem), NULL);
  // Simulates VBArray.
  RegisterMethod("toArray", NewSlot(impl_, &Impl::ToArray));
}

ScriptableArray::~ScriptableArray() {
  ASSERT(impl_->native_owned_ || impl_->delete_from_script_);
  delete [] impl_->array_;
  delete impl_;
  impl_ = NULL;
}

size_t ScriptableArray::GetCount() const {
  return impl_->count_;
}

Variant ScriptableArray::GetItem(size_t index) const {
  return index < impl_->count_ ? impl_->array_[index] : Variant();
}

ScriptableInterface::OwnershipPolicy ScriptableArray::Attach() {
  return impl_->native_owned_ ? NATIVE_OWNED : OWNERSHIP_TRANSFERRABLE;
}

bool ScriptableArray::Detach() {
  if (!impl_->native_owned_) {
    impl_->delete_from_script_ = true;
    delete this;
    return true;
  }
  return false;
}

} // namespace ggadget
