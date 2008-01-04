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
#include "converter.h"
#include "js_script_context.h"

namespace ggadget {
namespace smjs {

// This JSClass is used to create the reference tracker JSObjects.
JSClass JSNativeWrapper::js_reference_tracker_class_ = {
  "JSReferenceTracker",
  // Use the private slot to store the wrapper.
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, FinalizeTracker,
  JSCLASS_NO_OPTIONAL_MEMBERS
};

static const char *kTrackerReferenceName = "[[[TrackerReference]]]";

JSNativeWrapper::JSNativeWrapper(JSContext *js_context, JSObject *js_object)
    : ref_count_(0),
      js_context_(js_context),
      js_object_(js_object) {
  SetDynamicPropertyHandler(NewSlot(this, &JSNativeWrapper::GetProperty),
                            NewSlot(this, &JSNativeWrapper::SetProperty));
  SetArrayHandler(NewSlot(this, &JSNativeWrapper::GetElement),
                  NewSlot(this, &JSNativeWrapper::SetElement));

  // Wrap this object again into a JS object, and add this object as a property
  // of the original object, to make it possible to auto detach this object
  // when the original object is finalized.
  JSObject *js_reference_tracker = JS_NewObject(js_context,
                                                &js_reference_tracker_class_,
                                                NULL, NULL);
  JS_DefineProperty(js_context, js_object, kTrackerReferenceName,
                    OBJECT_TO_JSVAL(js_reference_tracker),
                    NULL, NULL, JSPROP_READONLY | JSPROP_PERMANENT);
  JS_SetPrivate(js_context, js_reference_tracker, this);
  // Count the current JavaScript reference.
  Attach();
  ASSERT(GetRefCount() == 1);
}

JSNativeWrapper::~JSNativeWrapper() {
  JSScriptContext::FinalizeJSNativeWrapper(js_context_, this);
}

ScriptableInterface::OwnershipPolicy JSNativeWrapper::Attach() {
  if (GetRefCount() > 0) {
    // There must be a new native reference, let JavaScript know it by adding
    // the object to root.
    JS_AddRoot(js_context_, &js_object_);
  }
  return ScriptableHelperOwnershipShared::Attach();
}

bool JSNativeWrapper::Detach() {
  if (GetRefCount() == 2) {
    // The last native reference is about to be released, let JavaScript know
    // it by removing the root reference.
    JS_RemoveRoot(js_context_, &js_object_);
  }
  return ScriptableHelperOwnershipShared::Detach();
}

bool JSNativeWrapper::EnumerateProperties(
    EnumeratePropertiesCallback *callback) {
  ASSERT(callback);
  bool result = true;
  JSIdArray *id_array = JS_Enumerate(js_context_, js_object_);
  if (id_array) {
    for (int i = 0; i < id_array->length; i++) {
      jsid id = id_array->vector[i];
      jsval key = JSVAL_VOID;
      JS_IdToValue(js_context_, id, &key);
      if (JSVAL_IS_STRING(key)) {
        char *name = JS_GetStringBytes(JSVAL_TO_STRING(key));
        if (name &&
            !(*callback)(kDynamicPropertyId, name, GetProperty(name), false)) {
          result = false;
          break;
        }
      }
      // Otherwise, ignore the property.
    }
  }
  JS_DestroyIdArray(js_context_, id_array);
  delete callback;
  return result;
}

bool JSNativeWrapper::EnumerateElements(EnumerateElementsCallback *callback) {
  ASSERT(callback);
  bool result = true;
  JSIdArray *id_array = JS_Enumerate(js_context_, js_object_);
  if (id_array) {
    for (int i = 0; i < id_array->length; i++) {
      jsid id = id_array->vector[i];
      jsval key = JSVAL_VOID;
      JS_IdToValue(js_context_, id, &key);
      if (JSVAL_IS_INT(key)) {
        int index = JSVAL_TO_INT(key);
        if (!(*callback)(index, GetElement(index))) {
          result = false;
          break;
        }
      }
      // Otherwise, ignore the property.
    }
  }
  JS_DestroyIdArray(js_context_, id_array);
  delete callback;
  return result;
}

Variant JSNativeWrapper::GetProperty(const char *name) {
  Variant result;
  jsval rval;
  if (JS_GetProperty(js_context_, js_object_, name, &rval) &&
      !ConvertJSToNativeVariant(js_context_, NULL, rval, &result)) {
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
      !ConvertJSToNativeVariant(js_context_, NULL, rval, &result)) {
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

void JSNativeWrapper::FinalizeTracker(JSContext *cx, JSObject *obj) {
  if (obj) {
    JSClass *cls = JS_GET_CLASS(cx, obj);
    if (cls && cls->finalize == js_reference_tracker_class_.finalize) {
      JSNativeWrapper *wrapper =
          reinterpret_cast<JSNativeWrapper *>(JS_GetPrivate(cx, obj));
      if (wrapper) {
        // The JavaScript reference should be the last reference to be released,
        // because the object is added to root if there are native references.
        ASSERT(wrapper->GetRefCount() == 1);
        wrapper->Detach();
      }
    }
  }
}

} // namespace smjs
} // namespace ggadget
