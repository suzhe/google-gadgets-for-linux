// Copyright 2007 Google Inc. All Rights Reserved.
// Author: wangxianzhu@google.com (Xianzhu Wang)

#ifndef GGADGET_STATIC_SCRIPTABLE_H__
#define GGADGET_STATIC_SCRIPTABLE_H__

#include "scriptable_interface.h"
#include "slot.h"
#include "variant.h"

namespace ggadget {

class Signal;

/**
 * A @c ScriptableInterface implementation for objects whose definitions
 * of properties and methods don't change during their lifetime.
 */
class StaticScriptable : public ScriptableInterface {
 public:
  StaticScriptable();
  virtual ~StaticScriptable();

  /**
   * Register a scriptable property.
   * This @c StaticScriptable @a name owns the pointers of the
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
   * Register a scriptable method.
   * This @c StaticScriptable owns the pointer of @c slot.
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
   * @c StaticScriptable object are delegated to the prototype.
   * One prototype can be shared among multiple <code>StaticScriptable</code>s. 
   */
  void SetPrototype(ScriptableInterface *prototype);

  /** @see ScriptableInterface::Attach() */
  virtual void Attach() { ASSERT(false); }
  /** @see ScriptableInterface::Detach() */
  virtual void Detach() { ASSERT(false); }
  /** @see ScriptableInterface::IsInstanceof() */
  virtual bool IsInstanceOf(int class_id) const { ASSERT(false); return false; }
  /** @see ScriptableInterface::ConnectionToOnDeleteSignal() */
  virtual Connection *ConnectToOnDeleteSignal(Slot0<void> *slot);
  /** @see ScriptableInterface::GetPropertyInfoByName() */
  virtual bool GetPropertyInfoByName(const char *name,
                                     int *id, Variant *prototype,
                                     bool *is_method);
  /** @see ScriptableInterface::GetPropertyInfoById() */
  virtual bool GetPropertyInfoById(int id, Variant *prototype,
                                   bool *is_method);
  /** @see ScriptableInterface::GetProperty() */
  virtual Variant GetProperty(int id);
  /** @see ScriptableInterface::SetProperty() */
  virtual bool SetProperty(int id, Variant value);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(StaticScriptable);

  class Impl;
  Impl *impl_;
};

/**
 * A macro used to declare default ownership policy, that is, the native side
 * always has the ownership of the scriptable object.
 */
#define DEFAULT_OWNERSHIP_POLICY \
virtual void Attach() { }        \
virtual void Detach() { }

/**
 * A macro used in the declaration section of a @c ScriptableInterface
 * implementation to delegate most @c ScriptableInterface methods to another
 * object (normally @c StaticScriptable).
 * 
 * Full qualified type names are used in this macro to allow the user to use
 * other namespaces.
 */
#define DELEGATE_SCRIPTABLE_INTERFACE(delegate)                               \
virtual ggadget::Connection *ConnectToOnDeleteSignal(                         \
    ggadget::Slot0<void> *slot) {                                             \
  return (delegate).ConnectToOnDeleteSignal(slot);                            \
}                                                                             \
virtual bool GetPropertyInfoByName(const char *name, int *id,                 \
                                   ggadget::Variant *prototype,               \
                                   bool *is_method) {                         \
  return (delegate).GetPropertyInfoByName(name, id, prototype, is_method);    \
}                                                                             \
virtual bool GetPropertyInfoById(int id, ggadget::Variant *prototype,         \
                                 bool *is_method) {                           \
  return (delegate).GetPropertyInfoById(id, prototype, is_method);            \
}                                                                             \
virtual ggadget::Variant GetProperty(int id) {                                \
  return (delegate).GetProperty(id);                                          \
}                                                                             \
virtual bool SetProperty(int id, ggadget::Variant value) {                    \
  return (delegate).SetProperty(id, value);                                   \
}

/**
 * A macro used in the declaration section of a @c ScriptableInterface
 * implementation to delegate all @c StaticScriptable @c RegisterXXXX methods
 * to a @c StaticScriptable object.
 * 
 * Full qualified type names are used in this macro to allow the user to use
 * other namespaces.
 */
#define DELEGATE_SCRIPTABLE_REGISTER(delegate)                                \
void RegisterProperty(const char *name,                                       \
                      ggadget::Slot *getter, ggadget::Slot *setter) {         \
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
void RegisterMethod(const char *name, ggadget::Slot *slot) {                  \
  (delegate).RegisterMethod(name, slot);                                      \
}                                                                             \
void RegisterSignal(const char *name, ggadget::Signal *signal) {              \
  (delegate).RegisterSignal(name, signal);                                    \
}                                                                             \
void RegisterConstants(int c, const char *const n[],                          \
                       const ggadget::Variant v[]) {                          \
  (delegate).RegisterConstants(c, n, v);                                      \
}                                                                             \
template <typename T>                                                         \
void RegisterConstant(const char *name, T value) {                            \
  (delegate).RegisterConstant(name, value);                                   \
}                                                                             \
void SetPrototype(ggadget::ScriptableInterface *prototype) {                  \
  (delegate).SetPrototype(prototype);                                         \
}

} // namespace ggadget

#endif // GGADGET_STATIC_SCRIPTABLE_H__
