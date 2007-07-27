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
#include "ggadget/scriptable_interface.h"
#include "ggadget/slot.h"
#include "converter.h"
#include "js_script_context.h"

namespace ggadget {

// This JSClass is used to create wrapper JSObjects.
JSClass NativeJSWrapper::wrapper_js_class_ = {
  "NativeJSWrapper",
  // Use the private slot to store the wrapper.
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub, GetWrapperProperty, SetWrapperProperty,
  JS_EnumerateStub, ResolveWrapperProperty, JS_ConvertStub, FinalizeWrapper,
  JSCLASS_NO_OPTIONAL_MEMBERS
};

NativeJSWrapper::NativeJSWrapper(JSContext *js_context,
                                 ScriptableInterface *scriptable)
    : deleted_(false),
      js_context_(js_context),
      scriptable_(scriptable) {

  js_object_ = JS_NewObject(js_context, &wrapper_js_class_, NULL, NULL);
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
      JS_GET_CLASS(cx, js_object) == &wrapper_js_class_) {
    NativeJSWrapper *wrapper = reinterpret_cast<NativeJSWrapper *>
                               (JS_GetPrivate(cx, js_object));
    ASSERT(wrapper && wrapper->js_context_ == cx &&
           wrapper->js_object_ == js_object);

    if (wrapper->deleted_) {
      JS_ReportError(wrapper->js_context_, "Native object has been deleted");
      return NULL;
    } else {
      return wrapper;
    }
  } else {
    JS_ReportError(cx, "Object is not a native wrapper");
    // The JSObject is not a JS wrapped ScriptableInterface object.
    return NULL;
  }
}

JSBool NativeJSWrapper::CallWrapperMethod(JSContext *cx, JSObject *obj,
                                          uintN argc, jsval *argv,
                                          jsval *rval) {
  if (!GetWrapperFromJS(cx, obj))
    return JS_FALSE;
  return JSScriptContext::CallNativeSlot(cx, obj, argc, argv, rval);
}

JSBool NativeJSWrapper::GetWrapperProperty(JSContext *cx, JSObject *obj,
                                           jsval id, jsval *vp) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  return wrapper && wrapper->GetProperty(id, vp);
}

JSBool NativeJSWrapper::SetWrapperProperty(JSContext *cx, JSObject *obj,
                                           jsval id, jsval *vp) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  return wrapper && wrapper->SetProperty(id, *vp);
}

JSBool NativeJSWrapper::ResolveWrapperProperty(JSContext *cx, JSObject *obj,
                                               jsval id) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  return wrapper && wrapper->ResolveProperty(id);
}

void NativeJSWrapper::FinalizeWrapper(JSContext *cx, JSObject *obj) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  if (wrapper)
    delete wrapper;
}

JSBool NativeJSWrapper::GetProperty(jsval id, jsval *vp) {
  if (!JSVAL_IS_INT(id))
    // Use the default logic. This may occur if the property is a method or
    // an arbitrary JavaScript property.
    return JS_TRUE;

  Variant return_value = scriptable_->GetProperty(JSVAL_TO_INT(id));
  if (return_value.type == Variant::TYPE_VOID)
    // This property is not supported by the Scriptable, use default logic.
    return JS_TRUE;

  if (!ConvertNativeToJS(js_context_, return_value, vp)) {
    JS_ReportError(js_context_, "Failed to convert native (%s) to jsval",
                   return_value.ToString().c_str());
    return JS_FALSE;
  }
  return JS_TRUE;
}

JSBool NativeJSWrapper::SetProperty(jsval id, jsval js_val) {
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
  if (!ConvertJSToNative(js_context_, prototype, js_val, &value)) {
    JS_ReportError(js_context_, "Failed to convert jsval (%s) to native",
                   ConvertJSToChars(js_context_, js_val));
    // TODO: check result and raise exception.
    return JS_FALSE;
  }

  if (!scriptable_->SetProperty(int_id, value))
    // It's only a warning.  Continue to return JS_TRUE.
    JS_ReportError(js_context_,
                   "Failed to set property %d (may be readonly)", int_id);
  return JS_TRUE;
}

JSBool NativeJSWrapper::ResolveProperty(jsval id) {
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
                                             CallWrapperMethod,
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
    ASSERT(int_id <= 0 && int_id >= -128);
    jsval js_val;
    if (!ConvertNativeToJS(js_context_, prototype, &js_val)) {
      JS_ReportError(js_context_, "Failed to convert init value (%s) to jsval",
                     prototype.ToString().c_str());
      return JS_FALSE;
    }

    if (int_id == 0) {
      // This property is a constant, register a property with initial value
      // and without a tiny ID.  Then the JavaScript engine will handle it.
      if (!JS_DefineProperty(js_context_, js_object_, name,
                             js_val, NULL, NULL,
                             JSPROP_READONLY | JSPROP_PERMANENT))
        return JS_FALSE;
    } else {
      // This property is a normal property.  The 'get' and 'set' operations
      // will call back to native slots.
      if (!JS_DefinePropertyWithTinyId(js_context_, js_object_, name,
                                       static_cast<int8>(int_id), js_val,
                                       NULL, NULL,  // Use class getter/setter.
                                       JSPROP_PERMANENT))
        return JS_FALSE;
    }
  }

  return JS_TRUE;
}

} // namespace ggadget
