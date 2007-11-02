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
#include "ggadget/scriptable_interface.h"
#include "ggadget/script_context_interface.h"
#include "ggadget/signal.h"

namespace ggadget {

class NativeJSWrapper;

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
  static JSObject *WrapNativeObjectToJS(JSContext *cx,
                                        ScriptableInterface *scriptable);

  /**
   * Called when JavaScript engine is to finalized a JavaScript object wrapper
   */
  static void FinalizeNativeJSWrapper(JSContext *cx, NativeJSWrapper *wrapper);

  /**
   * Convert a native @c Slot into a JavaScript function object (in jsval).
   * @param cx JavaScript context.
   * @param slot the native @c Slot.
   * @return the converted JavaScript function object (in jsval).
   *     Return @c JSVAL_NULL if @a slot is not previously returned from
   *     @c NewJSFunctionSlot.
   */
  static jsval ConvertSlotToJS(JSContext *cx, Slot *slot);

  /**
   * Calls a native slot from the JavaScript side.
   */
  static JSBool CallNativeSlot(JSContext *cx, JSObject *obj,
                               uintN argc, jsval *argv, jsval *rval);

  /**
   * Handles a native exception and throws it into the script engine.
   */
  static JSBool HandleException(JSContext *cx,
                                const ScriptableExceptionHolder &e);

  /**
   * Create a @c Slot that is targeted to a JavaScript function object.
   * @param cx JavaScript context.
   * @param prototype another @c Slot acting as the prototype that has
   *     compatible parameter list and return value.  Can be @c NULL.
   * @param function_val a JavaScript function object (in jsval).
   */
  static Slot *NewJSFunctionSlot(JSContext *cx,
                                 const Slot *prototype,
                                 jsval function_val);

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

 private:
  DISALLOW_EVIL_CONSTRUCTORS(JSScriptContext);
  friend class JSScriptRuntime;
  JSScriptContext(JSContext *context);
  virtual ~JSScriptContext();

  void GetCurrentFileAndLineInternal(const char **filename, int *lineno);
  JSObject *WrapNativeObjectToJSInternal(
      JSObject *js_object, ScriptableInterface *scriptableInterface);
  jsval ConvertSlotToJSInternal(Slot *slot);
  Slot *NewJSFunctionSlotInternal(const Slot *prototype,
                                  jsval function_val);
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
  // Native slot to JavaScript function object (in jsval) map.
  typedef std::map<Slot *, jsval> SlotJSMap;
  SlotJSMap slot_js_map_;

  typedef std::vector<JSClassWithNativeCtor *> ClassVector;
  ClassVector registered_classes_;
};

} // namespace ggadget

#endif  // GGADGET_JS_SCRIPT_CONTEXT_H__
