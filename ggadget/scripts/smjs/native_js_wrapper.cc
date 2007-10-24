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
#include "ggadget/signal.h"
#include "ggadget/slot.h"
#include "converter.h"
#include "js_script_context.h"

namespace ggadget {

// This JSClass is used to create wrapper JSObjects.
JSClass NativeJSWrapper::wrapper_js_class_ = {
  "NativeJSWrapper",
  // Use the private slot to store the wrapper.
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub,
  GetWrapperPropertyDefault, SetWrapperPropertyDefault,
  JS_EnumerateStub, ResolveWrapperProperty,
  JS_ConvertStub, FinalizeWrapper,
  JSCLASS_NO_OPTIONAL_MEMBERS
};

NativeJSWrapper::NativeJSWrapper(JSContext *js_context,
                                 ScriptableInterface *scriptable)
    : deleted_(false),
      js_context_(js_context),
      js_object_(NULL),
      scriptable_(scriptable),
      ondelete_connection_(NULL) {

  js_object_ = JS_NewObject(js_context, &wrapper_js_class_, NULL, NULL);
  if (!js_object_)
    // Can't continue.
    return;

  // Store this wrapper into the JSObject's private slot.
  JS_SetPrivate(js_context, js_object_, this);

  // Connect the "ondelete" callback.
  ondelete_connection_ = scriptable->ConnectToOnDeleteSignal(
      NewSlot(this, &NativeJSWrapper::OnDelete));
  scriptable->Attach();
}

NativeJSWrapper::~NativeJSWrapper() {
  if (!deleted_) {
    ondelete_connection_->Disconnect();
    scriptable_->Detach();
  }
}

JSBool NativeJSWrapper::Unwrap(JSContext *cx, JSObject *obj,
                               ScriptableInterface **scriptable) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj, false);
  if (wrapper) {
    *scriptable = wrapper->scriptable_;
    return JS_TRUE;
  } else {
    return JS_FALSE;
  }
}

// Get the NativeJSWrapper from a JS wrapped ScriptableInterface object.
// The NativeJSWrapper pointer is stored in the object's private slot.
NativeJSWrapper *NativeJSWrapper::GetWrapperFromJS(JSContext *cx,
                                                   JSObject *js_object,
                                                   JSBool finalizing) {
  if (js_object != NULL &&
      JS_GET_CLASS(cx, js_object) == &wrapper_js_class_) {
    NativeJSWrapper *wrapper = reinterpret_cast<NativeJSWrapper *>
                               (JS_GetPrivate(cx, js_object));
    ASSERT(wrapper && wrapper->js_context_ == cx &&
           wrapper->js_object_ == js_object);

    if (!finalizing && wrapper->deleted_) {
      // Don't report error when finalizing.
      // Early deletion of a native object is allowed.
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
  if (!GetWrapperFromJS(cx, obj, false))
    return JS_FALSE;
  return JSScriptContext::CallNativeSlot(cx, obj, argc, argv, rval);
}

JSBool NativeJSWrapper::GetWrapperPropertyDefault(JSContext *cx, JSObject *obj,
                                                  jsval id, jsval *vp) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj, false);
  return wrapper && wrapper->GetPropertyDefault(id, vp);
}

JSBool NativeJSWrapper::SetWrapperPropertyDefault(JSContext *cx, JSObject *obj,
                                                  jsval id, jsval *vp) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj, false);
  return wrapper && wrapper->SetPropertyDefault(id, *vp);
}

JSBool NativeJSWrapper::GetWrapperPropertyByIndex(JSContext *cx, JSObject *obj,
                                                  jsval id, jsval *vp) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj, false);
  return wrapper && wrapper->GetPropertyByIndex(id, vp);
}

JSBool NativeJSWrapper::SetWrapperPropertyByIndex(JSContext *cx, JSObject *obj,
                                                  jsval id, jsval *vp) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj, false);
  return wrapper && wrapper->SetPropertyByIndex(id, *vp);
}

JSBool NativeJSWrapper::GetWrapperPropertyByName(JSContext *cx, JSObject *obj,
                                                 jsval id, jsval *vp) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj, false);
  return wrapper && wrapper->GetPropertyByName(id, vp);
}

JSBool NativeJSWrapper::SetWrapperPropertyByName(JSContext *cx, JSObject *obj,
                                                 jsval id, jsval *vp) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj, false);
  return wrapper && wrapper->SetPropertyByName(id, *vp);
}

