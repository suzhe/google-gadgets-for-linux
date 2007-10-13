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

#ifndef GGADGET_SCRIPTABLE_HELPER_H__
#define GGADGET_SCRIPTABLE_HELPER_H__

#include "slot.h"
#include "variant.h"

namespace ggadget {

class Connection;
class Signal;

/**
 * A @c ScriptableInterface implementation helper.
 */
class ScriptableHelper {
 public:
  ScriptableHelper();
  virtual ~ScriptableHelper();

  /**
   * Register a scriptable property.
   * This @c ScriptableHelper @a name owns the pointers of the
   * @a getter and the @a setter.
   * @param name property name.  It must point to static allocated memory.
   * @param getter the getter slot of the property.
   * @param setter the setter slot of the property.
   */
  void RegisterProperty(const char *name, Slot *getter, Slot *setter);

  /**
   * Register a simple scriptable property that maps to a variable.
   * @param name property name.  It must point to static allocated memory.
   * @param valuep point to a value.
   */
  template <typename T>
  void RegisterSimpleProperty(const char *name, T *valuep) {
    RegisterProperty(name,
                     NewSimpleGetterSlot<T>(valuep),
                     NewSimpleSetterSlot<T>(valuep));
  }

  /**
   * Register a simple readonly scriptable property that maps to a variable.
   * @param name property name.  It must point to static allocated memory.
   * @param valuep point to a value.
   */
  template <typename T>
  void RegisterReadonlySimpleProperty(const char *name, const T *valuep) {
    RegisterProperty(name, NewSimpleGetterSlot<T>(valuep), NULL);
  }

  /**
   * Register a scriptable property having enumerated values that should
   * mapped to strings.
   * @param name property name.  It must point to static allocated memory.
   * @param getter a getter slot returning an enum value.
   * @param setter a setter slot accepting an enum value. @c NULL if the
   *     property is readonly.
   * @param names a table containing string values of every enum values.
   * @param count number of entries in the @a names table.
   */
  void RegisterStringEnumProperty(const char *name,
                                  Slot *getter, Slot *setter,
                                  const char **names, int count);

  /**
   * Register a scriptable method.
   * This @c ScriptableHelper owns the pointer of @c slot.
   * @param name method name.  It must point to static allocated memory.
   * @param slot the method slot.
   */
  void RegisterMethod(const char *name, Slot *slot);

  /**
   * Register a @c Signal that can connect to various @c Slot callbacks.
   * After this call, a same named property will be automatically registered
   * that can be used to get/set the @c Slot callback.
   * @param name the name of the @a signal.  It must point to static
   *     allocated memory.
   * @param signal the @c Signal to be registered.
   */
  void RegisterSignal(const char *name, Signal *signal);

  /**
   * Register a set of constants.
   * @param count number of constants to register.
   * @param names array of names of the constants.  The pointers must point to
   *     static allocated memory.
   * @param values array of constant values.  If it is @c NULL, the values
   *     will be automatically assigned from @c 0 to count-1, which is useful
   *     to define enum values.
   */
  void RegisterConstants(int count,
                         const char *const names[],
                         const Variant values[]);

  /**
   * Register a constant.
   * @param name the constant name.
   * @param value the constant value.
   */
  template <typename T>
  void RegisterConstant(const char *name, T value) {
    Variant variant(value);
    RegisterConstants(1, &name, &variant);
  }

  /**
   * Set a prototype object which defines common properties (including
   * methods and signals).
   * Any operations to properties not registered in current
   * @c ScriptableHelper object are delegated to the prototype.
   * One prototype can be shared among multiple <code>ScriptableHelper</code>s. 
   */
  void SetPrototype(ScriptableInterface *prototype);

  /**
   * Set the array handler which will handle array accesses.
   * @param getter handles 'get' accesses.  It accepts an int parameter as the
   *     array index and return the result of any type that can be contained
   *     in a @c Variant.  It should return a @c Variant of type
   *     @c Variant::TYPE_VOID if it doesn't support the property.
   * @param setter handles 'set' accesses.  It accepts an int parameter as the
   *     array index and a value.  If it returns a @c bool value, @c true on
   *     success.
   */
  void SetArrayHandler(Slot *getter, Slot *setter);

