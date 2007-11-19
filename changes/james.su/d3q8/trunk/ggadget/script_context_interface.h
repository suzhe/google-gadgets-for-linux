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

#ifndef GGADGET_SCRIPT_CONTEXT_INTERFACE_H__
#define GGADGET_SCRIPT_CONTEXT_INTERFACE_H__

#include <ggadget/variant.h>

namespace ggadget {

class Slot;

/**
 * The context of script compilation and execution.
 * All script related compilation and execution must occur in a
 * @c ScriptContext instance.
 */
class ScriptContextInterface {
 protected:
  /**
   * Disallow direct deletion.
   */
  virtual ~ScriptContextInterface() { }

 public:
  /**
   * Destroys a context after use.
   */
  virtual void Destroy() = 0;

  /**
   * Compiles and execute a script fragment in the context.
   * @param script the script source code.
   * @param filename the name of the file containing the @a script.
   * @param lineno the line number of the @a script in the file.
   */
  virtual void Execute(const char *script,
                       const char *filename,
                       int lineno) = 0;

  /**
   * Compiles a script fragment in the context.
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
   * Sets the global object of the context.
   * @param global_object the global object of the context.
   * return @c true if succeeds.
   */
  virtual bool SetGlobalObject(ScriptableInterface *global_object) = 0;

  /**
   * Registers the constructor for a global class.
   * @param name name of the class.
   * @param constructor constructor of the class.
   * @return @c true if succeeded.
   */
  virtual bool RegisterClass(const char *name, Slot *constructor) = 0;

  /**
   * Locks an scriptable object to prevent the script engine from garbage
   * collecting the object. Object with @c ScriptableInterface::NATIVE_OWNED
   * or @c ScriptableInterface::NATIVE_PERMANENT ownership policies need NOT
   * call this because the script adapter should do this automatically.
   * The object must has already been attached into the script engine when
   * this method is called, otherwise this method does nothing. 
   */
  virtual void LockObject(ScriptableInterface *object) = 0;

  /**
   * Unlocks an scriptable object to allow the script engine to garbage
   * collect the object when possible.
   * The object must has already been attached into the script engine when
   * this method is called, otherwise this method does nothing. 
   */
  virtual void UnlockObject(ScriptableInterface *object) = 0;

};

} // namespace ggadget

#endif // GGADGET_SCRIPT_CONTEXT_INTERFACE_H__
