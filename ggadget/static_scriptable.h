// Copyright 2007 Google Inc. All Rights Reserved.
// Author: wangxianzhu@google.com (Xianzhu Wang)

#ifndef GGADGET_STATIC_SCRIPTABLE_H__
#define GGADGET_STATIC_SCRIPTABLE_H__

#include "scriptable_interface.h"
#include "variant.h"

class Slot;

namespace ggadget {

/**
 * A @c ScriptableInterface implementation for objects whose definitions
 * of properties and methods don't change during their lifetime.
 */
class StaticScriptable : public ScriptableInterface {
 public:
  StaticScriptable();

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

  /** @see ScriptableInterface::AddRef() */
  virtual int AddRef();
  /** @see ScriptableInterface::Release() */
  virtual int Release();
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
  // Only allow to delete by itself.
  virtual ~StaticScriptable();

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_STATIC_SCRIPTABLE_H__
