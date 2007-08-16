// Copyright 2007 Google Inc. All Rights Reserved.
// Author: wangxianzhu@google.com (Xianzhu Wang)

/**
 * This header file defines interfaces between C++ and script engines.
 */

#ifndef GGADGET_SCRIPT_CONTEXT_INTERFACE_H__
#define GGADGET_SCRIPT_CONTEXT_INTERFACE_H__

#include "common.h"
#include "variant.h"

namespace ggadget {

class ScriptContextInterface;
class Slot;
template <typename R, typename P1> class Slot1;
class Connection;

/**
 * The script engine runtime.
 * Normally there is one @c ScriptRuntimeInterface instance in a process
 * for each script engine.
 */
class ScriptRuntimeInterface {
 public:
  virtual ~ScriptRuntimeInterface() { }

  /**
   * Create a new @c ScriptContextInterface instance.
   * Must call @c DestroyContext after use.
   * @return the created context.
   */
  virtual ScriptContextInterface *CreateContext() = 0;

  /**
   * An @c ErrorReporter can be connected to the error reporter signal.
   * It will receive a message string when it is called.
   */ 
  typedef Slot1<void, const char *> ErrorReporter;

  /**
   * Connect a error reporter to the error reporter signal.
   * After connected, the reporter will receive all Script error reports.
   * @param reporter the error reporter.
   * @return the signal @c Connection.
   */
  virtual Connection *ConnectErrorReporter(ErrorReporter *reporter) = 0;
};

/**
 * The context of script compilation and execution.
 * All script related compilation and execution must occur in a
 * @c ScriptContext instance.
 */
class ScriptContextInterface {
 protected:
  /**
   * Disallow irect deletion. 
   */
  virtual ~ScriptContextInterface() { }

 public:
  /**
   * Destroy a context after use.
   */
  virtual void Destroy() = 0;

  /**
   * Compile a script fragment in the context.
   * @param script the script source code.
   * @param filename the name of the file containing the @a script.
   * @param lineno the line number of the @a script in the file.
   * @return a compiled @c ScriptFunction instance, or @c NULL on error.
   *     The caller then owns the @c Slot pointer.
   */
  virtual Slot *Compile(const char *script,
                        const char *filename,
                        int lineno) = 0;

  /**
   * Set the property value of a JavaScript object.
   * @param object_expression a JavaScript expression that can be evaluated
   *     to a JavaScript object.  If it is @c NULL or blank, the object is the
   *     global object of this @c ScriptContextInterface.
   * @param property_name the name of the property to set the value.
   * @param value the value.
   * @return @c true if succeeds.
   */
  virtual bool SetValue(const char *object_expression,
                        const char *property_name,
                        Variant value) = 0;

  /**
   * Set the global object of the context.
   * @param global_object the global object of the new context.
   * return @c true if succeeds.
   */
  virtual bool SetGlobalObject(ScriptableInterface *global_object) = 0;
};

} // namespace ggadget

#endif // GGADGET_SCRIPT_CONTEXT_INTERFACE_H__
