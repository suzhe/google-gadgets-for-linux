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

#include <ggadget/logger.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/signals.h>
#include <ggadget/slot.h>
#include <ggadget/string_utils.h>
#include "native_js_wrapper.h"
#include "converter.h"
#include "js_function_slot.h"
#include "js_script_context.h"

#ifdef _DEBUG
// Uncomment the following to debug wrapper memory.
// #define DEBUG_JS_WRAPPER_MEMORY
// #define DEBUG_FORCE_GC
#endif

#ifdef DEBUG_JS_WRAPPER_MEMORY
#include <jscntxt.h>
#include <jsdhash.h>
#endif

namespace ggadget {
namespace smjs {

// This JSClass is used to create wrapper JSObjects.
JSClass NativeJSWrapper::wrapper_js_class_ = {
  "NativeJSWrapper",
  // Use the private slot to store the wrapper.
  JSCLASS_HAS_PRIVATE | JSCLASS_NEW_ENUMERATE | JSCLASS_NEW_RESOLVE,
  JS_PropertyStub, JS_PropertyStub,
  GetWrapperPropertyDefault, SetWrapperPropertyDefault,
  reinterpret_cast<JSEnumerateOp>(EnumerateWrapper),
  reinterpret_cast<JSResolveOp>(ResolveWrapperProperty),
  JS_ConvertStub, FinalizeWrapper,
  NULL, NULL, CallWrapperSelf, NULL,
  NULL, NULL, MarkWrapper, NULL,
};

NativeJSWrapper::NativeJSWrapper(JSContext *js_context,
                                 JSObject *js_object,
                                 ScriptableInterface *scriptable)
    : js_context_(js_context),
      js_object_(js_object),
      scriptable_(NULL),
      on_reference_change_connection_(NULL) {
  ASSERT(js_object);

  // Store this wrapper into the JSObject's private slot.
  JS_SetPrivate(js_context, js_object_, this);

  if (scriptable)
    Wrap(scriptable);
}

NativeJSWrapper::~NativeJSWrapper() {
  if (scriptable_) {
#ifdef DEBUG_JS_WRAPPER_MEMORY
    DLOG("Delete: cx=%p jsobj=%p wrapper=%p scriptable=%s refcount=%d",
         js_context_, js_object_, this, name_.c_str(),
         scriptable_->GetRefCount());
#endif
    DetachJS(false);
  }
}

#ifdef DEBUG_JS_WRAPPER_MEMORY
JS_STATIC_DLL_CALLBACK(JSDHashOperator) PrintRoot(JSDHashTable *table,
                                                  JSDHashEntryHdr *hdr,
                                                  uint32 number, void *arg) {
  JSGCRootHashEntry *rhe = reinterpret_cast<JSGCRootHashEntry *>(hdr);
  jsval *rp = reinterpret_cast<jsval *>(rhe->root);
  DLOG("%d: name=%s address=%p value=%p",
       number, rhe->name, rp, JSVAL_TO_OBJECT(*rp));
  return JS_DHASH_NEXT;
}

static void DebugRoot(JSContext *cx) {
  DLOG("============== Roots ================");
  JSRuntime *rt = JS_GetRuntime(cx);
  JS_DHashTableEnumerate(&rt->gcRootsHash, PrintRoot, cx);
  DLOG("=========== End of Roots ============");
}
#else
#define DebugRoot(cx)
#endif

void NativeJSWrapper::Wrap(ScriptableInterface *scriptable) {
  ASSERT(scriptable && !scriptable_);
  scriptable_ = scriptable;
  name_ = StringPrintf("%p(CLASS_ID=%jx)",
                       scriptable, scriptable->GetClassId());

  if (scriptable->GetRefCount() > 0) {
    // There must be at least one native reference, let JavaScript know it
    // by adding the object to root.
#ifdef DEBUG_JS_WRAPPER_MEMORY
    DLOG("AddRoot: cx=%p jsobjaddr=%p jsobj=%p wrapper=%p scriptable=%s",
         js_context_, &js_object_, js_object_, this, name_.c_str());
#endif
    JS_AddNamedRoot(js_context_, &js_object_, name_.c_str());
    DebugRoot(js_context_);
  }
  scriptable->Ref();
  on_reference_change_connection_ = scriptable->ConnectOnReferenceChange(
      NewSlot(this, &NativeJSWrapper::OnReferenceChange));

#ifdef DEBUG_JS_WRAPPER_MEMORY
  DLOG("Wrap: cx=%p jsobj=%p wrapper=%p scriptable=%s refcount=%d",
       js_context_, js_object_, this, name_.c_str(), scriptable->GetRefCount());
#ifdef DEBUG_FORCE_GC
  // This GC forces many hidden memory allocation errors to expose.
  DLOG("ForceGC");
  JS_GC(js_context_);
#endif
#endif
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

      ASSERT(wrapper && wrapper->js_object_ == js_object);
      return wrapper;
    }
  }

