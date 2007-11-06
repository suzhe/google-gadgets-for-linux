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

#ifndef GGADGET_JS_FUNCTION_SLOT_H__
#define GGADGET_JS_FUNCTION_SLOT_H__

#include <jsapi.h>
#include <ggadget/common.h>
#include <ggadget/slot.h>

namespace ggadget {
namespace internal {

/**
 * A Slot that wraps a JavaScript function object.
 */
class JSFunctionSlot : public Slot {
 public:
  JSFunctionSlot(const Slot *prototype, JSContext *context, jsval function_val);
  virtual ~JSFunctionSlot();

  virtual Variant Call(int argc, Variant argv[]) const;

  virtual bool HasMetadata() const { return prototype_ != NULL; }
  virtual Variant::Type GetReturnType() const {
    return prototype_ ? prototype_->GetReturnType() : Variant::TYPE_VOID;
  }
  virtual int GetArgCount() const {
    return prototype_ ? prototype_->GetArgCount() : 0;
  }
  virtual const Variant::Type *GetArgTypes() const {
    return prototype_ ? prototype_->GetArgTypes() : NULL;
  }
  virtual bool operator==(const Slot &another) const {
    return function_val_ ==
           down_cast<const JSFunctionSlot *>(&another)->function_val_;
  }

  /**
   * Add a reference from obj to the function object to prevent it from being
   * GC'ed while it is being hold by a C++ object.
   */
  void SetReferenceFrom(JSObject *obj);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(JSFunctionSlot);

  const Slot *prototype_;
  JSContext *context_;
  jsval function_val_;
  JSObject *reference_from_;
};

} // namespace internal
} // namespace ggadget

#endif  // GGADGET_JS_FUNCTION_SLOT_H__
