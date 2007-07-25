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

#include "native_js_wrapper.h"
#include "converter.h"
#include "ggadget/scriptable_interface.h"
#include "ggadget/slot.h"

namespace ggadget {

// Forward declaration of callback functions.
static JSBool InvokeWrapperMethod(JSContext *cx, JSObject *obj,
                                  uintN argc, jsval *argv, jsval *rval);
static JSBool GetWrapperProperty(JSContext *cx, JSObject *obj,
                                 jsval id, jsval *vp);
static JSBool SetWrapperProperty(JSContext *cx, JSObject *obj,
                                 jsval id, jsval *vp);
static JSBool ResolveWrapperProperty(JSContext *cx, JSObject *obj, jsval id);
static void FinalizeWrapper(JSContext *cx, JSObject *obj);

// This JSClass is used to create wrapper JSObjects.
static JSClass g_wrapper_js_class = {
  "NativeJSWrapper",
  // Use the private slot to store the wrapper.
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub, GetWrapperProperty, SetWrapperProperty,
  JS_EnumerateStub, ResolveWrapperProperty, JS_ConvertStub, FinalizeWrapper,
  JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool InvokeWrapperMethod(JSContext *cx, JSObject *obj,
                                  uintN argc, jsval *argv, jsval *rval) {
  NativeJSWrapper *wrapper = NativeJSWrapper::GetWrapperFromJS(cx, obj);
  return wrapper && wrapper->InvokeMethod(argc, argv, rval);
}

static JSBool GetWrapperProperty(JSContext *cx, JSObject *obj,
                                 jsval id, jsval *vp) {
  NativeJSWrapper *wrapper = NativeJSWrapper::GetWrapperFromJS(cx, obj);
  return wrapper && wrapper->GetProperty(id, vp);
}

static JSBool SetWrapperProperty(JSContext *cx, JSObject *obj,
                                 jsval id, jsval *vp) {
  NativeJSWrapper *wrapper = NativeJSWrapper::GetWrapperFromJS(cx, obj);
  return wrapper && wrapper->SetProperty(id, *vp);
}

static JSBool ResolveWrapperProperty(JSContext *cx, JSObject *obj, jsval id) {
  NativeJSWrapper *wrapper = NativeJSWrapper::GetWrapperFromJS(cx, obj);
  return wrapper && wrapper->ResolveProperty(id);
}

static void FinalizeWrapper(JSContext *cx, JSObject *obj) {
  NativeJSWrapper *wrapper = NativeJSWrapper::GetWrapperFromJS(cx, obj);
  if (wrapper)
    delete wrapper;
}

NativeJSWrapper::NativeJSWrapper(JSContext *js_context,
                                 ScriptableInterface *scriptable)
    : deleted_(false),
      js_context_(js_context),
      scriptable_(scriptable) {

  js_object_ = JS_NewObject(js_context, &g_wrapper_js_class, NULL, NULL);
  if (!js_object_)
    // Can't continue.
    return;

  // Store this wrapper into the JSObject's private slot.
  JS_SetPrivate(js_context, js_object_, this);

  // Connect the "ondelete" callback.
  scriptable->ConnectToOnDeleteSignal(
      NewSlot(this, &NativeJSWrapper::OnDelete));
}

NativeJSWrapper::~NativeJSWrapper() {
}

JSObject *NativeJSWrapper::Wrap(JSContext *cx,
                                ScriptableInterface *scriptable) {
  NativeJSWrapper *wrapper = new NativeJSWrapper(cx, scriptable);
  return wrapper->js_object_;
}

JSBool NativeJSWrapper::Unwrap(JSContext *cx, JSObject *obj,
                               ScriptableInterface **scriptable) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  if (wrapper && !wrapper->deleted_) {
    *scriptable = wrapper->scriptable_;
    return JS_TRUE;
  } else {
    return JS_FALSE;
  }
}

// Get the NativeJSWrapper from a JS wrapped ScriptableInterface object.
// The NativeJSWrapper pointer is stored in the object's private slot.
NativeJSWrapper *NativeJSWrapper::GetWrapperFromJS(JSContext *cx,
                                                   JSObject *js_object) {
  if (js_object != NULL &&
      JS_GET_CLASS(cx, js_object) == &g_wrapper_js_class) {
    NativeJSWrapper *result = reinterpret_cast<NativeJSWrapper *>
                              (JS_GetPrivate(cx, js_object));
    ASSERT(result && result->js_context_ == cx &&
           result->js_object_ == js_object);
    return result;
  } else {
    JS_ReportError(cx, "Object is not a native wrapper");
    // The JSObject is not a JS wrapped ScriptableInterface object.
    return NULL;
  }
}

// If the wrapped native scriptable has been deleted, report an error and
// return JS_FALSE.
#define CHECK_DELETED do { \
  if (deleted_) { \
    JS_ReportError(js_context_, "Native object has been deleted"); \
    return JS_FALSE; } \
  } while (0)