  /**
   * Set the dynamic property handler which will handle property accesses not
   * registered statically.
   *
   * @param getter handles 'get' accesses.  It accepts a property name
   *     parameter (<code>const char *</code>) and return the result of any
   *     type that can be contained in a @c Variant.
   * @param setter handles 'set' accesses.  It accepts a property name
   *     parameter (<code>const char *</code>) and a value. If it returns a
   *     @c bool value, @c true on success.
   */
  void SetDynamicPropertyHandler(Slot *getter, Slot *setter);

  /** @see ScriptableInterface::Attach() */
  void Attach() { ASSERT(false); }
  /** @see ScriptableInterface::Detach() */
  void Detach() { ASSERT(false); }
  /** @see ScriptableInterface::ConnectionToOnDeleteSignal() */
  Connection *ConnectToOnDeleteSignal(Slot0<void> *slot);
  /** @see ScriptableInterface::GetPropertyInfoByName() */
  bool GetPropertyInfoByName(const char *name,
                             int *id, Variant *prototype,
                             bool *is_method);
  /** @see ScriptableInterface::GetPropertyInfoById() */
  bool GetPropertyInfoById(int id, Variant *prototype,
                                   bool *is_method, const char **name);
  /** @see ScriptableInterface::GetProperty() */
  Variant GetProperty(int id);
  /** @see ScriptableInterface::SetProperty() */
  bool SetProperty(int id, Variant value);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableHelper);

  class Impl;
  Impl *impl_;
};

/**
 * A macro used in the declaration section of a @c ScriptableInterface
 * implementation to delegate all @c ScriptableHelper @c RegisterXXXX methods
 * to a @c ScriptableHelper object.
 * 
 * Full qualified type names are used in this macro to allow the user to use
 * other namespaces.
 */
#define DELEGATE_SCRIPTABLE_REGISTER(delegate)                                \
void RegisterProperty(const char *name,                                       \
                      ::ggadget::Slot *getter, ::ggadget::Slot *setter) {     \
  (delegate).RegisterProperty(name, getter, setter);                          \
}                                                                             \
template <typename T>                                                         \
void RegisterSimpleProperty(const char *name, T *valuep) {                    \
  (delegate).RegisterSimpleProperty<T>(name, valuep);                         \
}                                                                             \
template <typename T>                                                         \
void RegisterReadonlySimpleProperty(const char *name, const T *valuep) {      \
  (delegate).RegisterReadonlySimpleProperty<T>(name, valuep);                 \
}                                                                             \
void RegisterStringEnumProperty(const char *name,                             \
                                Slot *getter, Slot *setter,                   \
                                const char **names, int count) {              \
  (delegate).RegisterStringEnumProperty(name, getter, setter, names, count);  \
}                                                                             \
void RegisterMethod(const char *name, ::ggadget::Slot *slot) {                \
  (delegate).RegisterMethod(name, slot);                                      \
}                                                                             \
void RegisterSignal(const char *name, ::ggadget::Signal *signal) {            \
  (delegate).RegisterSignal(name, signal);                                    \
}                                                                             \
void RegisterConstants(int c, const char *const n[],                          \
                       const ::ggadget::Variant v[]) {                        \
  (delegate).RegisterConstants(c, n, v);                                      \
}                                                                             \
template <typename T>                                                         \
void RegisterConstant(const char *name, T value) {                            \
  (delegate).RegisterConstant(name, value);                                   \
}                                                                             \
void SetPrototype(::ggadget::ScriptableInterface *prototype) {                \
  (delegate).SetPrototype(prototype);                                         \
}                                                                             \
void SetArrayHandler(::ggadget::Slot *getter, ::ggadget::Slot *setter) {      \
  (delegate).SetArrayHandler(getter, setter);                                 \
}                                                                             \
void SetDynamicPropertyHandler(                                               \
    ::ggadget::Slot *getter, ::ggadget::Slot *setter) {                       \
  (delegate).SetDynamicPropertyHandler(getter, setter);                       \
}

} // namespace ggadget

#endif // GGADGET_SCRIPTABLE_HELPER_H__
