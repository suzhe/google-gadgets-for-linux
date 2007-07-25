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

#include <jsobj.h>
#include <jsfun.h>
#include "converter.h"
#include "native_js_wrapper.h"
//#include "js_native_wrapper.h"

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
    if (result)
      *native_val = Variant(static_cast<int64>(double_val));
  }
  return result;
}

static JSBool ConvertJSToNativeDouble(JSContext *cx, jsval js_val,
                                      Variant *native_val) {
  double double_val;
  JSBool result = JS_ValueToNumber(cx, js_val, &double_val);
  if (result)
    *native_val = Variant(double_val);
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
  if (JSVAL_IS_NULL(js_val) || JSVAL_IS_VOID(js_val)) {
    *native_val = Variant(static_cast<ScriptableInterface *>(NULL));
  } /*else if (JSVAL_IS_STRING(js_val)) {
    
    // Sometimew we allow a string containing a function definition in place
    // of an IDispatch callback object.
    // Any method call to the object will be delegated to this function.
    result = JSStringNativeWrapper::Wrap(cx,
                                         JSVAL_TO_STRING(js_val),
                                         &com_val->pdispVal);
    // JS_CompileFunction(cx, 
  } else if (JSVAL_IS_FUNCTION(cx, js_val)) {
    // Sometimes we allow a function in place of an IDispatch callback object.
    // Any method call to the object will be delegated to this function.
    result = JSFunctionDispatchWrapper::Wrap(cx,
                                             JS_ValueToFunction(cx, js_val),
                                             &com_val->pdispVal);
  } else if (JSVAL_IS_OBJECT(js_val)) {
    // This object may be a JS wrapped native object.
    // If it is not, NativeJSWrapper::Unwrap simply fails.
    // We don't support wrapping JS object into native object now.
    result = NativeJSWrapper::Unwrap(cx, JSVAL_TO_OBJECT(js_val),
                                     &com_val->pdispVal);
  } */else {
    result = JS_FALSE;
  }

  return result;
}

static JSBool ConvertJSToSlot(JSContext *cx, jsval js_val,
                              Variant *native_val) {
  // TODO: implement it.
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
  ConvertJSToSlot,
};

JSBool ConvertJSToNative(JSContext *cx, Variant::Type type,
                         jsval js_val, Variant *native_val) {
  if (type >= Variant::TYPE_VOID && type <= Variant::TYPE_SLOT)
    return kConvertJSToNativeFuncs[type](cx, js_val, native_val);
  else
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
  int64 value = native_val.v.int64_value;
  if (value >= JSVAL_INT_MIN && value <= JSVAL_INT_MAX) {
    *js_val = INT_TO_JSVAL(value);
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
    JSObject *js_object = NativeJSWrapper::Wrap(cx, scriptable);
    if (js_object)
      *js_val = OBJECT_TO_JSVAL(js_object);
    else
      result = JS_FALSE;
  }
  return result;
}

static JSBool ConvertNativeToJSFunction(JSContext *cx, Variant native_val,
                                        jsval *js_val) {
  if (native_val.v.slot_value) {
    // TODO: is this useful?
    return JS_FALSE;
  } else {
    *js_val = JSVAL_NULL;
    return JS_TRUE;
  }
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
};

JSBool ConvertNativeToJS(JSContext *cx, Variant native_val, jsval *js_val) {
  return kConvertNativeToJSFuncs[native_val.type](cx, native_val, js_val);
}

} // namespace ggadget