JSBool NativeJSWrapper::ResolveWrapperProperty(JSContext *cx, JSObject *obj,
                                               jsval id) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj, false);
  return wrapper && wrapper->ResolveProperty(id);
}

void NativeJSWrapper::FinalizeWrapper(JSContext *cx, JSObject *obj) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj, true);
  if (wrapper) {
    if (!wrapper->deleted_)
      JSScriptContext::FinalizeNativeJSWrapper(cx, wrapper);
    delete wrapper;
  }
}

void NativeJSWrapper::OnDelete() {
  ondelete_connection_->Disconnect();
  deleted_ = true;
  // Remove the wrapper mapping from the context, but leave this wrapper
  // alive to accept mistaken JavaScript calls gracefully.
  JSScriptContext::FinalizeNativeJSWrapper(js_context_, this);
}

JSBool NativeJSWrapper::GetPropertyDefault(jsval id, jsval *vp) {
  if (JSVAL_IS_INT(id))
    // The script wants to get the property by an array index.
    return GetPropertyByIndex(id, vp);

  // Use the default JavaScript logic.
  return JS_TRUE;
}

JSBool NativeJSWrapper::SetPropertyDefault(jsval id, jsval js_val) {
  if (JSVAL_IS_INT(id))
    // The script wants to set the property by an array index.
    return SetPropertyByIndex(id, js_val);

  if (scriptable_->IsStrict()) {
    // The scriptable object don't allow the script engine to assign to
    // unregistered properties.
    JS_ReportError(js_context_,
                   "The native object doesn't support setting property %s.",
                   PrintJSValue(js_context_, id).c_str());
    return JS_FALSE;
  }
  // Use the default JavaScript logic.
  return JS_TRUE;
}

JSBool NativeJSWrapper::GetPropertyByIndex(jsval id, jsval *vp) {
  if (!JSVAL_IS_INT(id))
    // Should not occur.
    return JS_FALSE;

  int int_id = JSVAL_TO_INT(id);
  try {
    Variant return_value = scriptable_->GetProperty(int_id);
    if (!ConvertNativeToJS(js_context_, return_value, vp)) {
      JS_ReportError(js_context_,
                     "Failed to convert native property value(%s) to jsval.",
                     return_value.ToString().c_str());
      return JS_FALSE;
    }
  } catch (ScriptableExceptionHolder e) {
    return JSScriptContext::HandleException(js_context_, e);
  }
  return JS_TRUE;
}

JSBool NativeJSWrapper::SetPropertyByIndex(jsval id, jsval js_val) {
  if (!JSVAL_IS_INT(id))
    // Should not occur.
    return JS_FALSE;

  try {
    int int_id = JSVAL_TO_INT(id);
    Variant prototype;
    bool is_method = false;
    const char *name = NULL;

    if (!scriptable_->GetPropertyInfoById(int_id, &prototype,
                                          &is_method, &name)) {
      // This property is not supported by the Scriptable, use default logic.
      JS_ReportError(
          js_context_,
          "The native object doesn't support setting property %s(%d).",
          name, int_id);
      return JS_FALSE;
    }
    ASSERT(!is_method);
  
    Variant value;
    if (!ConvertJSToNative(js_context_, prototype, js_val, &value)) {
      JS_ReportError(js_context_,
                     "Failed to convert JS property value(%s) to native",
                     PrintJSValue(js_context_, js_val).c_str());
      return JS_FALSE;
    }
  
    if (!scriptable_->SetProperty(int_id, value)) {
      JS_ReportError(js_context_,
                     "Failed to set native property %s(%d) (may be readonly)",
                     name, int_id);
      return JS_FALSE;
    }
  } catch (ScriptableExceptionHolder e) {
    return JSScriptContext::HandleException(js_context_, e);
  }

  return JS_TRUE;
}