JSBool NativeJSWrapper::InvokeMethod(uintN argc, jsval *argv, jsval *rval) {
  CHECK_DELETED;

  // According to JS stack structure, argv[-2] is the current function object.
  JSObject *func_object = JSVAL_TO_OBJECT(argv[-2]);
  // Get the method slot from the reserved slot.
  jsval val;
  if (!JS_GetReservedSlot(js_context_, func_object, 0, &val) ||
      !JSVAL_IS_INT(val))
    return JS_FALSE;
  Slot *slot = reinterpret_cast<Slot *>(JSVAL_TO_PRIVATE(val));

  const Variant::Type *arg_types = NULL;
  if (slot->HasMetadata()) {
    if (argc != static_cast<uintN>(slot->GetArgCount())) {
      // Argc mismatch.
      JS_ReportError(js_context_, "Wrong number of arguments: %d. %d expected",
                     argc, slot->GetArgCount());
      return JS_FALSE;
    }
    arg_types = slot->GetArgTypes();
  }

  Variant *params = NULL;
  if (argc > 0) {
    params = new Variant[argc];
    for (uintN i = 0; i < argc; i++) {
      JSBool result = arg_types != NULL ?
          ConvertJSToNative(js_context_, arg_types[i], argv[i], &params[i]) :
          ConvertJSToNativeVariant(js_context_, argv[i], &params[i]);
      if (!result) {
        JS_ReportError(js_context_,
                       "Failed to convert argument %d to native", i);
        delete [] params;
        return JS_FALSE;
      }
    }
  }

  Variant return_value = slot->Call(argc, params);
  JSBool result = ConvertNativeToJS(js_context_, return_value, rval);
  if (!result)
    JS_ReportError(js_context_, "Failed to convert result to JavaScript");
  
  if (argc > 0) {
    // TODO: how to cleanup Variant if it contains some heap-allocated things?
    delete [] params;
  }
  return result;
}

JSBool NativeJSWrapper::GetProperty(jsval id, jsval *vp) {
  CHECK_DELETED;

  if (!JSVAL_IS_INT(id))
    // Use the default logic. This may occur if the property is a method or
    // an arbitrary JavaScript property.
    return JS_TRUE;

  Variant return_value = scriptable_->GetProperty(JSVAL_TO_INT(id));
  if (return_value.type == Variant::TYPE_VOID)
    // This property is not supported by the Scriptable, use default logic.
    return JS_TRUE;

  return ConvertNativeToJS(js_context_, return_value, vp);
}

JSBool NativeJSWrapper::SetProperty(jsval id, jsval js_val) {
  CHECK_DELETED;

  if (!JSVAL_IS_INT(id))
    // Use the default logic. This may occur if the property is a method or
    // an arbitrary JavaScript property.
    return JS_TRUE;

  int int_id = JSVAL_TO_INT(id);
  Variant prototype;
  bool is_method;
  if (!scriptable_->GetPropertyInfoById(int_id, &prototype, &is_method)
      || is_method)
    // This property is not supported by the Scriptable, use default logic.
    // 'is_method' should never be true here, because only actual properties
    // are registered with tiny ids.
    return JS_TRUE;

  Variant value;
  if (!ConvertJSToNative(js_context_, prototype.type, js_val, &value)) {
    JS_ReportError(js_context_, "Failed to convert value to native");
    // TODO: check result and raise exception.
    return JS_FALSE;
  }

  if (!scriptable_->SetProperty(int_id, value)) {
    JS_ReportError(js_context_,
                   "Failed to set property %d (may be readonly)", int_id);
    return JS_FALSE;
  }
  return JS_TRUE;
}

JSBool NativeJSWrapper::ResolveProperty(jsval id) {
  CHECK_DELETED;

  if (!JSVAL_IS_STRING(id))
    return JS_TRUE;

  JSString *idstr = JS_ValueToString(js_context_, id);
  if (!idstr)
    return JS_FALSE;
  const char *name = JS_GetStringBytes(idstr);

  int int_id;
  Variant prototype;
  bool is_method;
  if (!scriptable_->GetPropertyInfoByName(name, &int_id,
                                          &prototype, &is_method))
    // This property is not supported by the Scriptable, use default logic.
    return JS_TRUE;

  if (is_method) {
    // Define a Javascript function.
    Slot *slot = prototype.v.slot_value;
    JSFunction *function = JS_DefineFunction(js_context_, js_object_, name,
                                             InvokeWrapperMethod,
                                             slot->GetArgCount(), 0);
    if (!function)
      return JS_FALSE;

    JSObject *func_object = JS_GetFunctionObject(function);
    if (!func_object)
      return JS_FALSE;
    // Put the slot pointer into the first reserved slot of the
    // function object.  A function object has 2 reserved slots.
    if (!JS_SetReservedSlot(js_context_, func_object,
                            0, PRIVATE_TO_JSVAL(slot)))
      return JS_FALSE;
  } else {
    // Define a JavaScript property.
    // Javascript tinyid is a 8-bit integer, and should  be negative to avoid
    // conflict with array indexes.
    ASSERT(int_id < 0 && int_id >= -128);
    if (!JS_DefinePropertyWithTinyId(js_context_, js_object_, name,
                                     static_cast<int8>(int_id),
                                     INT_TO_JSVAL(0),
                                     NULL, NULL,  // Use class getter/setter.
                                     JSPROP_ENUMERATE | JSPROP_PERMANENT))
      return JS_FALSE;
  }

  return JS_TRUE;
}

} // namespace ggadget
