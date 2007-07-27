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
#include <jsapi.h>
#include "ggadget/script_context_interface.h"

namespace ggadget {

class NativeJSWrapper;

/**
 * @c ScriptRuntime implementation for SpiderMonkey JavaScript engine.
 */
class JSScriptRuntime : public ScriptRuntimeInterface {
 public:
  JSScriptRuntime();

  /** @see ScriptRuntimeInterface::CreateContext() */
  virtual ScriptContextInterface *CreateContext();
  /** @see ScriptRuntimeInterface::DestroyContext() */
  virtual void DestroyContext(ScriptContextInterface *context);
  /** @see ScriptRuntimeInterface::Destroy() */
  virtual void Destroy();
 private:
  DISALLOW_EVIL_CONSTRUCTORS(JSScriptRuntime);
  ~JSScriptRuntime() { }
  JSRuntime *runtime_;
};

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
  static JSObject *WrapNativeObjectToJS(
      JSContext *cx, ScriptableInterface *scriptableInterface);

  /**
   * Convert a native @c Slot into a @c JSFunction.
   * <ul>
   * <li>If @a slot is a @c JSFunctionSlot (a private class defined in
   * js_script_context.cc, this method returns the original @c JSFunction.</li>
   * <li>Otherwise, the @a slot is wrapped into a @c JSFunction.</li>
   * </ul>
   * @param cx JavaScript context.
   * @param slot the native @c Slot to be wrapped.
   * @return the converted JavaScript function.
   */
  static JSFunction *ConvertNativeSlotToJS(JSContext *cx, Slot *slot);

  /**
   * The callback of a @c JSFunction that wraps a native @c Slot.
   * It conforms to @c JSNative.
   */
  static JSBool CallNativeSlot(JSContext *cx, JSObject *obj,
                               uintN argc, jsval *argv,
                               jsval *rval);

  /**
   * Create a @c Slot that is  targeted to a @c JSFunction.
   * @param cx JavaScript context.
   * @param prototype another @c Slot acting as the prototype that has
   *     compatible parameter list and return value.  Can be @c NULL.
   * @param function a JavaScript function.
   */
  static Slot *NewJSFunctionSlot(JSContext *cx,
                                 const Slot *prototype,
                                 JSFunction *js_function);

  JSContext *context() const { return context_; }

  /** @see ScriptContextInterface::Compile() */
  virtual Slot *Compile(const char *script,
                        const char *filename,
                        int lineno);
  /** @see ScriptContextInterface::SetValue() */
  virtual bool SetValue(const char *object_expression,
                        const char *property_name,
                        Variant value);
  /** @see ScriptContextInterface::SetGlobalObject() */
  virtual bool SetGlobalObject(ScriptableInterface *global_object);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(JSScriptContext);
  friend class JSScriptRuntime;
  JSScriptContext(JSContext *context);
  ~JSScriptContext();

  void GetCurrentFileAndLineInternal(const char **filename, int *lineno);
  JSObject *WrapNativeObjectToJSInternal(
      ScriptableInterface *scriptableInterface);
  JSFunction *ConvertNativeSlotToJSInternal(Slot *slot);
  Slot *NewJSFunctionSlotInternal(const Slot *prototype,
                                  JSFunction *js_function);

  const char *JSValToString(jsval js_val);

  /**
   * This function is a JSErrorReporter that is used by
   * @c GetCurrentFileAndLine().
   * As we don't want to depend on only the public SpiderMonkey APIs, the only
   * way to get the current filename and lineno is from the JSErrorReport.
   */
  static void FileAndLineRecorder(JSContext *cx, const char *message,
                                  JSErrorReport *report);

  JSContext *context_;
  // The following two fields are only used during GetCurrentFileAndLine.
  const char *filename_;
  int lineno_;

  typedef std::map<ScriptableInterface *, NativeJSWrapper *> WrapperMap;
  WrapperMap wrapper_map_;
  // Native slot to JavaScript function object map.
  typedef std::map<Slot *, JSObject *> SlotJSMap;
  SlotJSMap slot_js_map_;
};

} // namespace ggadget

#endif  // GGADGET_JS_SCRIPT_CONTEXT_H__
