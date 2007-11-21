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
namespace internal {

JSFunctionSlot::JSFunctionSlot(const Slot *prototype,
                               JSContext *context,
                               NativeJSWrapper *wrapper,
                               jsval function_val)
    : prototype_(prototype),
      context_(context),
      wrapper_(wrapper),
      function_val_(function_val),
      finalized_(false) {
  if (wrapper)
    wrapper->AddJSFunctionSlot(this);
  else
    JS_AddRoot(context, &function_val_);
}

JSFunctionSlot::~JSFunctionSlot() {
  if (!finalized_) {
    if (wrapper_)
      wrapper_->RemoveJSFunctionSlot(this);
    else
      JS_RemoveRoot(context_, &function_val_);
  }
}

Variant JSFunctionSlot::Call(int argc, Variant argv[]) const {
  if (finalized_) {
    JS_ReportError(context_, "Finalized JavaScript funcction still be called");
    return Variant();
  }

  AutoLocalRootScope local_root_scope(context_);
  if (!local_root_scope.good())
    return Variant();

  scoped_array<jsval> js_args;
  Variant return_value(GetReturnType());
  if (argc > 0) {
    js_args.reset(new jsval[argc]);
    for (int i = 0; i < argc; i++) {
      if (!ConvertNativeToJS(context_, argv[i], &js_args[i])) {
        JS_ReportError(context_, "Failed to convert argument %d(%s) to jsval",
                       i, argv[i].ToString().c_str());
        return return_value;
      }
    }
  }

  jsval rval;
  JSBool result = JS_CallFunctionValue(context_, NULL, function_val_,
                                       argc, js_args.get(), &rval);
  if (result) {
    if (JSVAL_IS_OBJECT(rval) &&
        JS_IsArrayObject(context_, JSVAL_TO_OBJECT(rval))) {
      // Returning an array from JS to native is not supported for now, to
      // avoid memory management difficulties.
      result = JS_FALSE;
    } else {
      result = ConvertJSToNative(context_, NULL, return_value, rval,
                                 &return_value);
    }
    if (!result)
      JS_ReportError(context_,
                     "Failed to convert JS function return value(%s) to native",
                     PrintJSValue(context_, rval).c_str());
  }
  return return_value;
}

void JSFunctionSlot::Mark() {
  JS_MarkGCThing(context_, JSVAL_TO_OBJECT(function_val_),
                 "JSFunctionSlot", NULL);
}

void JSFunctionSlot::Finalize() {
  finalized_ = true;
}

} // namespace internal
} // namespace ggadget
