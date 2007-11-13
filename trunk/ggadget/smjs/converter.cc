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

#include <cmath>
#include <jsobj.h>
#include <jsfun.h>
#include <ggadget/scriptable_binary_data.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/unicode_utils.h>
#include "converter.h"
#include "js_function_slot.h"
#include "js_script_context.h"
#include "json.h"
#include "native_js_wrapper.h"

namespace ggadget {
namespace internal {

static JSBool ConvertJSToNativeVoid(JSContext *cx, jsval js_val,
                                    Variant *native_val) {
  *native_val = Variant();
  return JS_TRUE;
}

static JSBool ConvertJSToNativeBool(JSContext *cx, jsval js_val,
                                    Variant *native_val) {
  if (JSVAL_IS_STRING(js_val)) {
    JSString *js_string = JSVAL_TO_STRING(js_val);
    char *bytes = JS_GetStringBytes(js_string);
    if (!bytes)
      return JS_FALSE;
    // Convert "" or "false" to bool value false.
    *native_val = Variant(*bytes && strcasecmp(bytes, "false") != 0);
    return JS_TRUE;
  }

  JSBool value;
  if (!JS_ValueToBoolean(cx, js_val, &value))
    return JS_FALSE;

  *native_val = Variant(static_cast<bool>(value));
  return JS_TRUE;
}

static JSBool ConvertJSToNativeInt(JSContext *cx, jsval js_val,
                                   Variant *native_val) {
  if (JSVAL_IS_NULL(js_val) || JSVAL_IS_VOID(js_val)) {
    *native_val = Variant(0);
    return JS_TRUE;
  }

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
      // string containing non-numeric chars. Both case are invalid for int.
      if (!std::isnan(double_val))
        *native_val = Variant(static_cast<int64_t>(round(double_val)));
      else
        result = JS_FALSE;
    }
  }
  return result;
}

static JSBool ConvertJSToNativeDouble(JSContext *cx, jsval js_val,
                                      Variant *native_val) {
  if (JSVAL_IS_NULL(js_val) || JSVAL_IS_VOID(js_val)) {
    *native_val = Variant(0.0);
    return JS_TRUE;
  }

  double double_val = 0;
  JSBool result = JS_ValueToNumber(cx, js_val, &double_val);
  if (result) {
    if (JSVAL_IS_DOUBLE(js_val) || !std::isnan(double_val))
      // If double_val is NaN, it may because js_val is NaN, or js_val is a
      // string containing non-numeric chars. The former case is acceptable.
      *native_val = Variant(double_val);
    else
      result = JS_FALSE;
  }
  return result;
}

static JSBool ConvertJSToNativeString(JSContext *cx, jsval js_val,
                                      Variant *native_val) {
  JSBool result = JS_FALSE;
  if (JSVAL_IS_NULL(js_val)) {
    *native_val = Variant(static_cast<const char *>(NULL));
    result = JS_TRUE;
  } else if (JSVAL_IS_VOID(js_val)) {
    // Default value of a string is "";
    *native_val = Variant("");
    return JS_TRUE;
  } else if (JSVAL_IS_STRING(js_val) || JSVAL_IS_BOOLEAN(js_val) ||
             JSVAL_IS_INT(js_val) || JSVAL_IS_DOUBLE(js_val)) {
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
  } else if (JSVAL_IS_OBJECT(js_val)) {
    // Here allows asssigning ScriptableBinaryData to a native string, because
    // Windows version also allows it.
    ScriptableInterface *scriptable;
    result = NativeJSWrapper::Unwrap(cx, JSVAL_TO_OBJECT(js_val), &scriptable);
    if (result && scriptable->IsInstanceOf(ScriptableBinaryData::CLASS_ID)) {
      ScriptableBinaryData *data =
          down_cast<ScriptableBinaryData *>(scriptable);
      // Any data after '0' will be truncated.
      *native_val = Variant(data->data());
    } else {
      result = JS_FALSE;
    }
  }
  return result;
}

static JSBool ConvertJSToNativeUTF16String(JSContext *cx, jsval js_val,
                                           Variant *native_val) {
  static const UTF16Char kEmptyUTF16String[] = { 0 };
  JSBool result = JS_FALSE;
  if (JSVAL_IS_NULL(js_val)) {
    *native_val = Variant(static_cast<const UTF16Char *>(NULL));
    result = JS_TRUE;
  } else if (JSVAL_IS_VOID(js_val)) {
    *native_val = Variant(kEmptyUTF16String);
    result = JS_TRUE;
  } else if (JSVAL_IS_STRING(js_val) || JSVAL_IS_BOOLEAN(js_val) ||
             JSVAL_IS_INT(js_val) || JSVAL_IS_DOUBLE(js_val)) {
    JSString *js_string = JS_ValueToString(cx, js_val);
    if (js_string) {
      result = JS_TRUE;
      jschar *chars = JS_GetStringChars(js_string);
      // Don't cast chars to UTF16Char *, to let the compiler check if they
      // are compatible.
      *native_val = Variant(chars);
    }
  }
  return result;
}

