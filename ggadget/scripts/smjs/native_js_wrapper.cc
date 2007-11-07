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

#include <ggadget/scriptable_interface.h>
#include <ggadget/signals.h>
#include <ggadget/slot.h>
#include "converter.h"
#include "native_js_wrapper.h"
#include "js_script_context.h"

#ifdef _DEBUG
// Uncomment the following to debug wrapper memory.
// #define DEBUG_JS_WRAPPER_MEMORY
#endif

namespace ggadget {
namespace internal {

// This JSClass is used to create wrapper JSObjects.
JSClass NativeJSWrapper::wrapper_js_class_ = {
  "NativeJSWrapper",
  // Use the private slot to store the wrapper.
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub,
  GetWrapperPropertyDefault, SetWrapperPropertyDefault,
  JS_EnumerateStub, ResolveWrapperProperty,
  JS_ConvertStub, FinalizeWrapper,
  NULL, NULL, CallWrapperSelf,
  JSCLASS_NO_RESERVED_MEMBERS
};

NativeJSWrapper::NativeJSWrapper(JSContext *js_context,
                                 JSObject *js_object,
                                 ScriptableInterface *scriptable)
    : deleted_(false),
      js_context_(js_context),
      js_object_(js_object),
      scriptable_(scriptable),
      ondelete_connection_(NULL),
      ownership_policy_(ScriptableInterface::NATIVE_OWNED) {
  // Store this wrapper into the JSObject's private slot.
  JS_SetPrivate(js_context, js_object_, this);

  // Connect the "ondelete" callback.
  ondelete_connection_ = scriptable->ConnectToOnDeleteSignal(
      NewSlot(this, &NativeJSWrapper::OnDelete));
  ownership_policy_ = scriptable->Attach();

  // If the object is native owned, the script side should not delete the
  // object unless the native side tells it to do.
  if (ownership_policy_ == ScriptableInterface::NATIVE_OWNED ||
      ownership_policy_ == ScriptableInterface::NATIVE_PERMANENT)
    JS_AddRoot(js_context_, &js_object_);

#ifdef DEBUG_JS_WRAPPER_MEMORY
  DLOG("Wrap: policy=%d jsobj=%p wrapper=%p scriptable=%p(CLASS_ID=%jx)",
       ownership_policy_, js_object_, this,
       scriptable_, scriptable_->GetClassId());
  // This GC forces many hidden memory allocation errors to expose.
  DLOG("ForceGC");
  JS_GC(js_context_);
#endif
}

NativeJSWrapper::~NativeJSWrapper() {
  if (!deleted_) {
#ifdef DEBUG_JS_WRAPPER_MEMORY
    DLOG("Delete: policy=%d jsobj=%p wrapper=%p scriptable=%p(CLASS_ID=%jx)",
         ownership_policy_, js_object_, this,
         scriptable_, scriptable_->GetClassId());
#endif

    deleted_ = true;
    DetachJS();
    scriptable_->Detach();
  }
}

JSBool NativeJSWrapper::Unwrap(JSContext *cx, JSObject *obj,
                               ScriptableInterface **scriptable) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
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
                                                   JSObject *js_object) {
  if (js_object) {
    JSClass *cls = JS_GET_CLASS(cx, js_object);
    if (cls && cls->getProperty == wrapper_js_class_.getProperty &&
        cls->setProperty == wrapper_js_class_.setProperty) {
      ASSERT(cls->resolve == wrapper_js_class_.resolve &&
             cls->finalize == wrapper_js_class_.finalize);

      NativeJSWrapper *wrapper =
          reinterpret_cast<NativeJSWrapper *>(JS_GetPrivate(cx, js_object));
      if (!wrapper)
        // This is the prototype object created by JS_InitClass();
        return NULL;

      ASSERT(wrapper && wrapper->js_context_ == cx &&
             wrapper->js_object_ == js_object);
      return wrapper;
    }
  }

  JS_ReportError(cx, "Object is not a native wrapper");
  // The JSObject is not a JS wrapped ScriptableInterface object.
  return NULL;
}

JSBool NativeJSWrapper::CheckNotDeleted() {
  if (deleted_) {
    JS_ReportError(js_context_, "Native object has been deleted");
    return JS_FALSE;
  }
  return JS_TRUE;
}

JSBool NativeJSWrapper::CallWrapperSelf(JSContext *cx, JSObject *obj,
                                        uintN argc, jsval *argv,
                                        jsval *rval) {
  // In this case, the real self object being called is at argv[-2].
  JSObject *self_object = JSVAL_TO_OBJECT(argv[-2]);
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, self_object);
  return !wrapper ||
         (wrapper->CheckNotDeleted() &&
          wrapper->CallSelf(argc, argv, rval));
}

JSBool NativeJSWrapper::CallWrapperMethod(JSContext *cx, JSObject *obj,
                                          uintN argc, jsval *argv,
                                          jsval *rval) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  return !wrapper ||
         (wrapper->CheckNotDeleted() &&
          wrapper->CallMethod(argc, argv, rval));
}