  // The JSObject is not a JS wrapped ScriptableInterface object.
  return NULL;
}

JSBool NativeJSWrapper::CheckNotDeleted() {
  if (!scriptable_) {
    JS_ReportError(js_context_, "Native object has been deleted");
    return JS_FALSE;
  }
  return JS_TRUE;
}

JSBool NativeJSWrapper::CallWrapperSelf(JSContext *cx, JSObject *obj,
                                        uintN argc, jsval *argv,
                                        jsval *rval) {
  if (JS_IsExceptionPending(cx))
    return JS_FALSE;
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
  if (JS_IsExceptionPending(cx))
    return JS_FALSE;
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  return !wrapper ||
         (wrapper->CheckNotDeleted() &&
          wrapper->CallMethod(argc, argv, rval));
}

JSBool NativeJSWrapper::GetWrapperPropertyDefault(JSContext *cx, JSObject *obj,
                                                  jsval id, jsval *vp) {
  // Don't check exception here to let exception handling succeed.
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  return !wrapper ||
         (wrapper->CheckNotDeleted() && wrapper->GetPropertyDefault(id, vp));
}

JSBool NativeJSWrapper::SetWrapperPropertyDefault(JSContext *cx, JSObject *obj,
                                                  jsval id, jsval *vp) {
  // Don't check exception here to let exception handling succeed.
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  return !wrapper ||
         (wrapper->CheckNotDeleted() && wrapper->SetPropertyDefault(id, *vp));
}

JSBool NativeJSWrapper::GetWrapperPropertyByIndex(JSContext *cx, JSObject *obj,
                                                  jsval id, jsval *vp) {
  if (JS_IsExceptionPending(cx))
    return JS_FALSE;
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  return !wrapper ||
         (wrapper->CheckNotDeleted() && wrapper->GetPropertyByIndex(id, vp));
}

JSBool NativeJSWrapper::SetWrapperPropertyByIndex(JSContext *cx, JSObject *obj,
                                                  jsval id, jsval *vp) {
  if (JS_IsExceptionPending(cx))
    return JS_FALSE;
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  return !wrapper ||
         (wrapper->CheckNotDeleted() && wrapper->SetPropertyByIndex(id, *vp));
}

JSBool NativeJSWrapper::GetWrapperPropertyByName(JSContext *cx, JSObject *obj,
                                                 jsval id, jsval *vp) {
  if (JS_IsExceptionPending(cx))
    return JS_FALSE;
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  return !wrapper ||
         (wrapper->CheckNotDeleted() && wrapper->GetPropertyByName(id, vp));
}

JSBool NativeJSWrapper::SetWrapperPropertyByName(JSContext *cx, JSObject *obj,
                                                 jsval id, jsval *vp) {
  if (JS_IsExceptionPending(cx))
    return JS_FALSE;
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  return !wrapper ||
         (wrapper->CheckNotDeleted() && wrapper->SetPropertyByName(id, *vp));
}

JSBool NativeJSWrapper::EnumerateWrapper(JSContext *cx, JSObject *obj,
                                         JSIterateOp enum_op,
                                         jsval *statep, jsid *idp) {
  if (JS_IsExceptionPending(cx))
    return JS_FALSE;
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  return !wrapper ||
         // Don't CheckNotDeleted() when enum_op == JSENUMERATE_DESTROY
         // because we need this to clean up the resources allocated during
         // enumeration. NOTE: this may occur during GC.
         ((enum_op == JSENUMERATE_DESTROY || wrapper->CheckNotDeleted()) &&
          wrapper->Enumerate(enum_op, statep, idp));
}

JSBool NativeJSWrapper::ResolveWrapperProperty(JSContext *cx, JSObject *obj,
                                               jsval id, uintN flags,
                                               JSObject **objp) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  return !wrapper ||
         (wrapper->CheckNotDeleted() &&
          wrapper->ResolveProperty(id, flags, objp));
}

