// Copyright 2007 Google Inc.  All Rights Reserved.
// Author: wangxianzhu@google.com (Xianzhu Wang)

#ifndef GGADGET_NATIVE_JS_WRAPPER_H__
#define GGADGET_NATIVE_JS_WRAPPER_H__

#include <jsapi.h>

namespace ggadget {

class ScriptableInterface;

/**
 * A wrapper wrapping a native @c ScriptableInterface object into a
 * JavaScript object.
 */
class NativeJSWrapper {

 public:
  ~NativeJSWrapper();

  /**
   * Wrap a native @c ScriptableInterface object into a JavaScript object.
   * The caller must immediately hook the object in the JS object tree to
   * prevent it from being unexpectedly GC'ed.
   * @param cx JavaScript context.
   * @param scriptable the native @c ScriptableInterface object to be wrapped.
   * @return the wrapped JavaScript object, or @c NULL on errors.
   */
  static JSObject *Wrap(JSContext *cx,
                        ScriptableInterface *scriptableInterface);

  /**
   * Unwrap a native @c ScriptableInterface object from a JavaScript object.
   * The JS object must have been returned from the above Wrap.
   * Return @c JS_FALSE on errors.
   */
  static JSBool Unwrap(JSContext *cx, JSObject *obj,
                       ScriptableInterface **scriptable);

  /**
   * Get the @c NativeJSWrapper pointer from a JS wrapped
   * @c ScriptableInterface object.
   */
  static NativeJSWrapper *NativeJSWrapper::GetWrapperFromJS(
      JSContext *cx, JSObject *js_object);

  /**
   * Invoke a native method.
   */
  JSBool InvokeMethod(uintN argc, jsval *argv, jsval *rval);
  /**
   * Get a native property.
   */
  JSBool GetProperty(jsval id, jsval *vp);
  /**
   * Set a native property.
   */
  JSBool SetProperty(jsval id, jsval vp);
  /**
   * Resolve a property.
   */
  JSBool ResolveProperty(jsval id);

private:
  NativeJSWrapper(JSContext *js_context, ScriptableInterface *scriptable);
  void OnDelete() { deleted_ = true; }

  bool deleted_;
  JSContext *js_context_;
  JSObject *js_object_;
  ScriptableInterface *scriptable_;
};

} // namespace ggadget

#endif // GGADGET_NATIVE_JS_WRAPPER_H__
