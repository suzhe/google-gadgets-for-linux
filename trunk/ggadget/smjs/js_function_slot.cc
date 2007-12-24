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

#include <ggadget/scoped_ptr.h>
#include <ggadget/string_utils.h>
#include "js_function_slot.h"
#include "converter.h"
#include "js_script_context.h"
#include "native_js_wrapper.h"

namespace ggadget {
namespace smjs {

JSFunctionSlot::JSFunctionSlot(const Slot *prototype,
                               JSContext *context,
                               NativeJSWrapper *owner,
                               JSObject *function)
    : prototype_(prototype),
      context_(context),
      owner_(owner),
      function_(function) {
  ASSERT(function &&
         JS_TypeOfValue(context, OBJECT_TO_JSVAL(function)) == JSTYPE_FUNCTION);
  // Because the function may have a indirect reference to the owner through
  // the closure, we can't simply add the function to root, otherwise there
  // may be circular references if the native object's ownership is shared:
  //     native object =C++=> this slot =C++=> js function =JS=>
  //     closure =JS=> js wrapper object(owner) =C++=> native object.
  // This circle prevents the wrapper object and the function from being GC'ed.
  // Break the circle by letting the owner manage this object.
  if (owner)
    owner->AddJSFunctionSlot(this);

  int lineno;
  JSScriptContext::GetCurrentFileAndLine(context, &function_info_, &lineno);
  function_info_ += StringPrintf(":%d", lineno);
}

JSFunctionSlot::~JSFunctionSlot() {
  if (owner_)
    owner_->RemoveJSFunctionSlot(this);
}

Variant JSFunctionSlot::Call(int argc, const Variant argv[]) const {
  Variant return_value(GetReturnType());
  if (JS_IsExceptionPending(context_))
    return return_value;

  if (!function_) {
    JS_ReportError(context_, "Finalized JavaScript function still be called");
    return return_value;
  }

  AutoLocalRootScope local_root_scope(context_);
  if (!local_root_scope.good())
    return return_value;

  scoped_array<jsval> js_args;
  if (argc > 0) {
    js_args.reset(new jsval[argc]);
    for (int i = 0; i < argc; i++) {
      if (!ConvertNativeToJS(context_, argv[i], &js_args[i])) {
        JS_ReportError(context_, "Failed to convert argument %d(%s) to jsval",
                       i, argv[i].Print().c_str());
        return return_value;
      }
    }
  }

  jsval rval;
  if (JS_CallFunctionValue(context_, NULL, OBJECT_TO_JSVAL(function_),
                           argc, js_args.get(), &rval) &&
      !ConvertJSToNative(context_, NULL, return_value, rval, &return_value)) {
    JS_ReportError(context_,
                   "Failed to convert JS function return value(%s) to native",
                   PrintJSValue(context_, rval).c_str());
  }
  return return_value;
}

void JSFunctionSlot::Mark() {
  JS_MarkGCThing(context_, function_, "JSFunctionSlot", NULL);
}

void JSFunctionSlot::Finalize() {
  function_ = NULL;
}

} // namespace smjs
} // namespace ggadget
