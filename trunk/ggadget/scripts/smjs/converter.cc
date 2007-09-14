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
#include "ggadget/scriptable_interface.h"
#include "ggadget/unicode_utils.h"
#include "js_script_context.h"
#include "native_js_wrapper.h"

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
    // Result is a null string instead of a string containing "null".
    result = JS_TRUE;
    *native_val = Variant(static_cast<const char *>(NULL));
  } else {
    JSString *js_string = JS_ValueToString(cx, js_val);
    if (js_string) {
      jschar *chars = JS_GetStringChars(js_string);
      if (chars) {
        result = JS_TRUE;
        std::string utf8_string;
        // Don't cast chars to UTF16Char *, to let the compiler check if they
        // are compatible.
        ConvertStringUTF16ToUTF8(chars, JS_GetStringLength(js_string),
                                 &utf8_string);
        *native_val = Variant(utf8_string);
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

static JSBool ConvertJSToSlot(JSContext *cx, const Variant &prototype,
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
      slot = JSScriptContext::NewJSFunctionSlot(
          cx, VariantValue<Slot *>()(prototype), function_val);
    *native_val = Variant(slot);
  }
  return result;
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

typedef JSBool (*ConvertJSToNativeFunc)(JSContext *cx, jsval js_val,
                                        Variant *native_val);
static const ConvertJSToNativeFunc kConvertJSToNativeFuncs[] = {
  ConvertJSToNativeVoid,
  ConvertJSToNativeBool,
  ConvertJSToNativeInt,
  ConvertJSToNativeDouble,
  ConvertJSToNativeString,
  ConvertJSToScriptable,
  ConvertJSToScriptable,
  NULL, // ConvertJSToSlot is special because it needs the prototype argument.
  ConvertJSToNativeVariant,
};

JSBool ConvertJSToNative(JSContext *cx, const Variant &prototype,
                         jsval js_val, Variant *native_val) {
  if (prototype.type() == Variant::TYPE_SLOT)
    return ConvertJSToSlot(cx, prototype, js_val, native_val);

  if (prototype.type() >= Variant::TYPE_VOID &&
      prototype.type() <= Variant::TYPE_VARIANT)
    return kConvertJSToNativeFuncs[prototype.type()](cx, js_val, native_val);

  return JS_FALSE;
}

std::string ConvertJSToString(JSContext *cx, jsval js_val) {
  Variant v;
  return ConvertJSToNativeString(cx, js_val, &v) ?
         VariantValue<std::string>()(v) :
         std::string("##ERROR##");
}

static JSBool ConvertNativeToJSVoid(JSContext *cx,
                                    const Variant &native_val,
                                    jsval *js_val) {
  *js_val = JSVAL_VOID;
  return JS_TRUE;
}

static JSBool ConvertNativeToJSBool(JSContext *cx,
                                    const Variant &native_val,
                                    jsval *js_val) {
  *js_val = BOOLEAN_TO_JSVAL(VariantValue<bool>()(native_val));
  return JS_TRUE;
}

static JSBool ConvertNativeToJSInt(JSContext *cx,
                                   const Variant &native_val,
                                   jsval *js_val) {
  int64_t value = VariantValue<int64_t>()(native_val);
  if (value >= JSVAL_INT_MIN && value <= JSVAL_INT_MAX) {
    *js_val = INT_TO_JSVAL(static_cast<int32>(value));
    return JS_TRUE;
  } else {
    jsdouble *pdouble = JS_NewDouble(cx, value);
    if (pdouble) {
      *js_val = DOUBLE_TO_JSVAL(pdouble);
      return JS_TRUE;
    } else {
      return JS_FALSE;
    }
  }
}

static JSBool ConvertNativeToJSDouble(JSContext *cx,
                                      const Variant &native_val,
                                      jsval *js_val) {
  jsdouble *pdouble = JS_NewDouble(cx, VariantValue<double>()(native_val));
  if (pdouble) {
    *js_val = DOUBLE_TO_JSVAL(pdouble);
    return JS_TRUE;
  } else {
    return JS_FALSE;
  }
}

static JSBool ConvertNativeToJSString(JSContext *cx,
                                      const Variant &native_val,
                                      jsval *js_val) {
  JSBool result = JS_TRUE;
  const char *char_ptr = VariantValue<const char *>()(native_val);
  UTF16String utf16_string;
  ConvertStringUTF8ToUTF16(char_ptr, strlen(char_ptr), &utf16_string);
  // Don't cast utf16_string.c_str() to jschar *, to let the compiler check
  // if they are compatible.
  JSString *js_string = JS_NewUCStringCopyZ(cx, utf16_string.c_str());
  if (js_string)
    *js_val = STRING_TO_JSVAL(js_string);
  else
    result = JS_FALSE;
  return result;
}

static JSBool ConvertNativeToJSObject(JSContext *cx,
                                      const Variant &native_val,
                                      jsval *js_val) {
  JSBool result = JS_TRUE;
  ScriptableInterface *scriptable =
      native_val.type() == Variant::TYPE_CONST_SCRIPTABLE ?
      const_cast<ScriptableInterface *>
        (VariantValue<const ScriptableInterface *>()(native_val)) :
      VariantValue<ScriptableInterface *>()(native_val);
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

static JSBool ConvertNativeToJSFunction(JSContext *cx,
                                        const Variant &native_val,
                                        jsval *js_val) {
  Slot *slot = VariantValue<Slot *>()(native_val);
  *js_val = slot ? JSScriptContext::ConvertSlotToJS(cx, slot) : JSVAL_NULL;
  return JS_TRUE;
}

typedef JSBool (*ConvertNativeToJSFunc)(JSContext *cx,
                                        const Variant &native_val,
                                        jsval *js_val);
static const ConvertNativeToJSFunc kConvertNativeToJSFuncs[] = {
  ConvertNativeToJSVoid,
  ConvertNativeToJSBool,
  ConvertNativeToJSInt,
  ConvertNativeToJSDouble,
  ConvertNativeToJSString,
  ConvertNativeToJSObject,
  NULL, // Don't pass const ScriptableInterface * to JavaScript. 
  ConvertNativeToJSFunction,
  // Because normally there is no real value of this type, convert it to void. 
  ConvertNativeToJSVoid,
};

JSBool ConvertNativeToJS(JSContext *cx,
                         const Variant &native_val,
                         jsval *js_val) {
  if (native_val.type() == Variant::TYPE_CONST_SCRIPTABLE) {
    JS_ReportError(cx, "Don't pass const ScriptableInterface * to JavaScript");
    return JS_FALSE;
  }

  if (native_val.type() >= Variant::TYPE_VOID &&
      native_val.type() <= Variant::TYPE_VARIANT) {
    return kConvertNativeToJSFuncs[native_val.type()](cx, native_val, js_val);
  }
  return JS_FALSE;
}

} // namespace ggadget
