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

namespace ggadget {
namespace internal {

const char *kFunctionReferencePrefix = "@@@FunctionReference@@@";

JSFunctionSlot::JSFunctionSlot(const Slot *prototype, JSContext *context,
                               jsval function_val)
    : prototype_(prototype),
      context_(context),
      function_val_(function_val),
      reference_from_(NULL) {
  JS_AddRoot(context, &function_val_);
}

JSFunctionSlot::~JSFunctionSlot() {
  if (!reference_from_)
    JS_RemoveRoot(context_, &function_val_);
  // Otherwise leave the reference, because we don't know if reference_from_
  // is still valid now, and leaving it wonn't cause accumulated leaks.
}

void JSFunctionSlot::SetReferenceFrom(JSObject *obj) {
  static unsigned int reference_seq = 1;
  DLOG("SetReferenceFrom: func=%p obj=%p", JSVAL_TO_OBJECT(function_val_), obj);
  reference_from_ = obj;
  std::string reference_name = StringPrintf("%s%u", kFunctionReferencePrefix,
                                            reference_seq++);
  JS_DefineProperty(context_, obj, reference_name.c_str(), function_val_,
                    JS_PropertyStub, JS_PropertyStub, 0);
  JS_RemoveRoot(context_, &function_val_);
}

Variant JSFunctionSlot::Call(int argc, Variant argv[]) const {
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
    result = ConvertJSToNative(context_, NULL, return_value, rval,
                               &return_value);
    if (!result)
      JS_ReportError(context_,
                     "Failed to convert JS function return value(%s) to native",
                     PrintJSValue(context_, rval).c_str());
  }
  return return_value;
}

} // namespace internal
} // namespace ggadget
