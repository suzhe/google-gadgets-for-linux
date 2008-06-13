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

#ifndef GGADGET_SMJS_JS_FUNCTION_SLOT_H__
#define GGADGET_SMJS_JS_FUNCTION_SLOT_H__

#include <map>
#include <string>
#include <jsapi.h>
#include <ggadget/common.h>
#include <ggadget/slot.h>

namespace ggadget {
namespace smjs {

class NativeJSWrapper;

/**
 * A Slot that wraps a JavaScript function object.
 */
class JSFunctionSlot : public Slot {
 public:
  JSFunctionSlot(const Slot *prototype, JSContext *context,
                 NativeJSWrapper *owner, JSObject *function);
  virtual ~JSFunctionSlot();

  virtual ResultVariant Call(int argc, const Variant argv[]) const;

  virtual bool HasMetadata() const { return prototype_ != NULL; }
  virtual Variant::Type GetReturnType() const {
    return prototype_ ? prototype_->GetReturnType() : Variant::TYPE_VARIANT;
  }
  virtual int GetArgCount() const {
    return prototype_ ? prototype_->GetArgCount() : 0;
  }
  virtual const Variant::Type *GetArgTypes() const {
    return prototype_ ? prototype_->GetArgTypes() : NULL;
  }
  virtual bool operator==(const Slot &another) const {
    return function_ ==
           down_cast<const JSFunctionSlot *>(&another)->function_;
  }

  JSContext *js_context() const { return context_; }
  JSObject *js_function() const { return function_; }
  std::string function_info() const { return function_info_; }

  /** Called by the owner to mark this object is reachable from GC roots. */
  void Mark();
  /** Called by the owner when the owner is about to be finalized. */
  void Finalize();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(JSFunctionSlot);

  JSBool RemoveReferenceFrom();

  const Slot *prototype_;
  JSContext *context_;
  NativeJSWrapper *owner_;
  JSObject *function_;
  std::string function_info_;
};

} // namespace smjs
} // namespace ggadget

#endif  // GGADGET_SMJS_JS_FUNCTION_SLOT_H__