static JSBool ConvertJSToScriptable(JSContext *cx, jsval js_val,
                                    Variant *native_val) {
  JSBool result = JS_TRUE;
  ScriptableInterface *scriptable;
  if (JSVAL_IS_NULL(js_val)) {
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

static JSBool ConvertJSToSlot(JSContext *cx, NativeJSWrapper *wrapper,
                              const Variant &prototype,
                              jsval js_val, Variant *native_val) {
  JSBool result = JS_TRUE;
  jsval function_val;
  if (JSVAL_IS_NULL(js_val)) {
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
    JSFunctionSlot *slot = NULL;
    if (function_val != JSVAL_NULL) {
      slot = new JSFunctionSlot(VariantValue<Slot *>()(prototype),
                                cx, wrapper, function_val);
    }
    *native_val = Variant(slot);
  }
  return result;
}

static JSBool ConvertJSToJSON(JSContext *cx, jsval js_val,
                              Variant *native_val) {
  std::string json;
  JSONEncode(cx, js_val, &json);
  *native_val = Variant(JSONString(json));
  return JS_TRUE;
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

JSBool ConvertJSToNativeInvalid(JSContext *cx, jsval js_val,
                                Variant *native_val) {
  return JS_FALSE;
}

JSBool ConvertJSToNative(JSContext *cx, NativeJSWrapper *wrapper,
                         const Variant &prototype,
                         jsval js_val, Variant *native_val) {
  switch (prototype.type()) {
    case Variant::TYPE_VOID:
      return ConvertJSToNativeVoid(cx, js_val, native_val);
    case Variant::TYPE_BOOL:
      return ConvertJSToNativeBool(cx, js_val, native_val);
    case Variant::TYPE_INT64:
      return ConvertJSToNativeInt(cx, js_val, native_val);
    case Variant::TYPE_DOUBLE:
      return ConvertJSToNativeDouble(cx, js_val, native_val);
    case Variant::TYPE_STRING:
      return ConvertJSToNativeString(cx, js_val, native_val);
    case Variant::TYPE_JSON:
      return ConvertJSToJSON(cx, js_val, native_val);
    case Variant::TYPE_UTF16STRING:
      return ConvertJSToNativeUTF16String(cx, js_val, native_val);
    case Variant::TYPE_SCRIPTABLE:
    case Variant::TYPE_CONST_SCRIPTABLE:
      return ConvertJSToScriptable(cx, js_val, native_val);
    case Variant::TYPE_SLOT:
      return ConvertJSToSlot(cx, wrapper, prototype, js_val, native_val);
    case Variant::TYPE_ANY:
    case Variant::TYPE_CONST_ANY:
      JS_ReportError(cx, "Script adapter doesn't support void * type");
      return JS_FALSE;
    case Variant::TYPE_VARIANT:
      return ConvertJSToNativeVariant(cx, js_val, native_val);
    default:
      return JS_FALSE;
  }
}

void FreeNativeValue(const Variant &native_val) {
  // Delete the JSFunctionSlot object that was created by ConvertJSToNative().
  if (native_val.type() == Variant::TYPE_SLOT)
    delete VariantValue<Slot *>()(native_val);
}

std::string PrintJSValue(JSContext *cx, jsval js_val) {
  switch (JS_TypeOfValue(cx, js_val)) {
    case JSTYPE_STRING: {
      Variant v;
      ConvertJSToNativeString(cx, js_val, &v);
      return VariantValue<std::string>()(v);
    }
    case JSTYPE_OBJECT: {
      std::string json;
      JSONEncode(cx, js_val, &json);
      return json;
    }
    default: {
      JSString *str = JS_ValueToString(cx, js_val);
      if (str) {
        char *bytes = JS_GetStringBytes(str);
        if (bytes)
          return std::string(bytes);
      }
      return "##ERROR##";
    }
  }
}

JSBool ConvertJSArgsToNative(JSContext *cx, NativeJSWrapper *wrapper,
                             Slot *slot, uintN argc, jsval *argv,
                             Variant **params, uintN *expected_argc) {
  *params = NULL;
  const Variant::Type *arg_types = NULL;
  *expected_argc = argc;
  if (slot->HasMetadata()) {
    arg_types = slot->GetArgTypes();
    *expected_argc = static_cast<uintN>(slot->GetArgCount());
    if (argc != *expected_argc) {
      uintN min_arg_count = *expected_argc;
      if (argc < *expected_argc) {
        for (uintN i = min_arg_count - 1; i >= 0; i--) {
          // Variant parameters at the end of the parameter list are optional.
          if (arg_types[i] == Variant::TYPE_VARIANT)
            min_arg_count--;
          else
            break;
        }
      }

      if (argc > *expected_argc || argc < min_arg_count) {
        // Argc mismatch.
        JS_ReportError(cx, "Wrong number of arguments: %u "
                       "(expected: %u, at least: %u)",
                       argc, slot->GetArgCount(), min_arg_count);
        return JS_FALSE;
      }
    }
  }

  if (*expected_argc > 0) {
    *params = new Variant[*expected_argc];
    for (uintN i = 0; i < argc; i++) {
      JSBool result = arg_types != NULL ?
          ConvertJSToNative(cx, wrapper, Variant(arg_types[i]), argv[i],
                            &(*params)[i]) :
          ConvertJSToNativeVariant(cx, argv[i], &(*params)[i]);
      if (!result) {
        for (uintN j = 0; j <= i; j++)
          FreeNativeValue((*params)[j]);
        delete [] *params;
        *params = NULL;
        JS_ReportError(cx, "Failed to convert argument %d(%s) to native",
                       i, PrintJSValue(cx, argv[i]).c_str());
        return JS_FALSE;
      }
    }
    // Not specified optional parameters remain TYPE_VOID. 
  }
  return JS_TRUE;
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
  if (!char_ptr) {
    *js_val = JSVAL_NULL;
  } else {
    UTF16String utf16_string;
    ConvertStringUTF8ToUTF16(char_ptr, strlen(char_ptr), &utf16_string);
    // Don't cast utf16_string.c_str() to jschar *, to let the compiler check
    // if they are compatible.
    JSString *js_string = JS_NewUCStringCopyZ(cx, utf16_string.c_str());
    if (js_string)
      *js_val = STRING_TO_JSVAL(js_string);
    else
      result = JS_FALSE;
  }
  return result;
}

static JSBool ConvertNativeUTF16ToJSString(JSContext *cx,
                                           const Variant &native_val,
                                           jsval *js_val) {
  JSBool result = JS_TRUE;
  const UTF16Char *char_ptr = VariantValue<const UTF16Char *>()(native_val);
  if (!char_ptr) {
    *js_val = JSVAL_NULL;
  } else {
    // Don't cast utf16_string.c_str() to jschar *, to let the compiler check
    // if they are compatible.
    JSString *js_string = JS_NewUCStringCopyZ(cx, char_ptr);
    if (js_string)
      *js_val = STRING_TO_JSVAL(js_string);
    else
      result = JS_FALSE;
  }
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
        JSScriptContext::WrapNativeObjectToJS(cx, scriptable)->js_object();
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
  DLOG("Reading native function in JavaScript");
  // Just leave the value that SpiderMonkey recorded in SetProperty.
  return JS_TRUE;
}

static JSBool ConvertJSONToJS(JSContext *cx,
                              const Variant &native_val,
                              jsval *js_val) {
  JSONString json_str = VariantValue<JSONString>()(native_val);
  return JSONDecode(cx, json_str.value.c_str(), js_val);
}

JSBool ConvertNativeToJS(JSContext *cx,
                         const Variant &native_val,
                         jsval *js_val) {
  switch (native_val.type()) {
    case Variant::TYPE_VOID:
      return ConvertNativeToJSVoid(cx, native_val, js_val);
    case Variant::TYPE_BOOL:
      return ConvertNativeToJSBool(cx, native_val, js_val);
    case Variant::TYPE_INT64:
      return ConvertNativeToJSInt(cx, native_val, js_val);
    case Variant::TYPE_DOUBLE:
      return ConvertNativeToJSDouble(cx, native_val, js_val);
    case Variant::TYPE_STRING:
      return ConvertNativeToJSString(cx, native_val, js_val);
    case Variant::TYPE_JSON:
      return ConvertJSONToJS(cx, native_val, js_val);
    case Variant::TYPE_UTF16STRING:
      return ConvertNativeUTF16ToJSString(cx, native_val, js_val);
    case Variant::TYPE_SCRIPTABLE:
      return ConvertNativeToJSObject(cx, native_val, js_val);
    case Variant::TYPE_CONST_SCRIPTABLE:
      JS_ReportError(cx,
                     "Don't pass const ScriptableInterface * to JavaScript");
      return JS_FALSE;
    case Variant::TYPE_SLOT:
      return ConvertNativeToJSFunction(cx, native_val, js_val);
    case Variant::TYPE_ANY:
    case Variant::TYPE_CONST_ANY:
      JS_ReportError(cx, "Don't pass (const) void * to JavaScript");
      return JS_FALSE;
    case Variant::TYPE_VARIANT:
      // Normally there is no real value of this type, so convert it to void.
      return ConvertNativeToJSVoid(cx, native_val, js_val);
    default:
      return JS_FALSE;
  }
}

} // namespace internal
} // namespace ggadget