JSBool NativeJSWrapper::GetWrapperPropertyDefault(JSContext *cx, JSObject *obj,
                                                  jsval id, jsval *vp) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  return !wrapper ||
         (wrapper->CheckNotDeleted() && wrapper->GetPropertyDefault(id, vp));
}

JSBool NativeJSWrapper::SetWrapperPropertyDefault(JSContext *cx, JSObject *obj,
                                                  jsval id, jsval *vp) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  return !wrapper ||
         (wrapper->CheckNotDeleted() && wrapper->SetPropertyDefault(id, *vp));
}

JSBool NativeJSWrapper::GetWrapperPropertyByIndex(JSContext *cx, JSObject *obj,
                                                  jsval id, jsval *vp) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  return !wrapper ||
         (wrapper->CheckNotDeleted() && wrapper->GetPropertyByIndex(id, vp));
}

JSBool NativeJSWrapper::SetWrapperPropertyByIndex(JSContext *cx, JSObject *obj,
                                                  jsval id, jsval *vp) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  return !wrapper ||
         (wrapper->CheckNotDeleted() && wrapper->SetPropertyByIndex(id, *vp));
}

JSBool NativeJSWrapper::GetWrapperPropertyByName(JSContext *cx, JSObject *obj,
                                                 jsval id, jsval *vp) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  return !wrapper ||
         (wrapper->CheckNotDeleted() && wrapper->GetPropertyByName(id, vp));
}

JSBool NativeJSWrapper::SetWrapperPropertyByName(JSContext *cx, JSObject *obj,
                                                 jsval id, jsval *vp) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  return !wrapper ||
         (wrapper->CheckNotDeleted() && wrapper->SetPropertyByName(id, *vp));
}

JSBool NativeJSWrapper::ResolveWrapperProperty(JSContext *cx, JSObject *obj,
                                               jsval id) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  return !wrapper ||
         (wrapper->CheckNotDeleted() && wrapper->ResolveProperty(id));
}

void NativeJSWrapper::FinalizeWrapper(JSContext *cx, JSObject *obj) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  if (wrapper) {
#ifdef DEBUG_JS_WRAPPER_MEMORY
    DLOG("Finalize: policy=%d jsobj=%p wrapper=%p scriptable=%p",
         wrapper->ownership_policy_, obj, wrapper, wrapper->scriptable_);
#endif

    if (!wrapper->deleted_)
      JSScriptContext::FinalizeNativeJSWrapper(cx, wrapper);
    delete wrapper;
  }
}

void NativeJSWrapper::DetachJS() {
#ifdef DEBUG_JS_WRAPPER_MEMORY
  DLOG("DetachJS: policy=%d jsobj=%p wrapper=%p scriptable=%p",
       ownership_policy_, js_object_, this, scriptable_);
#endif

  ondelete_connection_->Disconnect();
  if (ownership_policy_ == ScriptableInterface::NATIVE_OWNED ||
      ownership_policy_ == ScriptableInterface::NATIVE_PERMANENT)
    JS_RemoveRoot(js_context_, &js_object_);
}

void NativeJSWrapper::OnDelete() {
#ifdef DEBUG_JS_WRAPPER_MEMORY
  DLOG("OnDelete: policy=%d jsobj=%p wrapper=%p scriptable=%p",
       ownership_policy_, js_object_, this, scriptable_);
#endif

  deleted_ = true;

  // As the native side has deleted the object, now the script side can also
  // delete it.
  DetachJS();

  // Remove the wrapper mapping from the context, but leave this wrapper
  // alive to accept mistaken JavaScript calls gracefully.
  JSScriptContext::FinalizeNativeJSWrapper(js_context_, this);

#ifdef DEBUG_JS_WRAPPER_MEMORY
  // This GC forces many hidden memory allocation errors to expose.
  DLOG("ForceGC");
  JS_GC(js_context_);
#endif
}

JSBool NativeJSWrapper::CallSelf(uintN argc, jsval *argv, jsval *rval) {
  Variant prototype;
  int int_id;
  bool is_method;

  // Get the default method for this object.
  if (!scriptable_->GetPropertyInfoByName("", &int_id,
                                          &prototype, &is_method)) {
    JS_ReportError(js_context_, "Object can't be called as a function");
    return JS_FALSE;
  }

  if (!JSScriptContext::CheckException(js_context_, scriptable_))
    return JS_FALSE;

  ASSERT(is_method);
  return CallNativeSlot(VariantValue<Slot *>()(prototype), argc, argv, rval);
}

JSBool NativeJSWrapper::CallMethod(uintN argc, jsval *argv, jsval *rval) {
  // According to JS stack structure, argv[-2] is the current function object.
  JSObject *func_object = JSVAL_TO_OBJECT(argv[-2]);
  // Get the method slot from the reserved slot.
  jsval val;
  if (!JS_GetReservedSlot(js_context_, func_object, 0, &val) ||
      !JSVAL_IS_INT(val))
    return JS_FALSE;

  return CallNativeSlot(reinterpret_cast<Slot *>(JSVAL_TO_PRIVATE(val)),
                        argc, argv, rval);
}

