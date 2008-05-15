/*
  Copyright 2008 Google Inc.

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

#ifndef GGADGET_SMJS_JS_SCRIPT_CONTEXT_H__
#define GGADGET_SMJS_JS_SCRIPT_CONTEXT_H__

#include <map>
#include <string>
#include <vector>
#include <jsapi.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/script_context_interface.h>
#include <ggadget/signals.h>

namespace ggadget {
namespace smjs {

class JSScriptRuntime;
class NativeJSWrapper;
class JSNativeWrapper;
class JSFunctionSlot;

/**
 * The name of the global object property to temporarily protect a JavaScript
 * value from being GC'ed. Its differences from JS_AddRoot() are
 *     - It doesn't need to clean up (like JS_RemoveRoot());
 *     - It is overwritable, so the protection only applicable temporarily
 *       after a JavaScript invocation from the native side.
 */
const char *const kGlobalReferenceName = "[[[GlobalReference]]]";

/**
 * @c ScriptContext implementation for SpiderMonkey JavaScript engine.
 */
class JSScriptContext : public ScriptContextInterface {
 public:
  JSScriptContext(JSScriptRuntime *runtime, JSContext *context);
  virtual ~JSScriptContext();

  /**
   * Get the current filename and line number of this @c JSScriptContext.
   */
  static void GetCurrentFileAndLine(JSContext *cx,
                                    std::string *filename,
                                    int *lineno);

  /**
   * Wrap a native @c ScriptableInterface object into a JavaScript object.
   * If the object has already been wrapped, returns the existing wrapper.
   * The caller must immediately hook the object in the JS object tree to
   * prevent it from being unexpectedly GC'ed.
   * @param cx JavaScript context.
   * @param scriptable the native @c ScriptableInterface object to be wrapped.
   * @return the wrapped JavaScript object, or @c NULL on errors.
   */
  static NativeJSWrapper *WrapNativeObjectToJS(JSContext *cx,
                                               ScriptableInterface *scriptable);

  /**
   * Called when JavaScript engine is to finalized a JavaScript object wrapper
   */
  static void FinalizeNativeJSWrapper(JSContext *cx, NativeJSWrapper *wrapper);

  /**
   * Wrap a @c JSObject into a @c JSNativeWrapper or a JS function into a
   * @c JSFunctionNativeWrapper.
   * If the object has already been wrapped, returns the existing wrapper.
   * @param cx JavaScript context.
   * @param object the JS object or function to be wrapped.
   * @return the wrapped native object, or @c NULL on errors.
   */
  static JSNativeWrapper *WrapJSToNative(JSContext *cx, JSObject *obj);

  /**
   * Called when @c JSNativeWrapper or @c JSFunctionNativeWrapper is about
   * to be deleted.
   */
  static void FinalizeJSNativeWrapper(JSContext *cx, JSNativeWrapper *wrapper);

  /**
   * Checks if there is pending exception. If there is, handles it and throws
   * it into the script engine.
   */
  static JSBool CheckException(JSContext *cx, ScriptableInterface *scriptable);

  JSContext *context() const { return context_; }

  /** @see ScriptContextInterface::Destroy() */
  virtual void Destroy();
  /** @see ScriptContextInterface::Execute() */
  virtual void Execute(const char *script,
                       const char *filename,
                       int lineno);
  /** @see ScriptContextInterface::Compile() */
  virtual Slot *Compile(const char *script,
                        const char *filename,
                        int lineno);
  /** @see ScriptContextInterface::SetGlobalObject() */
  virtual bool SetGlobalObject(ScriptableInterface *global_object);
  /** @see ScriptContextInterface::RegisterClass() */
  virtual bool RegisterClass(const char *name, Slot *constructor);
  /** @see ScriptContextInterface::AssignFromContext() */
  virtual bool AssignFromContext(ScriptableInterface *dest_object,
                                 const char *dest_object_expr,
                                 const char *dest_property,
                                 ScriptContextInterface *src_context,
                                 ScriptableInterface *src_object,
                                 const char *src_expr);
  /** @see ScriptContextInterface::AssignFromContext() */
  virtual bool AssignFromNative(ScriptableInterface *object,
                                const char *object_expr,
                                const char *property,
                                const Variant &value);
  /** @see ScriptContextInterface::Evaluate() */
  virtual Variant Evaluate(ScriptableInterface *object, const char *expr);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(JSScriptContext);

  void GetCurrentFileAndLineInternal(std::string *filename, int *lineno);
  NativeJSWrapper *WrapNativeObjectToJSInternal(
      JSObject *js_object, NativeJSWrapper *wrapper,
      ScriptableInterface *scriptable);
  void FinalizeNativeJSWrapperInternal(NativeJSWrapper *wrapper);
  JSNativeWrapper *WrapJSToNativeInternal(JSObject *js_object);
  void FinalizeJSNativeWrapperInternal(JSNativeWrapper *wrapper);

  const char *JSValToString(jsval js_val);

  /**
   * This function is a JSErrorReporter that is used by
   * @c GetCurrentFileAndLine().
   * As we don't want to depend on only the public SpiderMonkey APIs, the only
   * way to get the current filename and lineno is from the JSErrorReport.
   */
  static void RecordFileAndLine(JSContext *cx, const char *message,
                                JSErrorReport *report);

  /** Callback function for native classes. */
  static JSBool ConstructObject(JSContext *cx, JSObject *obj,
                                uintN argc, jsval *argv, jsval *rval);

  JSBool EvaluateToJSVal(ScriptableInterface *object, const char *expr,
                         jsval *result);

  class JSClassWithNativeCtor {
   public:
    JSClassWithNativeCtor(const char *name, Slot *constructor);
    ~JSClassWithNativeCtor();
    JSClass js_class_;
    Slot *constructor_;
   private:
    DISALLOW_EVIL_CONSTRUCTORS(JSClassWithNativeCtor);
  };

  JSScriptRuntime *runtime_;
  JSContext *context_;
  // The following two fields are only used during GetCurrentFileAndLine.
  std::string filename_;
  int lineno_;

  typedef std::map<ScriptableInterface *, NativeJSWrapper *> NativeJSWrapperMap;
  NativeJSWrapperMap native_js_wrapper_map_;

  typedef std::map<JSObject *, JSNativeWrapper *> JSNativeWrapperMap;
  JSNativeWrapperMap js_native_wrapper_map_;

  typedef std::vector<JSClassWithNativeCtor *> ClassVector;
  ClassVector registered_classes_;
};

/**
 * This class is used in JavaScript callback functions to ensure that the local
 * newly created JavaScript objects won't be GC'ed during the callbacks.
 */
class AutoLocalRootScope {
 public:
  AutoLocalRootScope(JSContext *cx)
      : cx_(cx), good_(JS_EnterLocalRootScope(cx)) { }
  ~AutoLocalRootScope() { if (good_) JS_LeaveLocalRootScope(cx_); }
  JSBool good() { return good_; }
 private:
  JSContext *cx_;
  JSBool good_;
};

} // namespace smjs
} // namespace ggadget

#endif  // GGADGET_SMJS_JS_SCRIPT_CONTEXT_H__