JSBool NativeJSWrapper::GetPropertyByName(jsval id, jsval *vp) {
  if (!JSVAL_IS_STRING(id))
    // Should not occur
    return JS_FALSE;

  try {
    JSString *idstr = JSVAL_TO_STRING(id);
    if (!idstr)
      return JS_FALSE;
    const char *name = JS_GetStringBytes(idstr);
    int int_id;
    Variant prototype;
    bool is_method;
    if (!scriptable_->GetPropertyInfoByName(name, &int_id,
                                            &prototype, &is_method)) {
      // This must be a dynamic property which is no more available.
      // Remove the property and fallback to the default handler.
      JS_DeleteProperty(js_context_, js_object_, name);
      return GetPropertyDefault(id, vp);
    }
    ASSERT(!is_method);
  
    Variant return_value = scriptable_->GetProperty(int_id);
    if (!ConvertNativeToJS(js_context_, return_value, vp)) {
      JS_ReportError(js_context_,
                     "Failed to convert native property value(%s) to jsval",
                     return_value.ToString().c_str());
      return JS_FALSE;
    }
  } catch (ScriptableExceptionHolder e) {
    return JSScriptContext::HandleException(js_context_, e);
  }

  return JS_TRUE;
}

JSBool NativeJSWrapper::SetPropertyByName(jsval id, jsval js_val) {
  if (!JSVAL_IS_STRING(id))
    // Should not occur
    return JS_FALSE;

  try {
    JSString *idstr = JSVAL_TO_STRING(id);
    if (!idstr)
      return JS_FALSE;
    const char *name = JS_GetStringBytes(idstr);
    int int_id;
    Variant prototype;
    bool is_method;
    if (!scriptable_->GetPropertyInfoByName(name, &int_id,
                                            &prototype, &is_method)) {
      // This must be a dynamic property which is no more available.
      // Remove the property and fallback to the default handler.
      JS_DeleteProperty(js_context_, js_object_, name);
      return SetPropertyDefault(id, js_val);
    }
    ASSERT(!is_method);
  
    Variant value;
    if (!ConvertJSToNative(js_context_, prototype, js_val, &value)) {
      JS_ReportError(js_context_,
                     "Failed to convert JS property value(%s) to native.",
                     PrintJSValue(js_context_, js_val).c_str());
      return JS_FALSE;
    }
  
    if (!scriptable_->SetProperty(int_id, value)) {
      JS_ReportError(js_context_,
                     "Failed to set native property %s(%d) (may be readonly).",
                     name, int_id);
      return JS_FALSE;
    }
  } catch (ScriptableExceptionHolder e) {
    return JSScriptContext::HandleException(js_context_, e);
  }

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

  try {
    if (!scriptable_->GetPropertyInfoByName(name, &int_id,
                                            &prototype, &is_method))
      // This property is not supported by the Scriptable, use default logic.
      return JS_TRUE;
  } catch (ScriptableExceptionHolder e) {
    return JSScriptContext::HandleException(js_context_, e);
  }

  ASSERT(int_id <= 0);

  if (is_method) {
    // Define a Javascript function.
    Slot *slot = VariantValue<Slot *>()(prototype);
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
    jsval js_val;
    if (!ConvertNativeToJS(js_context_, prototype, &js_val)) {
      JS_ReportError(js_context_, "Failed to convert init value(%s) to jsval",
                     prototype.ToString().c_str());
      return JS_FALSE;
    }

    if (int_id == ScriptableInterface::ID_CONSTANT_PROPERTY)
      // This property is a constant, register a property with initial value
      // and without a tiny ID.  Then the JavaScript engine will handle it.
      return JS_DefineProperty(js_context_, js_object_, name,
                               js_val, JS_PropertyStub, JS_PropertyStub,
                               JSPROP_READONLY | JSPROP_PERMANENT);

    if (int_id == ScriptableInterface::ID_DYNAMIC_PROPERTY)
      return JS_DefineProperty(js_context_, js_object_, name, js_val,
                               GetWrapperPropertyByName,
                               SetWrapperPropertyByName,
                               0);

    if (int_id < 0 && int_id >= -128)
      // Javascript tinyid is a 8-bit integer, and should be negative to avoid
      // conflict with array indexes.
      // This property is a normal property.  The 'get' and 'set' operations
      // will call back to native slots.
      return JS_DefinePropertyWithTinyId(js_context_, js_object_, name,
                                         static_cast<int8>(int_id), js_val,
                                         GetWrapperPropertyByIndex,
                                         SetWrapperPropertyByIndex,
                                         JSPROP_PERMANENT);

    // Too many properties, can't register all with tiny id.  The rest are
    // registered by name.
    return JS_DefineProperty(js_context_, js_object_, name, js_val,
                             GetWrapperPropertyByName,
                             SetWrapperPropertyByName,
                             JSPROP_PERMANENT);
  }

  return JS_TRUE;
}

} // namespace ggadget