void NativeJSWrapper::FinalizeWrapper(JSContext *cx, JSObject *obj) {
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  if (wrapper) {
#ifdef DEBUG_JS_WRAPPER_MEMORY
    DLOG("Finalize: cx=%p jsobj=%p wrapper=%p scriptable=%s",
         cx, obj, wrapper, wrapper->name_.c_str());
#endif

    if (wrapper->scriptable_) {
      // The current context may be different from wrapper's context during
      // GC collecting. Use the wrapper's context instead.
      JSScriptContext::FinalizeNativeJSWrapper(wrapper->js_context_, wrapper);
    }

    for (JSFunctionSlots::iterator it = wrapper->js_function_slots_.begin();
         it != wrapper->js_function_slots_.end(); ++it)
      (*it)->Finalize();
    delete wrapper;
  }
}

uint32 NativeJSWrapper::MarkWrapper(JSContext *cx, JSObject *obj, void *arg) {
  // The current context may be different from wrapper's context during
  // GC marking.
  NativeJSWrapper *wrapper = GetWrapperFromJS(cx, obj);
  if (wrapper && wrapper->scriptable_)
    wrapper->Mark();
  return 0;
}

void NativeJSWrapper::DetachJS(bool caused_by_native) {
#ifdef DEBUG_JS_WRAPPER_MEMORY
  DLOG("DetachJS: cx=%p jsobj=%p wrapper=%p scriptable=%s refcount=%d",
       js_context_, js_object_, this, name_.c_str(),
       scriptable_->GetRefCount());
#endif

  on_reference_change_connection_->Disconnect();
  scriptable_->Unref(caused_by_native);
  scriptable_ = NULL;

  JS_RemoveRoot(js_context_, &js_object_);
  DebugRoot(js_context_);
}

void NativeJSWrapper::OnReferenceChange(int ref_count, int change) {
#ifdef DEBUG_JS_WRAPPER_MEMORY
  DLOG("OnReferenceChange(%d,%d): cx=%p jsobj=%p wrapper=%p scriptable=%s",
       ref_count, change, js_context_, js_object_, this, name_.c_str());
#endif

  if (ref_count == 0 && change == 0) {
    // Remove the wrapper mapping from the context, but leave this wrapper
    // alive to accept mistaken JavaScript calls gracefully.
    JSScriptContext::FinalizeNativeJSWrapper(js_context_, this);

    // As the native side is deleting the object, now the script side can also
    // delete it if there is no other active references. 
    DetachJS(true);

#ifdef DEBUG_JS_WRAPPER_MEMORY
#ifdef DEBUG_FORCE_GC
    // This GC forces many hidden memory allocation errors to expose.
    DLOG("ForceGC");
    JS_GC(js_context_);
#endif
#endif
  } else {
    ASSERT(change == 1 || change == -1);
    if (change == 1 && ref_count == 1) {
      // There must be at least one native reference, let JavaScript know it
      // by adding the object to root.
#ifdef DEBUG_JS_WRAPPER_MEMORY
      DLOG("AddRoot: cx=%p jsobjaddr=%p jsobj=%p wrapper=%p scriptable=%s",
           js_context_, &js_object_, js_object_, this, name_.c_str());
#endif
      JS_AddNamedRoot(js_context_, &js_object_, name_.c_str());
      DebugRoot(js_context_);
    } else if (change == -1 && ref_count == 2) {
      // The last native reference is about to be released, let JavaScript know
      // it by removing the root reference.
#ifdef DEBUG_JS_WRAPPER_MEMORY
      DLOG("RemoveRoot: cx=%p jsobjaddr=%p jsobj=%p wrapper=%p scriptable=%s",
           js_context_, &js_object_, js_object_, this, name_.c_str());
#endif
      JS_RemoveRoot(js_context_, &js_object_);
      DebugRoot(js_context_);
    }
  }
}

JSBool NativeJSWrapper::CallSelf(uintN argc, jsval *argv, jsval *rval) {
  ASSERT(scriptable_);

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
  return CallNativeSlot("DEFAULT", VariantValue<Slot *>()(prototype),
                        argc, argv, rval);
}

JSBool NativeJSWrapper::CallMethod(uintN argc, jsval *argv, jsval *rval) {
  ASSERT(scriptable_);

  // According to JS stack structure, argv[-2] is the current function object.
  jsval func_val = argv[-2];
  // Get the method slot from the reserved slot.
  jsval val;
  if (!JS_GetReservedSlot(js_context_, JSVAL_TO_OBJECT(func_val), 0, &val) ||
      !JSVAL_IS_INT(val))
    return JS_FALSE;

  const char *name = JS_GetFunctionName(JS_ValueToFunction(js_context_,
                                                           func_val));
  return CallNativeSlot(name, reinterpret_cast<Slot *>(JSVAL_TO_PRIVATE(val)),
                        argc, argv, rval);
}

