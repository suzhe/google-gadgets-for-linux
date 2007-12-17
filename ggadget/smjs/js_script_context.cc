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

#include <ggadget/common.h>
#include <ggadget/slot.h>
#include <ggadget/unicode_utils.h>
#include "js_script_context.h"
#include "converter.h"
#include "native_js_wrapper.h"
#include "js_function_slot.h"
#include "js_script_runtime.h"

namespace ggadget {
namespace smjs {

JSScriptContext::JSScriptContext(JSContext *context)
    : context_(context),
      filename_(NULL),
      lineno_(0) {
  JS_SetContextPrivate(context_, this);
  // JS_SetOptions(context_, JS_GetOptions(context_) | JSOPTION_STRICT);
}

JSScriptContext::~JSScriptContext() {
  // Force a GC to make it possible to check if there are leaks.
  JS_GC(context_);

  for (WrapperMap::iterator it = wrapper_map_.begin();
       it != wrapper_map_.end();
       // NOTE: not ++it, but the following to ensure safe map operation.
       it = wrapper_map_.begin()) {
    NativeJSWrapper *wrapper = it->second;
    if (wrapper->ownership_policy() != ScriptableInterface::NATIVE_PERMANENT) {
      DLOG("POSSIBLE LEAK (Use NATIVE_PERMANENT if it's not a real leak):"
           "policy=%d jsobj=%p wrapper=%p scriptable=%p(CLASS_ID=%jx)",
           wrapper->ownership_policy(), wrapper->js_object(), wrapper,
           wrapper->scriptable(), wrapper->scriptable()->GetClassId());
    }

    // Inform the wrapper to detach from JavaScript so that it can be GC'ed.
    wrapper_map_.erase(it);
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
    context->filename_ = report->filename;
    context->lineno_ = report->lineno;
  }
}

void JSScriptContext::GetCurrentFileAndLineInternal(const char **filename,
                                                    int *lineno) {
  filename_ = NULL;
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
                                            const char **filename,
                                            int *lineno) {
  JSScriptContext *context_wrapper = GetJSScriptContext(context);
  if (context_wrapper) {
    context_wrapper->GetCurrentFileAndLineInternal(filename, lineno);
  } else {
    *filename = NULL;
    *lineno = 0;
  }
}

NativeJSWrapper *JSScriptContext::WrapNativeObjectToJSInternal(
    JSObject *js_object, NativeJSWrapper *wrapper,
    ScriptableInterface *scriptable) {
  ASSERT(scriptable);
  WrapperMap::const_iterator it = wrapper_map_.find(scriptable);
  if (it == wrapper_map_.end()) {
    if (!js_object)
      js_object = JS_NewObject(context_, NativeJSWrapper::GetWrapperJSClass(),
                               NULL, NULL);
    if (!js_object)
      return NULL;

    if (!wrapper)
      wrapper = new NativeJSWrapper(context_, js_object, scriptable);
    else
      wrapper->Wrap(scriptable);

    wrapper_map_[scriptable] = wrapper;
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
  wrapper_map_.erase(wrapper->scriptable());
}

void JSScriptContext::FinalizeNativeJSWrapper(JSContext *cx,
                                              NativeJSWrapper *wrapper) {
  JSScriptContext *context_wrapper = GetJSScriptContext(cx);
  ASSERT(context_wrapper);
  if (context_wrapper)
    context_wrapper->FinalizeNativeJSWrapperInternal(wrapper);
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
  delete this;
}

void JSScriptContext::Execute(const char *script,
                              const char *filename,
                              int lineno) {
  UTF16String utf16_string;
  ConvertStringUTF8ToUTF16(script, strlen(script), &utf16_string);
  jsval rval;
  JS_EvaluateUCScript(context_, JS_GetGlobalObject(context_),
                      utf16_string.c_str(), utf16_string.size(),
                      filename, lineno, &rval);
}

Slot *JSScriptContext::Compile(const char *script,
                               const char *filename,
                               int lineno) {
  UTF16String utf16_string;
  ConvertStringUTF8ToUTF16(script, strlen(script), &utf16_string);
  JSFunction *function = JS_CompileUCFunction(
      context_, NULL, NULL, 0, NULL,  // No name and no argument.
      // Don't cast utf16_string.c_str() to jschar *, to let the compiler check
      // if they are compatible.
      utf16_string.c_str(), utf16_string.size(),
      filename, lineno);
  if (!function)
    return NULL;

  return new JSFunctionSlot(
      NULL, context_, NULL, OBJECT_TO_JSVAL(JS_GetFunctionObject(function)));
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
  if (!ConvertJSArgsToNative(cx, wrapper, cls->constructor_, argc, argv,
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

void JSScriptContext::LockObject(ScriptableInterface *object) {
  ASSERT(object);
  WrapperMap::const_iterator it = wrapper_map_.find(object);
  if (it == wrapper_map_.end()) {
    DLOG("Can't lock %p(CLASS_ID=%jx) not attached to JavaScript",
         object, object->GetClassId());
  } else {
    DLOG("Lock: policy=%d jsobj=%p wrapper=%p scriptable=%p",
         it->second->ownership_policy(), it->second->js_object(),
         it->second, it->second->scriptable());
    JS_AddRoot(context_, &(it->second->js_object()));
  }
}

void JSScriptContext::UnlockObject(ScriptableInterface *object) {
  ASSERT(object);
  WrapperMap::const_iterator it = wrapper_map_.find(object);
  if (it == wrapper_map_.end()) {
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
    WrapperMap::const_iterator it = wrapper_map_.find(dest_object);
    if (it == wrapper_map_.end())
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
    WrapperMap::const_iterator it =
        src_js_context->wrapper_map_.find(src_object);
    if (it == src_js_context->wrapper_map_.end())
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
