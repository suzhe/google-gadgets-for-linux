// Copyright 2007 Google Inc. All Rights Reserved.
// Author: wangxianzhu@google.com (Xianzhu Wang)

#ifndef GGADGET_STATIC_SCRIPTABLE_H__
#define GGADGET_STATIC_SCRIPTABLE_H__

#include "scriptable_interface.h"
#include "variant.h"

namespace ggadget {

class Slot;
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
   * @param name property name.
   * @param getter the getter slot of the property.
   * @param setter the setter slot of the property.
   */
  void RegisterProperty(const char *name, Slot *getter, Slot *setter);

  /**
   * Register a scriptable method.
   * This @c StaticScriptable owns the pointer of @c slot.
   * @param name method name.
   * @param slot the method slot.
   */
  void RegisterMethod(const char *name, Slot *slot);

  /**
   * Register a @c Signal that can connect to various @c Slot callbacks.
   * After this call, a same named property will be automatically registered
   * that can be used to get/set the @c Slot callback.
   * @param name the name of the @a signal.
   * @param signal the @c Signal to be registered.
   */
  void RegisterSignal(const char *name, Signal *signal);

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
  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_STATIC_SCRIPTABLE_H__