JSBool NativeJSWrapper::CallNativeSlot(const char *name, Slot *slot,
                                       uintN argc, jsval *argv, jsval *rval) {
  ASSERT(scriptable_);

  AutoLocalRootScope local_root_scope(js_context_);
  if (!local_root_scope.good())
    return JS_FALSE;

  Variant *params = NULL;
  uintN expected_argc = argc;
  if (!ConvertJSArgsToNative(js_context_, this, name, slot, argc, argv,
                             &params, &expected_argc))
    return JS_FALSE;

  Variant return_value = slot->Call(expected_argc, params);
  delete [] params;
  params = NULL;

  if (!JSScriptContext::CheckException(js_context_, scriptable_))
    return JS_FALSE;

  JSBool result = ConvertNativeToJS(js_context_, return_value, rval);
  if (!result)
    JS_ReportError(js_context_,
                   "Failed to convert native function result(%s) to jsval",
                   return_value.Print().c_str());
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
  ASSERT(scriptable_);

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
  ASSERT(scriptable_);

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
                   "Failed to convert native property(%d) value(%s) to jsval.",
                   int_id, return_value.Print().c_str());
    return JS_FALSE;
  }

  return JSScriptContext::CheckException(js_context_, scriptable_);
}

JSBool NativeJSWrapper::SetPropertyByIndex(jsval id, jsval js_val) {
  ASSERT(scriptable_);

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
  if (!ConvertJSToNative(js_context_, this, prototype, js_val, &value)) {
    JS_ReportError(js_context_,
                   "Failed to convert JS property %s(%d) value(%s) to native.",
                   name, int_id, PrintJSValue(js_context_, js_val).c_str());
    return JS_FALSE;
  }

  if (!scriptable_->SetProperty(int_id, value)) {
    JS_ReportError(js_context_,
                   "Failed to set native property %s(%d) (may be readonly).",
                   name, int_id);
    FreeNativeValue(value);
    return JS_FALSE;
  }

  return JSScriptContext::CheckException(js_context_, scriptable_);
}

JSBool NativeJSWrapper::GetPropertyByName(jsval id, jsval *vp) {
  ASSERT(scriptable_);

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
    JS_ReportError(
        js_context_,
        "Failed to convert native property %s(%d) value(%s) to jsval",
        name, int_id, return_value.Print().c_str());
    return JS_FALSE;
  }
  return JS_TRUE;
}

JSBool NativeJSWrapper::SetPropertyByName(jsval id, jsval js_val) {
  ASSERT(scriptable_);

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
  if (!ConvertJSToNative(js_context_, this, prototype, js_val, &value)) {
    JS_ReportError(js_context_,
                   "Failed to convert JS property %s(%d) value(%s) to native.",
                   name, int_id, PrintJSValue(js_context_, js_val).c_str());
    return JS_FALSE;
  }

  if (!scriptable_->SetProperty(int_id, value)) {
    JS_ReportError(js_context_,
                   "Failed to set native property %s(%d) (may be readonly).",
                   name, int_id);
    FreeNativeValue(value);
    return JS_FALSE;
  }

  return JSScriptContext::CheckException(js_context_, scriptable_);
}

class NameCollector {
 public:
  NameCollector(std::vector<std::string> *names) : names_(names) { }
  bool Collect(int id, const char *name,
               const Variant &value, bool is_method) {
    names_->push_back(name);
    return true;
  }
  std::vector<std::string> *names_;
};

