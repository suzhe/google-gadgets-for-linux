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

#ifndef GGADGET_SMJS_NATIVE_JS_WRAPPER_H__
#define GGADGET_SMJS_NATIVE_JS_WRAPPER_H__

#include <set>
#include <jsapi.h>
#include <ggadget/common.h>
#include <ggadget/scriptable_interface.h>

namespace ggadget {

class Connection;

namespace smjs {

class JSFunctionSlot;

/**
 * A wrapper wrapping a native @c ScriptableInterface object into a
 * JavaScript object.
 */
class NativeJSWrapper {

 public:
  /**
   * Creation of a NativeJSWrapper can be in one or two steps.
   * Passing a non-NULL scriptable in the constructor creates the
   * @c NativeJSWrapper in one step. Passing a NULL scriptable in the
   * constructor, and then calling the Wrap method are two steps.
   */
  NativeJSWrapper(JSContext *js_context, JSObject *js_object,
                  ScriptableInterface *scriptable);
  ~NativeJSWrapper();
  void Wrap(ScriptableInterface *scriptable);

  /**
   * Unwrap a native @c ScriptableInterface object from a JavaScript object.
   * The JS object must have been returned from the above Wrap.
   * Return @c JS_FALSE on errors.
   */
  static JSBool Unwrap(JSContext *cx, JSObject *obj,
                       ScriptableInterface **scriptable);

  JSObject *&js_object() { return js_object_; }
  ScriptableInterface *scriptable() { return scriptable_; }
  ScriptableInterface::OwnershipPolicy ownership_policy() {
    return ownership_policy_;
  }

  static JSClass *GetWrapperJSClass() { return &wrapper_js_class_; }

  /** Gets the NativeJSWrapper pointer from a JS wrapped object. */
  static NativeJSWrapper *GetWrapperFromJS(JSContext *cx, JSObject *js_object);

  /** Detach the wrapper object from JavaScript so that the engine can GC it. */
  void DetachJS();

  /**
   * Registers an owned @c JSFunctionSlot.
   * It will be called when a JavaScript function is passed to one of the
   * methods or property setters. This object manages the references to these
   * functions.
   */
  void AddJSFunctionSlot(JSFunctionSlot *slot);
  void RemoveJSFunctionSlot(JSFunctionSlot *slot);

private:
  DISALLOW_EVIL_CONSTRUCTORS(NativeJSWrapper);

  void OnDelete();

  // Callback for invocation of the object itself as a function.
  static JSBool CallWrapperSelf(JSContext *cx, JSObject *obj,
                                uintN argc, jsval *argv, jsval *rval);
  static JSBool CallWrapperMethod(JSContext *cx, JSObject *obj,
                                  uintN argc, jsval *argv, jsval *rval);

  // This pair of methods handle all GetProperty and SetProperty callbacks
  // for system built-in properties, unknown properties or array indexes.
  static JSBool GetWrapperPropertyDefault(JSContext *cx, JSObject *obj,
                                          jsval id, jsval *vp);
  static JSBool SetWrapperPropertyDefault(JSContext *cx, JSObject *obj,
                                          jsval id, jsval *vp);

  // This pair of methods handle all GetProperty and SetProperty callbacks
  // for registered native properties with ids fitting in tinyid (-128>=id>0).
  static JSBool GetWrapperPropertyByIndex(JSContext *cx, JSObject *obj,
                                          jsval id, jsval *vp);
  static JSBool SetWrapperPropertyByIndex(JSContext *cx, JSObject *obj,
                                          jsval id, jsval *vp);

  // This pair of methods handle all GetProperty and SetProperty callbacks
  // for dynamic properties and registered native properties with ids not
  // fitting in tinyid (id<-128).
  static JSBool GetWrapperPropertyByName(JSContext *cx, JSObject *obj,
                                         jsval id, jsval *vp);
  static JSBool SetWrapperPropertyByName(JSContext *cx, JSObject *obj,
                                         jsval id, jsval *vp);
  static JSBool EnumerateWrapper(JSContext *cx, JSObject *obj,
                                 JSIterateOp enum_op, jsval *statep, jsid *idp);
  static JSBool ResolveWrapperProperty(JSContext *cx, JSObject *obj, jsval id,
                                       uintN flags, JSObject **objp);
  static void FinalizeWrapper(JSContext *cx, JSObject *obj);
  static uint32 MarkWrapper(JSContext *cx, JSObject *obj, void *arg);

  JSBool CheckNotDeleted();
  JSBool CallSelf(uintN argc, jsval *argv, jsval *rval);
  JSBool CallMethod(uintN argc, jsval *argv, jsval *rval);
  JSBool CallNativeSlot(const char *name, Slot *slot,
                        uintN argc, jsval *argv, jsval *rval);
  JSBool GetPropertyDefault(jsval id, jsval *vp);
  JSBool SetPropertyDefault(jsval id, jsval vp);
  JSBool GetPropertyByIndex(jsval id, jsval *vp);
  JSBool SetPropertyByIndex(jsval id, jsval vp);
  JSBool GetPropertyByName(jsval id, jsval *vp);
  JSBool SetPropertyByName(jsval id, jsval vp);
  JSBool Enumerate(JSIterateOp enum_op, jsval *statep, jsid *idp);
  JSBool ResolveProperty(jsval id, uintN flags, JSObject **objp);
  void Mark();

  static JSClass wrapper_js_class_;

  bool detached_;
  JSContext *js_context_;
  JSObject *js_object_;
  ScriptableInterface *scriptable_;
  Connection *ondelete_connection_;
  ScriptableInterface::OwnershipPolicy ownership_policy_;

  typedef std::set<JSFunctionSlot *> JSFunctionSlots;
  JSFunctionSlots js_function_slots_;
};

} // namespace smjs
} // namespace ggadget

#endif // GGADGET_SMJS_NATIVE_JS_WRAPPER_H__
