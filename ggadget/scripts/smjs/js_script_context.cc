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
namespace internal {

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

JSObject *JSScriptContext::WrapNativeObjectToJSInternal(
    JSObject *js_object, ScriptableInterface *scriptable) {
  ASSERT(scriptable);
  WrapperMap::const_iterator it = wrapper_map_.find(scriptable);
  if (it == wrapper_map_.end()) {
    if (!js_object)
      js_object = JS_NewObject(context_, NativeJSWrapper::GetWrapperJSClass(),
                               NULL, NULL);
    if (!js_object)
      return NULL;

    NativeJSWrapper *wrapper = new NativeJSWrapper(context_,
                                                   js_object,
                                                   scriptable);
    wrapper_map_[scriptable] = wrapper;
    ASSERT(wrapper->scriptable() == scriptable);
    return wrapper->js_object();
  } else {
    ASSERT(!js_object);
    return it->second->js_object();
  }
}

JSObject *JSScriptContext::WrapNativeObjectToJS(
    JSContext *cx, ScriptableInterface *scriptable) {
  JSScriptContext *context_wrapper = GetJSScriptContext(cx);
  ASSERT(context_wrapper);
  if (context_wrapper)
    return context_wrapper->WrapNativeObjectToJSInternal(NULL, scriptable);
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

jsval JSScriptContext::ConvertSlotToJSInternal(Slot *slot) {
  ASSERT(slot);
  SlotJSMap::const_iterator it = slot_js_map_.find(slot);
  if (it != slot_js_map_.end()) {
    // If found, it->second JavaScript function object that has been wrapped
    // into a JSFunctionSlot.
    return it->second;
  }
  // We don't allow JavaScript to call native slot in this way.
  return JSVAL_NULL;
}

jsval JSScriptContext::ConvertSlotToJS(JSContext *cx, Slot *slot) {
  JSScriptContext *context_wrapper = GetJSScriptContext(cx);
  ASSERT(context_wrapper);
  if (context_wrapper)
    return context_wrapper->ConvertSlotToJSInternal(slot);
  return JSVAL_NULL;
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

JSFunctionSlot *JSScriptContext::NewJSFunctionSlotInternal(
    const Slot *prototype, jsval function_val) {
  JSFunctionSlot *slot = new JSFunctionSlot(prototype, context_, function_val);
  // Put it here to make it possible for ConvertNativeSlotToJS to unwrap
  // a JSFunctionSlot.
  slot_js_map_[slot] = function_val;
  return slot;
}

JSFunctionSlot *JSScriptContext::NewJSFunctionSlot(JSContext *cx,
                                                   const Slot *prototype,
                                                   jsval function_val) {
  JSScriptContext *context_wrapper = GetJSScriptContext(cx);
  ASSERT(context_wrapper);
  if (context_wrapper)
    return context_wrapper->NewJSFunctionSlotInternal(prototype, function_val);
  return NULL;
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
      NULL, context_, OBJECT_TO_JSVAL(JS_GetFunctionObject(function)));
}

bool JSScriptContext::SetGlobalObject(ScriptableInterface *global_object) {
  JSObject *js_global = WrapNativeObjectToJS(context_, global_object);
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

  Variant *params = NULL;
  uintN expected_argc = argc;
  if (!ConvertJSArgsToNative(cx, obj, cls->constructor_, argc, argv,
                             &params, &expected_argc))
    return JS_FALSE;

  Variant return_value = cls->constructor_->Call(expected_argc, params);
  ASSERT(return_value.type() == Variant::TYPE_SCRIPTABLE);
  ScriptableInterface *scriptable =
      VariantValue<ScriptableInterface *>()(return_value);
  
  JSScriptContext *context_wrapper = GetJSScriptContext(cx);
  ASSERT(context_wrapper);
  if (context_wrapper)
    context_wrapper->WrapNativeObjectToJSInternal(obj, scriptable);
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

} // namespace internal
} // namespace ggadget