JSBool NativeJSWrapper::CallNativeSlot(Slot *slot, uintN argc, jsval *argv,
                                       jsval *rval) {
  AutoLocalRootScope local_root_scope(js_context_);
  if (!local_root_scope.good())
    return JS_FALSE;

  Variant *params = NULL;
  uintN expected_argc = argc;
  if (!ConvertJSArgsToNative(js_context_, js_object_, slot, argc, argv,
                             &params, &expected_argc))
    return JS_FALSE;

  Variant return_value = slot->Call(expected_argc, params);
  if (!JSScriptContext::CheckException(js_context_, scriptable_)) {
    delete [] params;
    return JS_FALSE;
  }

  JSBool result = ConvertNativeToJS(js_context_, return_value, rval);
  if (!result)
    JS_ReportError(js_context_,
                   "Failed to convert native function result(%s) to jsval",
                   return_value.ToString().c_str());
  delete [] params;
  return result;
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

  AutoLocalRootScope local_root_scope(js_context_);
  if (!local_root_scope.good())
    return JS_FALSE;

  int int_id = JSVAL_TO_INT(id);
  Variant return_value = scriptable_->GetProperty(int_id);
  if (!ConvertNativeToJS(js_context_, return_value, vp)) {
    JS_ReportError(js_context_,
                   "Failed to convert native property value(%s) to jsval.",
                   return_value.ToString().c_str());
    return JS_FALSE;
  }

  return JSScriptContext::CheckException(js_context_, scriptable_);
}

JSBool NativeJSWrapper::SetPropertyByIndex(jsval id, jsval js_val) {
  if (!JSVAL_IS_INT(id))
    // Should not occur.
    return JS_FALSE;

  AutoLocalRootScope local_root_scope(js_context_);
  if (!local_root_scope.good())
    return JS_FALSE;

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
  if (!JSScriptContext::CheckException(js_context_, scriptable_))
    return JS_FALSE;

  ASSERT(!is_method);

  Variant value;
  if (!ConvertJSToNative(js_context_, js_object_, prototype, js_val,
                         &value)) {
    JS_ReportError(js_context_,
                   "Failed to convert JS property value(%s) to native",
                   PrintJSValue(js_context_, js_val).c_str());
    FreeNativeValue(value);
    return JS_FALSE;
  }

  if (!scriptable_->SetProperty(int_id, value)) {
    JS_ReportError(js_context_,
                   "Failed to set native property %s(%d) (may be readonly)",
                   name, int_id);
    FreeNativeValue(value);
    return JS_FALSE;
  }

  return JSScriptContext::CheckException(js_context_, scriptable_);
}

JSBool NativeJSWrapper::GetPropertyByName(jsval id, jsval *vp) {
  if (!JSVAL_IS_STRING(id))
    // Should not occur
    return JS_FALSE;

  JSString *idstr = JSVAL_TO_STRING(id);
  if (!idstr)
    return JS_FALSE;

  AutoLocalRootScope local_root_scope(js_context_);
  if (!local_root_scope.good())
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
  if (!JSScriptContext::CheckException(js_context_, scriptable_))
    return JS_FALSE;

  ASSERT(!is_method);

  Variant return_value = scriptable_->GetProperty(int_id);
  if (!JSScriptContext::CheckException(js_context_, scriptable_))
    return JS_FALSE;
  
  if (!ConvertNativeToJS(js_context_, return_value, vp)) {
    JS_ReportError(js_context_,
                   "Failed to convert native property value(%s) to jsval",
                   return_value.ToString().c_str());
    return JS_FALSE;
  }
  return JS_TRUE;
}

JSBool NativeJSWrapper::SetPropertyByName(jsval id, jsval js_val) {
  if (!JSVAL_IS_STRING(id))
    // Should not occur
    return JS_FALSE;

  JSString *idstr = JSVAL_TO_STRING(id);
  if (!idstr)
    return JS_FALSE;

  AutoLocalRootScope local_root_scope(js_context_);
  if (!local_root_scope.good())
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
  if (!JSScriptContext::CheckException(js_context_, scriptable_))
    return JS_FALSE;

  ASSERT(!is_method);

  Variant value;
  if (!ConvertJSToNative(js_context_, js_object_, prototype, js_val,
                         &value)) {
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

  return JSScriptContext::CheckException(js_context_, scriptable_);
}

JSBool NativeJSWrapper::ResolveProperty(jsval id) {
  if (!JSVAL_IS_STRING(id))
    return JS_TRUE;

  AutoLocalRootScope local_root_scope(js_context_);
  if (!local_root_scope.good())
    return JS_FALSE;

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

  if (!JSScriptContext::CheckException(js_context_, scriptable_))
    return JS_FALSE;

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

    if (int_id == ScriptableInterface::kConstantPropertyId)
      // This property is a constant, register a property with initial value
      // and without a tiny ID.  Then the JavaScript engine will handle it.
      return JS_DefineProperty(js_context_, js_object_, name,
                               js_val, JS_PropertyStub, JS_PropertyStub,
                               JSPROP_READONLY | JSPROP_PERMANENT);

    if (int_id == ScriptableInterface::kDynamicPropertyId)
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

} // namespace internal
} // namespace ggadget
