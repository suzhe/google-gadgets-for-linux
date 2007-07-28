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

#include <math.h>
#include <jsobj.h>
#include <jsfun.h>
#include "converter.h"
#include "native_js_wrapper.h"
#include "js_script_context.h"

namespace ggadget {

static JSBool ConvertJSToNativeVoid(JSContext *cx, jsval js_val,
                                    Variant *native_val) {
  *native_val = Variant();
  return JS_TRUE;
}

static JSBool ConvertJSToNativeBool(JSContext *cx, jsval js_val,
                                    Variant *native_val) {
  JSBool value;
  if (!JS_ValueToBoolean(cx, js_val, &value))
    return JS_FALSE;

  *native_val = Variant(static_cast<bool>(value));
  return JS_TRUE;
}

static JSBool ConvertJSToNativeInt(JSContext *cx, jsval js_val,
                                   Variant *native_val) {
  JSBool result = JS_FALSE;
  if (JSVAL_IS_INT(js_val)) {
    int32 int_val;
    result = JS_ValueToECMAInt32(cx, js_val, &int_val);
    if (result)
      *native_val = Variant(int_val);
  } else {
    jsdouble double_val;
    result = JS_ValueToNumber(cx, js_val, &double_val);
    if (result) {
      // If double_val is NaN, it may because js_val is NaN, or js_val is a
      // string containing non-numeric chars.
      if (!isnan(double_val) || js_val == JS_GetNaNValue(cx))
        *native_val = Variant(static_cast<int64_t>(double_val));
      else
        result = JS_FALSE;
    }
  }
  return result;
}

static JSBool ConvertJSToNativeDouble(JSContext *cx, jsval js_val,
                                      Variant *native_val) {
  double double_val;
  JSBool result = JS_ValueToNumber(cx, js_val, &double_val);
  if (result) {
    // If double_val is NaN, it may because js_val is NaN, or js_val is a
    // string containing non-numeric chars.
    if (!isnan(double_val) || js_val == JS_GetNaNValue(cx))
      *native_val = Variant(double_val);
    else
      result = JS_FALSE;
  }
  return result;
}

static JSBool ConvertJSToNativeString(JSContext *cx, jsval js_val,
                                      Variant *native_val) {
  JSBool result = JS_FALSE;
  if (JSVAL_IS_NULL(js_val) || JSVAL_IS_VOID(js_val)) {
    // Result is a blank string instead of a string containing "null", etc.
    result = JS_TRUE;
    *native_val = Variant(std::string());
  } else {
    JSString *js_string = JS_ValueToString(cx, js_val);
    if (js_string) {
      // TODO(wangxianzhu): Convert to UTF8.
      char *bytes = JS_GetStringBytes(js_string);
      if (bytes) {
        result = JS_TRUE;
        *native_val = Variant(std::string(bytes));
      }
    }
  }
  return result;
}

static JSBool ConvertJSToScriptable(JSContext *cx, jsval js_val,
                                    Variant *native_val) {
  JSBool result = JS_TRUE;
  ScriptableInterface *scriptable;
  if (JSVAL_IS_NULL(js_val) || JSVAL_IS_VOID(js_val)) {
    scriptable = NULL;
  } else if (JSVAL_IS_OBJECT(js_val)) {
    // This object may be a JS wrapped native object.
    // If it is not, NativeJSWrapper::Unwrap simply fails.
    // We don't support wrapping JS objects into native objects.
    result = NativeJSWrapper::Unwrap(cx, JSVAL_TO_OBJECT(js_val), &scriptable);
  } else {
    result = JS_FALSE;
  }

  if (result)
    *native_val = Variant(scriptable);
  return result;
}

static JSBool ConvertJSToSlot(JSContext *cx, Variant prototype,
                              jsval js_val, Variant *native_val) {
  JSBool result = JS_TRUE;
  jsval function_val;
  if (JSVAL_IS_NULL(js_val) || JSVAL_IS_VOID(js_val)) {
    function_val = JSVAL_NULL;
  } else if (JSVAL_IS_STRING(js_val)) {
    JSString *script_source = JSVAL_TO_STRING(js_val);
    const char *filename;
    int lineno;
    JSScriptContext::GetCurrentFileAndLine(cx, &filename, &lineno);
    JSFunction *function = JS_CompileUCFunction(
        cx, NULL, NULL, 0, NULL,  // No name and no argument.
        JS_GetStringChars(script_source), JS_GetStringLength(script_source),
        filename, lineno);
    if (!function)
      result = JS_FALSE;
    function_val = OBJECT_TO_JSVAL(JS_GetFunctionObject(function));
  } else {
    // If js_val is a function, JS_ValueToFunction will succeed.
    if (!JS_ValueToFunction(cx, js_val))
      result = JS_FALSE;
    function_val = js_val;
  }

  if (result) {
    Slot *slot = NULL;
    if (function_val != JSVAL_NULL)
      slot = JSScriptContext::NewJSFunctionSlot(cx,
                                                prototype.v.slot_value,
                                                function_val);
    *native_val = Variant(slot);
  }
  return result;
}

static JSBool ConvertJSToIntOrString(JSContext *cx, jsval js_val,
                                     Variant *native_val) {
  Variant temp;
  if (ConvertJSToNativeInt(cx, js_val, &temp)) {
    *native_val = Variant(
        CreateIntOrString(static_cast<int>(temp.v.int64_value)));
    return JS_TRUE;
  }

  if (ConvertJSToNativeString(cx, js_val, &temp)) {
    *native_val = Variant(CreateIntOrString(temp.v.string_value));
    return JS_TRUE;
  }

  return JS_FALSE;
}

typedef JSBool (*ConvertJSToNativeFunc)(JSContext *cx, jsval js_val,
                                        Variant *native_val);
static const ConvertJSToNativeFunc kConvertJSToNativeFuncs[] = {
  ConvertJSToNativeVoid,
  ConvertJSToNativeBool,
  ConvertJSToNativeInt,
  ConvertJSToNativeDouble,
  ConvertJSToNativeString,
  ConvertJSToScriptable,
  NULL, // ConvertJSToSlot is special because it needs the prototype argument.
  ConvertJSToIntOrString,
};

JSBool ConvertJSToNative(JSContext *cx, Variant prototype,
                         jsval js_val, Variant *native_val) {
  if (prototype.type == Variant::TYPE_SLOT)
    return ConvertJSToSlot(cx, prototype, js_val, native_val);

  if (prototype.type >= Variant::TYPE_VOID &&
      prototype.type <= Variant::TYPE_INT_OR_STRING)
    return kConvertJSToNativeFuncs[prototype.type](cx, js_val, native_val);

  return JS_FALSE;
}

JSBool ConvertJSToNativeVariant(JSContext *cx, jsval js_val,
                                Variant *native_val) {
  if (JSVAL_IS_VOID(js_val) || JSVAL_IS_NULL(js_val))
    return ConvertJSToNativeVoid(cx, js_val, native_val);
  if (JSVAL_IS_BOOLEAN(js_val))
    return ConvertJSToNativeBool(cx, js_val, native_val);
  if (JSVAL_IS_INT(js_val))
    return ConvertJSToNativeInt(cx, js_val, native_val);
  if (JSVAL_IS_DOUBLE(js_val))
    return ConvertJSToNativeDouble(cx, js_val, native_val);
  if (JSVAL_IS_STRING(js_val))
    return ConvertJSToNativeString(cx, js_val, native_val);
  if (JSVAL_IS_OBJECT(js_val))
    return ConvertJSToScriptable(cx, js_val, native_val);
  return JS_FALSE;
}

static JSBool ConvertNativeToJSVoid(JSContext *cx, Variant native_val,
                                    jsval *js_val) {
  *js_val = JSVAL_VOID;
  return JS_TRUE;
}

static JSBool ConvertNativeToJSBool(JSContext *cx, Variant native_val,
                                    jsval *js_val) {
  *js_val = BOOLEAN_TO_JSVAL(native_val.v.bool_value);
  return JS_TRUE;
}

static JSBool ConvertNativeToJSInt(JSContext *cx, Variant native_val,
                                   jsval *js_val) {
  int64_t value = native_val.v.int64_value;
  if (value >= JSVAL_INT_MIN && value <= JSVAL_INT_MAX) {
    *js_val = INT_TO_JSVAL(static_cast<int32>(value));
    return JS_TRUE;
  } else {
    jsdouble *pdouble = JS_NewDouble(cx, native_val.v.int64_value);
    if (pdouble) {
      *js_val = DOUBLE_TO_JSVAL(pdouble);
      return JS_TRUE;
    } else {
      return JS_FALSE;
    }
  }
}

static JSBool ConvertNativeToJSDouble(JSContext *cx, Variant native_val,
                                      jsval *js_val) {
  jsdouble *pdouble = JS_NewDouble(cx, native_val.v.double_value);
  if (pdouble) {
    *js_val = DOUBLE_TO_JSVAL(pdouble);
    return JS_TRUE;
  } else {
    return JS_FALSE;
  }
}

static JSBool ConvertNativeToJSString(JSContext *cx, Variant native_val,
                                      jsval *js_val) {
  JSBool result = JS_TRUE;
  JSString *js_string = JS_NewStringCopyZ(cx, native_val.v.string_value);
  // TODO(wangxianzhu): Convert UTF8 to jschar.
  if (js_string)
    *js_val = STRING_TO_JSVAL(js_string);
  else
    result = JS_FALSE;
  return result;
}

static JSBool ConvertNativeToJSObject(JSContext *cx, Variant native_val,
                                      jsval *js_val) {
  JSBool result = JS_TRUE;
  ScriptableInterface *scriptable = native_val.v.scriptable_value;
  if (!scriptable) {
    *js_val = JSVAL_NULL;
  } else {
    JSObject *js_object =
        JSScriptContext::WrapNativeObjectToJS(cx, scriptable);
    if (js_object)
      *js_val = OBJECT_TO_JSVAL(js_object);
    else
      result = JS_FALSE;
  }
  return result;
}

static JSBool ConvertNativeToJSFunction(JSContext *cx, Variant native_val,
                                        jsval *js_val) {
  Slot *slot = native_val.v.slot_value;
  *js_val = slot ? JSScriptContext::ConvertSlotToJS(cx, slot) :
            JSVAL_NULL;
  return JS_TRUE;
}

static JSBool ConvertNativeToJSIntOrString(JSContext *cx, Variant native_val,
                                           jsval *js_val) {
  if (native_val.v.int_or_string_value.type == IntOrString::TYPE_STRING)
    return ConvertNativeToJSString(cx,
        Variant(native_val.v.int_or_string_value.v.string_value), js_val);
  else
    return ConvertNativeToJSInt(cx,
        Variant(native_val.v.int_or_string_value.v.int_value), js_val);
}

typedef JSBool (*ConvertNativeToJSFunc)(JSContext *cx, Variant native_val,
                                        jsval *js_val);
static const ConvertNativeToJSFunc kConvertNativeToJSFuncs[] = {
  ConvertNativeToJSVoid,
  ConvertNativeToJSBool,
  ConvertNativeToJSInt,
  ConvertNativeToJSDouble,
  ConvertNativeToJSString,
  ConvertNativeToJSObject,
  ConvertNativeToJSFunction,
  ConvertNativeToJSIntOrString,
};

JSBool ConvertNativeToJS(JSContext *cx, Variant native_val, jsval *js_val) {
  return kConvertNativeToJSFuncs[native_val.type](cx, native_val, js_val);
}

} // namespace ggadget
