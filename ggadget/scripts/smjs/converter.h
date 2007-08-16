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

#ifndef CONVERTER_H__
#define CONVERTER_H__

#include <jsapi.h>
#include "ggadget/variant.h"

namespace ggadget {

/**
 * Converts a @c jsval to a @c Variant of desired type.
 * @param cx JavaScript context.
 * @param prototype providing the desired target type information.
 * @param js_val source @c jsval value.
 * @param[out] native_val result @c Variant value.
 * @return @c JS_TRUE if succeeds.
 */
JSBool ConvertJSToNative(JSContext *cx, const Variant &prototype,
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
 * Converts a @c Variant to a @c jsval.
 * @param cx JavaScript context.
 * @param native_val source @c Variant value.
 * @param[out] js_val result @c jsval value.
 * @return @c JS_TRUE if succeeds.
 */
JSBool ConvertNativeToJS(JSContext* cx,
                         const Variant &native_val,
                         jsval* js_val);

} // namespace ggadget

#endif  // CONVERTER_H__
