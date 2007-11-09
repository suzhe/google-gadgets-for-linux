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

#ifndef GGADGET_JS_SCRIPT_CONTEXT_H__
#define GGADGET_JS_SCRIPT_CONTEXT_H__

#include <map>
#include <vector>
#include <jsapi.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/script_context_interface.h>
#include <ggadget/signals.h>

namespace ggadget {

class JSScriptRuntime;

namespace internal {

class NativeJSWrapper;
class JSFunctionSlot;

/**
 * @c ScriptContext implementation for SpiderMonkey JavaScript engine.
 */
class JSScriptContext : public ScriptContextInterface {
 public:
  /**
   * Get the current filename and line number of this @c JSScriptContext.
   */
  static void GetCurrentFileAndLine(JSContext *cx,
                                    const char **filename,
                                    int *lineno);

  /**
   * Wrap a native @c ScriptableInterface object into a JavaScript object.
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
  /** @see ScriptContextInterface::LockObject() */
  virtual void LockObject(ScriptableInterface *object);
  /** @see ScriptContextInterface::UnlockObject() */
  virtual void UnlockObject(ScriptableInterface *object);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(JSScriptContext);
  friend class ::ggadget::JSScriptRuntime;
  JSScriptContext(JSContext *context);
  virtual ~JSScriptContext();

  void GetCurrentFileAndLineInternal(const char **filename, int *lineno);
  NativeJSWrapper *WrapNativeObjectToJSInternal(
      JSObject *js_object, NativeJSWrapper *wrapper,
      ScriptableInterface *scriptable);
  void FinalizeNativeJSWrapperInternal(NativeJSWrapper *wrapper);

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

  class JSClassWithNativeCtor {
   public:
    JSClassWithNativeCtor(const char *name, Slot *constructor);
    ~JSClassWithNativeCtor();
    JSClass js_class_;
    Slot *constructor_;
   private:
    DISALLOW_EVIL_CONSTRUCTORS(JSClassWithNativeCtor);
  };

  JSContext *context_;
  // The following two fields are only used during GetCurrentFileAndLine.
  const char *filename_;
  int lineno_;

  typedef std::map<ScriptableInterface *, NativeJSWrapper *> WrapperMap;
  WrapperMap wrapper_map_;

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

} // namespace internal
} // namespace ggadget

#endif  // GGADGET_JS_SCRIPT_CONTEXT_H__
