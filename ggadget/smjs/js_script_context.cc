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
  EvaluateScript(context_, JS_GetGlobalObject(context_), script,
                 filename, lineno, &rval);
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

static JSBool ReturnSelf(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                         jsval *rval) {
  *rval = OBJECT_TO_JSVAL(obj);
  return JS_TRUE;
}

static JSObject *GetClassPrototype(JSContext *cx, const char *class_name) {
  // Add some adapters for JScript.
  jsval ctor;
  if (!JS_GetProperty(cx, JS_GetGlobalObject(cx), class_name, &ctor) ||
      JSVAL_IS_NULL(ctor) || !JSVAL_IS_OBJECT(ctor))
    return NULL;

  jsval proto;
  if (!JS_GetProperty(cx, JSVAL_TO_OBJECT(ctor), "prototype", &proto) ||
      JSVAL_IS_NULL(proto) || !JSVAL_IS_OBJECT(proto))
    return NULL;

  return JSVAL_TO_OBJECT(proto);
}

static JSBool DoGC(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                   jsval *rval) {
  JS_GC(cx);
  return JS_TRUE;
}

bool JSScriptContext::SetGlobalObject(ScriptableInterface *global_object) {
  NativeJSWrapper *wrapper = WrapNativeObjectToJS(context_, global_object);
  JSObject *js_global = wrapper->js_object();
  if (!js_global)
    return false;
  if (!JS_InitStandardClasses(context_, js_global))
    return false;

  // Add some adapters for JScript.
  // We return JavaScript arrays where a VBArray is expected in original
  // JScript program. JScript program calls toArray() to convert a VBArray to
  // a JavaScript array. We just let toArray() return the array itself.
  JSObject *array_proto = GetClassPrototype(context_, "Array");
  JS_DefineFunction(context_, array_proto, "toArray", &ReturnSelf, 0, 0);
  // JScript programs call Date.getVarDate() to convert a JavaScript Date to
  // a COM VARDATE. We just use Date's where VARDATE's are expected by JScript
  // programs.
  JSObject *date_proto = GetClassPrototype(context_, "Date");
  JS_DefineFunction(context_, date_proto, "getVarDate", &ReturnSelf, 0, 0);
  // For Windows compatibility.
  JS_DefineFunction(context_, js_global, "CollectGarbage", &DoGC, 0, 0);

  return true;
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
  ASSERT_M(JS_GetGlobalObject(context_), ("Global object should be set first"));

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

bool JSScriptContext::AssignFromContext(ScriptableInterface *dest_object,
                                        const char *dest_object_expr,
                                        const char *dest_property,
                                        ScriptContextInterface *src_context,
                                        ScriptableInterface *src_object,
                                        const char *src_expr) {
  ASSERT(src_context);
  ASSERT(dest_property);
  AutoLocalRootScope scope(context_);

  jsval dest_val;
  if (!EvaluateToJSVal(dest_object, dest_object_expr, &dest_val) ||
      !JSVAL_IS_OBJECT(dest_val) || JSVAL_IS_NULL(dest_val)) {
    DLOG("Expression %s doesn't evaluate to a non-null object",
         dest_object_expr);
    return false;
  }
  JSObject *dest_js_object = JSVAL_TO_OBJECT(dest_val);

  jsval src_val;
  JSScriptContext *src_js_context = down_cast<JSScriptContext *>(src_context);
  AutoLocalRootScope scope1(src_js_context->context_);
  if (!src_js_context->EvaluateToJSVal(src_object, src_expr, &src_val))
    return false;

  return JS_SetProperty(context_, dest_js_object, dest_property, &src_val);
}

bool JSScriptContext::AssignFromNative(ScriptableInterface *object,
                                       const char *object_expr,
                                       const char *property,
                                       const Variant &value) {
  ASSERT(property);
  AutoLocalRootScope scope(context_);

  jsval dest_val;
  if (!EvaluateToJSVal(object, object_expr, &dest_val) ||
      !JSVAL_IS_OBJECT(dest_val) || JSVAL_IS_NULL(dest_val)) {
    DLOG("Expression %s doesn't evaluate to a non-null object", object_expr);
    return false;
  }
  JSObject *js_object = JSVAL_TO_OBJECT(dest_val);

  jsval src_val;
  if (!ConvertNativeToJS(context_, value, &src_val))
    return false;
  return JS_SetProperty(context_, js_object, property, &src_val);
}

Variant JSScriptContext::Evaluate(ScriptableInterface *object,
                                  const char *expr) {
  Variant result;
  jsval js_val;
  if (EvaluateToJSVal(object, expr, &js_val)) {
    ConvertJSToNativeVariant(context_, js_val, &result);
    // Just left result in void state on any error.
  }
  return result;
}

JSBool JSScriptContext::EvaluateToJSVal(ScriptableInterface *object,
                                        const char *expr, jsval *result) {
  *result = JSVAL_VOID;
  JSObject *js_object;
  if (object) {
    NativeJSWrapperMap::const_iterator it = native_js_wrapper_map_.find(object);
    if (it == native_js_wrapper_map_.end()) {
      DLOG("Object %p hasn't a wrapper in JS", object);
      return JS_FALSE;
    }
    js_object = it->second->js_object();
  } else {
    js_object = JS_GetGlobalObject(context_);
  }

  if (expr && *expr) {
    if (!EvaluateScript(context_, js_object, expr, expr, 1, result)) {
      DLOG("Failed to evaluate dest_object_expr %s against JSObject %p",
           expr, js_object);
      return JS_FALSE;
    }
  } else {
    *result = OBJECT_TO_JSVAL(js_object);
  }
  return JS_TRUE;
}

} // namespace smjs
} // namespace ggadget
