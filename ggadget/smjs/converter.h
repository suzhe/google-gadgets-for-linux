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

#ifndef GGADGET_SMJS_CONVERTER_H__
#define GGADGET_SMJS_CONVERTER_H__

#include <jsapi.h>
#include <string>

#include <ggadget/variant.h>

namespace ggadget {
namespace smjs {

class NativeJSWrapper;

/**
 * Converts a @c jsval to a @c Variant of desired type.
 * @param cx JavaScript context.
 * @param wrapper the related JavaScript object wrapper.
 * @param prototype providing the desired target type information.
 * @param js_val source @c jsval value.
 * @param[out] native_val result @c Variant value.
 * @return @c JS_TRUE if succeeds.
 */
JSBool ConvertJSToNative(JSContext *cx, NativeJSWrapper *wrapper,
                         const Variant &prototype,
                         jsval js_val, Variant *native_val);

/**
 * Converts a @c jsval to a @c Variant depending source @c jsval type.
 * @param cx JavaScript context.
 * @param js_val source @c jsval value.
 * @param[out] native_val result @c Variant value.
 * @return @c JS_TRUE if succeeds.
 */
JSBool ConvertJSToNativeVariant(JSContext *cx,
                                jsval js_val, Variant *native_val);

/**
 * Frees a native value that was created by @c ConvertJSToNative(),
 * if some failed conditions preventing this value from successfully
 * passing to the native code.
 */
void FreeNativeValue(const Variant &native_val);

/**
 * Converts a @c jsval to a @c std::string for printing.
 */
std::string PrintJSValue(JSContext *cx, jsval js_val);

/**
 * Converts JavaScript arguments to native for a native slot.
 */
JSBool ConvertJSArgsToNative(JSContext *cx, NativeJSWrapper *wrapper,
                             Slot *slot, uintN argc, jsval *argv,
                             Variant **params, uintN *expected_argc);

/**
 * Converts a @c Variant to a @c jsval.
 * @param cx JavaScript context.
 * @param native_val source @c Variant value.
 * @param[out] js_val result @c jsval value.
 * @return @c JS_TRUE if succeeds.
 */
JSBool ConvertNativeToJS(JSContext* cx,
                         const Variant &native_val,
                         jsval* js_val);

/**
 * Compiles function source into <code>JSFunction *</code>.
 */
JSFunction *CompileFunction(JSContext *cx, const char *script,
                            const char *filename, int lineno);

/**
 * Compile and evaluate a piece of script.
 */
JSBool EvaluateScript(JSContext *cx, const char *script,
                      const char *filename, int lineno, jsval *rval);

} // namespace smjs
} // namespace ggadget

#endif  // GGADGET_SMJS_CONVERTER_H__