JSBool NativeJSWrapper::Enumerate(JSIterateOp enum_op,
                                  jsval *statep, jsid *idp) {
#ifdef GGADGET_SMJS_ENUMERATE_SUPPORTED
  std::vector<std::string> *properties;
  switch (enum_op) {
    case JSENUMERATE_INIT: {
      properties = new std::vector<std::string>;
      NameCollector collector(properties);
      scriptable_->EnumerateProperties(NewSlot(&collector,
                                               &NameCollector::Collect));
      *statep = PRIVATE_TO_JSVAL(properties);
      if (idp)
        JS_ValueToId(js_context_, INT_TO_JSVAL(properties->size()), idp);
      break;
    }
    case JSENUMERATE_NEXT:
      properties = reinterpret_cast<std::vector<std::string> *>(
          JSVAL_TO_PRIVATE(*statep));
      if (!properties->empty()) {
        const char *name = properties->begin()->c_str();
        jsval idval = STRING_TO_JSVAL(JS_NewStringCopyZ(js_context_, name));
        JS_ValueToId(js_context_, idval, idp);
        properties->erase(properties->begin());
        break;
      }
      // Fall Through!
    case JSENUMERATE_DESTROY:
      properties = reinterpret_cast<std::vector<std::string> *>(
          JSVAL_TO_PRIVATE(*statep));
      delete properties;
      *statep = JSVAL_NULL;
      break;
    default:
      return JS_FALSE;
  }
#else
  *statep = JSVAL_NULL;
  if (idp)
    *idp = JS_ValueToId(js_context_, INT_TO_JSVAL(0), idp);
#endif
  return JS_TRUE;
}

JSBool NativeJSWrapper::ResolveProperty(jsval id, uintN flags,
                                        JSObject **objp) {
  ASSERT(scriptable_);
  ASSERT(objp);
  *objp = NULL;

  if (!JSVAL_IS_STRING(id))
    return JS_TRUE;

  AutoLocalRootScope local_root_scope(js_context_);
  if (!local_root_scope.good())
    return JS_FALSE;

  JSString *idstr = JS_ValueToString(js_context_, id);
  if (!idstr)
    return JS_FALSE;
  const char *name = JS_GetStringBytes(idstr);

  // The JS program defines a new symbol. This has higher priority than the
  // properties of the global scriptable object. 
  if (flags & JSRESOLVE_DECLARING)
    return JS_TRUE;

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

    *objp = js_object_;
    return JS_TRUE;
  }

  // Define a JavaScript property.
  jsval js_val = JSVAL_VOID;
  *objp = js_object_;
  if (int_id == ScriptableInterface::kConstantPropertyId) {
    if (!ConvertNativeToJS(js_context_, prototype, &js_val)) {
      JS_ReportError(js_context_, "Failed to convert init value(%s) to jsval",
                     prototype.Print().c_str());
      return JS_FALSE;
    }
    // This property is a constant, register a property with initial value
    // and without a tiny ID.  Then the JavaScript engine will handle it.
    return JS_DefineProperty(js_context_, js_object_, name,
                             js_val, JS_PropertyStub, JS_PropertyStub,
                             JSPROP_READONLY | JSPROP_PERMANENT);
  }

  if (int_id == ScriptableInterface::kDynamicPropertyId) {
    return JS_DefineProperty(js_context_, js_object_, name, js_val,
                             GetWrapperPropertyByName,
                             SetWrapperPropertyByName,
                             JSPROP_SHARED);
  }

  if (int_id < 0 && int_id >= -128) {
    // Javascript tinyid is a 8-bit integer, and should be negative to avoid
    // conflict with array indexes.
    // This property is a normal property.  The 'get' and 'set' operations
    // will call back to native slots.
    return JS_DefinePropertyWithTinyId(js_context_, js_object_, name,
                                       static_cast<int8>(int_id), js_val,
                                       GetWrapperPropertyByIndex,
                                       SetWrapperPropertyByIndex,
                                       JSPROP_PERMANENT | JSPROP_SHARED);
  }

  // Too many properties, can't register all with tiny id.  The rest are
  // registered by name.
  return JS_DefineProperty(js_context_, js_object_, name, js_val,
                           GetWrapperPropertyByName,
                           SetWrapperPropertyByName,
                           JSPROP_PERMANENT | JSPROP_SHARED);
}

void NativeJSWrapper::AddJSFunctionSlot(JSFunctionSlot *slot) {
  js_function_slots_.insert(slot);
}

void NativeJSWrapper::RemoveJSFunctionSlot(JSFunctionSlot *slot) {
  js_function_slots_.erase(slot);
}

void NativeJSWrapper::Mark() {
#ifdef DEBUG_JS_WRAPPER_MEMORY
  DLOG("Mark: cx=%p jsobj=%p wrapper=%p scriptable=%s refcount=%d",
       js_context_, js_object_, this, name_.c_str(),
       scriptable_->GetRefCount());
#endif
  for (JSFunctionSlots::const_iterator it = js_function_slots_.begin();
       it != js_function_slots_.end(); ++it)
    (*it)->Mark();
}

} // namespace smjs
} // namespace ggadget
