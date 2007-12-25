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
#include <ggadget/slot.h>
#include <ggadget/unicode_utils.h>
#include "js_script_context.h"
#include "converter.h"
#include "js_function_slot.h"
#include "js_native_wrapper.h"
#include "js_script_runtime.h"
#include "native_js_wrapper.h"

namespace ggadget {
namespace smjs {

JSScriptContext::JSScriptContext(JSScriptRuntime *runtime, JSContext *context)
    : runtime_(runtime),
      context_(context),
      lineno_(0) {
  JS_SetContextPrivate(context_, this);
  // JS_SetOptions(context_, JS_GetOptions(context_) | JSOPTION_STRICT);
}

JSScriptContext::~JSScriptContext() {
  // Don't report errors during shutdown because the state may be inconsistent.
  JS_SetErrorReporter(context_, NULL);
  // Remove the return value protection reference.
  // See comments in WrapJSObjectToNative() for details.  
  JS_DeleteProperty(context_, JS_GetGlobalObject(context_),
                    kGlobalReferenceName);

  // Force a GC to make it possible to check if there are leaks.
  JS_GC(context_);

  while (!native_js_wrapper_map_.empty()) {
    NativeJSWrapperMap::iterator it = native_js_wrapper_map_.begin();
    NativeJSWrapper *wrapper = it->second;
    if (wrapper->ownership_policy() != ScriptableInterface::NATIVE_PERMANENT) {
      DLOG("POSSIBLE LEAK (Use NATIVE_PERMANENT if possible and it's not a real"
           " leak): policy=%d jsobj=%p wrapper=%p scriptable=%p(CLASS_ID=%jx)",
           wrapper->ownership_policy(), wrapper->js_object(), wrapper,
           wrapper->scriptable(), wrapper->scriptable()->GetClassId());
    }

    // Inform the wrapper to detach from JavaScript so that it can be GC'ed.
    native_js_wrapper_map_.erase(it);
    wrapper->DetachJS();
  }

  JS_DestroyContext(context_);
  context_ = NULL;

  for (ClassVector::iterator it = registered_classes_.begin();
       it != registered_classes_.end(); ++it)
    delete *it;
  registered_classes_.clear();
}

static JSScriptContext *GetJSScriptContext(JSContext *context) {
  return reinterpret_cast<JSScriptContext *>(JS_GetContextPrivate(context));
}

// As we want to depend on only the public SpiderMonkey APIs, the only
// way to get the current filename and lineno is from the JSErrorReport.
void JSScriptContext::RecordFileAndLine(JSContext *cx, const char *message,
                                        JSErrorReport *report) {
  JSScriptContext *context = GetJSScriptContext(cx);
  if (context) {
    context->filename_ = report->filename ? report->filename : "";
    context->lineno_ = report->lineno;
  }
}

void JSScriptContext::GetCurrentFileAndLineInternal(std::string *filename,
                                                    int *lineno) {
  filename_.clear();
  lineno_ = 0;
  JSErrorReporter old_reporter = JS_SetErrorReporter(context_,
                                                     RecordFileAndLine);
  // Let the JavaScript engine call RecordFileAndLine.
  JS_ReportError(context_, "");
  JS_SetErrorReporter(context_, old_reporter);
  *filename = filename_;
  *lineno = lineno_;
}

void JSScriptContext::GetCurrentFileAndLine(JSContext *context,
                                            std::string *filename,
                                            int *lineno) {
  ASSERT(filename && lineno);
  JSScriptContext *context_wrapper = GetJSScriptContext(context);
  if (context_wrapper) {
    context_wrapper->GetCurrentFileAndLineInternal(filename, lineno);
  } else {
    filename->clear();
    *lineno = 0;
  }
}

NativeJSWrapper *JSScriptContext::WrapNativeObjectToJSInternal(
    JSObject *js_object, NativeJSWrapper *wrapper,
    ScriptableInterface *scriptable) {
  ASSERT(scriptable);
  NativeJSWrapperMap::const_iterator it =
      native_js_wrapper_map_.find(scriptable);
  if (it == native_js_wrapper_map_.end()) {
    if (!js_object)
      js_object = JS_NewObject(context_, NativeJSWrapper::GetWrapperJSClass(),
                               NULL, NULL);
    if (!js_object)
      return NULL;

    if (!wrapper)
      wrapper = new NativeJSWrapper(context_, js_object, scriptable);
    else
      wrapper->Wrap(scriptable);

    native_js_wrapper_map_[scriptable] = wrapper;
    ASSERT(wrapper->scriptable() == scriptable);
    return wrapper;
  } else {
    ASSERT(!wrapper);
    ASSERT(!js_object);
    return it->second;
  }
}

NativeJSWrapper *JSScriptContext::WrapNativeObjectToJS(
    JSContext *cx, ScriptableInterface *scriptable) {
  JSScriptContext *context_wrapper = GetJSScriptContext(cx);
  ASSERT(context_wrapper);
  if (context_wrapper)
    return context_wrapper->WrapNativeObjectToJSInternal(NULL, NULL,
                                                         scriptable);
  return NULL;
}

void JSScriptContext::FinalizeNativeJSWrapperInternal(
    NativeJSWrapper *wrapper) {
  native_js_wrapper_map_.erase(wrapper->scriptable());
}

void JSScriptContext::FinalizeNativeJSWrapper(JSContext *cx,
                                              NativeJSWrapper *wrapper) {
  JSScriptContext *context_wrapper = GetJSScriptContext(cx);
  ASSERT(context_wrapper);
  if (context_wrapper)
    context_wrapper->FinalizeNativeJSWrapperInternal(wrapper);
}

JSNativeWrapper *JSScriptContext::WrapJSToNativeInternal(JSObject *obj) {
  ASSERT(obj);
  jsval js_val = OBJECT_TO_JSVAL(obj);
  JSNativeWrapper *wrapper;
  JSNativeWrapperMap::const_iterator it = js_native_wrapper_map_.find(obj);
  if (it == js_native_wrapper_map_.end()) {
    wrapper = new JSNativeWrapper(context_, obj);
    js_native_wrapper_map_[obj] = wrapper;
    return wrapper;
  } else {
    wrapper = it->second;
  }

  // Set the wrapped object as a property of the global object to prevent it
  // from being unexpectedly GC'ed before the native side receives it.
  // This is useful when returning the object to the native side.
  // If this object is passed via native slot parameters, because the objects
  // are also protected by the JS stack, there is no problem of property
  // overwriting.
  // The native side can call Attach() if it wants to hold the wrapper object.
  JS_DefineProperty(context_, JS_GetGlobalObject(context_),
                    kGlobalReferenceName, js_val, NULL, NULL, 0);
  return wrapper;
}

JSNativeWrapper *JSScriptContext::WrapJSToNative(JSContext *cx,
                                                 JSObject *obj) {
  JSScriptContext *context_wrapper = GetJSScriptContext(cx);
  ASSERT(context_wrapper);
  if (context_wrapper)
    return context_wrapper->WrapJSToNativeInternal(obj);
  return NULL;
}

void JSScriptContext::FinalizeJSNativeWrapperInternal(
    JSNativeWrapper *wrapper) {
  js_native_wrapper_map_.erase(wrapper->js_object());
}

void JSScriptContext::FinalizeJSNativeWrapper(JSContext *cx,
                                              JSNativeWrapper *wrapper) {
  JSScriptContext *context_wrapper = GetJSScriptContext(cx);
  ASSERT(context_wrapper);
  if (context_wrapper)
    context_wrapper->FinalizeJSNativeWrapperInternal(wrapper);
}

JSBool JSScriptContext::CheckException(JSContext *cx,
                                       ScriptableInterface *scriptable) {
  ScriptableInterface *exception = scriptable->GetPendingException(true);
  if (!exception)
    return JS_TRUE;

  jsval js_exception;
  if (!ConvertNativeToJS(cx, Variant(exception), &js_exception)) {
    JS_ReportError(cx, "Failed to convert native exception to jsval");
    return JS_FALSE;
  }

  JS_SetPendingException(cx, js_exception);
  return JS_FALSE;
}

void JSScriptContext::Destroy() {
  runtime_->DestroyContext(this);
}

void JSScriptContext::Execute(const char *script,
                              const char *filename,
                              int lineno) {
  jsval rval;
  EvaluateScript(context_, script, filename, lineno, &rval);
}

Slot *JSScriptContext::Compile(const char *script,
                               const char *filename,
                               int lineno) {
  JSFunction *function = CompileFunction(context_, script, filename, lineno);
  if (!function)
    return NULL;

  return new JSFunctionSlot(NULL, context_, NULL,
                            JS_GetFunctionObject(function));
}

bool JSScriptContext::SetGlobalObject(ScriptableInterface *global_object) {
  NativeJSWrapper *wrapper = WrapNativeObjectToJS(context_, global_object);
  JSObject *js_global = wrapper->js_object();
  if (!js_global)
    return false;
  return JS_InitStandardClasses(context_, js_global);
}

JSScriptContext::JSClassWithNativeCtor::JSClassWithNativeCtor(
    const char *name, Slot *constructor)
    : constructor_(constructor) {
  js_class_ = *NativeJSWrapper::GetWrapperJSClass();
  js_class_.name = name;
}

JSScriptContext::JSClassWithNativeCtor::~JSClassWithNativeCtor() {
  delete constructor_;
  constructor_ = NULL;
}

JSBool JSScriptContext::ConstructObject(JSContext *cx, JSObject *obj,
                                        uintN argc, jsval *argv, jsval *rval) {
  AutoLocalRootScope local_root_scope(cx);
  if (!local_root_scope.good())
    return JS_FALSE;

  JSClassWithNativeCtor *cls =
      reinterpret_cast<JSClassWithNativeCtor *>(JS_GET_CLASS(cx, obj));
  ASSERT(cls);

  // Create a wrapper first which doesn't really wrap a scriptable object,
  // because it is not available before the constructor is called.
  // This wrapper is important if there are any JavaScript callbacks in the
  // constructor argument list.
  NativeJSWrapper *wrapper = new NativeJSWrapper(cx, obj, NULL);
  Variant *params = NULL;
  uintN expected_argc = argc;
  if (!ConvertJSArgsToNative(cx, wrapper, cls->js_class_.name,
                             cls->constructor_, argc, argv,
                             &params, &expected_argc))
    return JS_FALSE;

  Variant return_value = cls->constructor_->Call(expected_argc, params);
  ASSERT(return_value.type() == Variant::TYPE_SCRIPTABLE);
  ScriptableInterface *scriptable =
      VariantValue<ScriptableInterface *>()(return_value);

  JSScriptContext *context_wrapper = GetJSScriptContext(cx);
  ASSERT(context_wrapper);
  if (context_wrapper)
    context_wrapper->WrapNativeObjectToJSInternal(obj, wrapper, scriptable);
  delete [] params;
  return JS_TRUE;
}

bool JSScriptContext::RegisterClass(const char *name, Slot *constructor) {
  ASSERT(constructor);
  ASSERT(constructor->GetReturnType() == Variant::TYPE_SCRIPTABLE);

  JSClassWithNativeCtor *cls = new JSClassWithNativeCtor(name, constructor);
  if (!JS_InitClass(context_, JS_GetGlobalObject(context_), NULL,
                    &cls->js_class_, &ConstructObject,
                    constructor->GetArgCount(),
                    NULL, NULL, NULL, NULL)) {
    delete cls;
    return false;
  }

  registered_classes_.push_back(cls);
  return true;
}

void JSScriptContext::LockObject(ScriptableInterface *object,
                                 const char *name) {
  ASSERT(object);
  NativeJSWrapperMap::const_iterator it = native_js_wrapper_map_.find(object);
  if (it == native_js_wrapper_map_.end()) {
    DLOG("Can't lock %p(CLASS_ID=%jx) not attached to JavaScript",
         object, object->GetClassId());
  } else {
    DLOG("Lock: policy=%d jsobj=%p wrapper=%p scriptable=%p",
         it->second->ownership_policy(), it->second->js_object(),
         it->second, it->second->scriptable());
    JS_AddNamedRoot(context_, &(it->second->js_object()), name);
  }
}

void JSScriptContext::UnlockObject(ScriptableInterface *object) {
  ASSERT(object);
  NativeJSWrapperMap::const_iterator it = native_js_wrapper_map_.find(object);
  if (it == native_js_wrapper_map_.end()) {
    DLOG("Can't unlock %p not attached to JavaScript", object);
  } else {
    DLOG("Unlock: policy=%d jsobj=%p wrapper=%p scriptable=%p",
         it->second->ownership_policy(), it->second->js_object(),
         it->second, it->second->scriptable());
    JS_RemoveRoot(context_, &(it->second->js_object()));
  }
}

bool JSScriptContext::AssignFromContext(ScriptableInterface *dest_object,
                                        const char *dest_object_expr,
                                        const char *dest_property,
                                        ScriptContextInterface *src_context,
                                        ScriptableInterface *src_object,
                                        const char *src_expr) {
  ASSERT(src_context);
  ASSERT(dest_property);
  ASSERT(src_expr);
  JSScriptContext *src_js_context = down_cast<JSScriptContext *>(src_context);

  JSObject *dest_js_object;
  if (dest_object) {
    NativeJSWrapperMap::const_iterator it =
        native_js_wrapper_map_.find(dest_object);
    if (it == native_js_wrapper_map_.end())
      return false;
    dest_js_object = it->second->js_object();
  } else {
    dest_js_object = JS_GetGlobalObject(context_);
  }

  if (dest_object_expr && *dest_object_expr) {
    UTF16String utf16_dest_object_expr;
    ConvertStringUTF8ToUTF16(dest_object_expr, strlen(dest_object_expr),
                             &utf16_dest_object_expr);
    jsval rval;
    if (!JS_EvaluateUCScript(context_, dest_js_object,
                             utf16_dest_object_expr.c_str(),
                             utf16_dest_object_expr.size(),
                             "", 1, &rval)) {
      DLOG("Failed to evaluate dest_object_expr %s against JSObject %p",
           dest_object_expr, dest_js_object);
      return false;
    }
    if (!JSVAL_IS_OBJECT(rval) || JSVAL_IS_NULL(rval)) {
      DLOG("Expression %s doesn't evaluate to a non-null object",
           dest_object_expr);
      return false;
    }

    dest_js_object = JSVAL_TO_OBJECT(rval);
  }

  JSObject *src_js_object;
  if (src_object) {
    NativeJSWrapperMap::const_iterator it =
        src_js_context->native_js_wrapper_map_.find(src_object);
    if (it == src_js_context->native_js_wrapper_map_.end())
      return false;
    src_js_object = it->second->js_object();
  } else {
    src_js_object = JS_GetGlobalObject(src_js_context->context_);
  }

  UTF16String utf16_src_expr;
  ConvertStringUTF8ToUTF16(src_expr, strlen(src_expr), &utf16_src_expr);
  jsval src_val;
  if (!JS_EvaluateUCScript(src_js_context->context_, src_js_object,
                           utf16_src_expr.c_str(), utf16_src_expr.size(),
                           "", 1, &src_val)) {
    DLOG("Failed to evaluate src_expr %s against JSObject %p",
         src_expr, src_js_object);
    return false;
  }

  return JS_SetProperty(context_, dest_js_object, dest_property, &src_val);
}

} // namespace smjs
} // namespace ggadget
