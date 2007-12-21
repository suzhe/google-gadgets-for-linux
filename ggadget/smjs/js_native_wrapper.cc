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

#include "js_native_wrapper.h"
#include "js_script_context.h"
#include "native_js_wrapper.h"
#include "converter.h"

namespace ggadget {
namespace smjs {

static const char *kGlobalReferenceName = "[[[GlobalReference]]]";
static const char *kWrapperReferenceName = "[[[WrapperReference]]]";

JSNativeWrapper::JSNativeWrapper(JSContext *js_context, JSObject *js_object)
    : js_context_(js_context),
      js_object_(js_object) {
  SetDynamicPropertyHandler(NewSlot(this, &JSNativeWrapper::GetProperty),
                            NewSlot(this, &JSNativeWrapper::SetProperty));
  SetArrayHandler(NewSlot(this, &JSNativeWrapper::GetElement),
                  NewSlot(this, &JSNativeWrapper::SetElement));

  // Set the object as a property of the global object to prevent it from
  // being GC'ed. This is useful when returning the object to the native side.
  // The native side can't hold the object, because the property may be
  // overwrite by later such objects.
  // For native slot parameters, because the objects are also protected by
  // the JS stack, there is no problem of property overwriting.
  // TODO: Use more sophisticated method (e.g. reference counting).
  jsval js_val = OBJECT_TO_JSVAL(js_object);
  JS_SetProperty(js_context, JS_GetGlobalObject(js_context),
                 kGlobalReferenceName, &js_val);

  // Wrap this object again into a JS object, and add this object as a property
  // of the original object, to make it possible to auto clean up this object
  // when the original object is finalized.
  NativeJSWrapper *wrapper = JSScriptContext::WrapNativeObjectToJS(js_context,
                                                                   this);
  js_val = OBJECT_TO_JSVAL(wrapper->js_object());
  JS_SetProperty(js_context, js_object, kWrapperReferenceName, &js_val);
}

JSNativeWrapper::~JSNativeWrapper() {
}

Variant JSNativeWrapper::GetProperty(const char *name) {
  Variant result;
  jsval rval;
  if (JS_GetProperty(js_context_, js_object_, name, &rval) &&
      !ConvertJSToNativeVariant(js_context_, rval, &result)) {
    JS_ReportError(js_context_,
                   "Failed to convert JS property %s value(%s) to native.",
                   name, PrintJSValue(js_context_, rval).c_str());
  }
  return result;
}

bool JSNativeWrapper::SetProperty(const char *name, const Variant &value) {
  jsval js_val;
  if (!ConvertNativeToJS(js_context_, value, &js_val)) {
    JS_ReportError(js_context_,
                   "Failed to convert native property %s value(%s) to jsval.",
                   name, value.Print().c_str());
    return false;
  }
  return JS_SetProperty(js_context_, js_object_, name, &js_val);
}

Variant JSNativeWrapper::GetElement(int index) {
  Variant result;
  jsval rval;
  if (JS_GetElement(js_context_, js_object_, index, &rval) &&
      !ConvertJSToNativeVariant(js_context_, rval, &result)) {
    JS_ReportError(js_context_,
                   "Failed to convert JS property %d value(%s) to native.",
                   index, PrintJSValue(js_context_, rval).c_str());
  }
  return result;
}

bool JSNativeWrapper::SetElement(int index, const Variant &value) {
  jsval js_val;
  if (!ConvertNativeToJS(js_context_, value, &js_val)) {
    JS_ReportError(js_context_,
                   "Failed to convert native property %d value(%s) to jsval.",
                   index, value.Print().c_str());
    return false;
  }
  return JS_SetElement(js_context_, js_object_, index, &js_val);
}

} // namespace smjs
} // namespace ggadget
