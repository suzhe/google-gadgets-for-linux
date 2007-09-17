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

#ifndef GGADGET_SCRIPTABLE_INTERFACE_H__
#define GGADGET_SCRIPTABLE_INTERFACE_H__

#include <limits.h>
#include <stdint.h>
#include "variant.h"

namespace ggadget {

class Connection;
template <typename R> class Slot0;

/**
 * Object interface that can be called from script languages.
 * Only objects with dynamic properties or methods need to directly
 * implement this interface.  Other objects should use @c ScriptableHelper.
 *
 * Any interface or abstract class inheriting @c ScriptableInterface should
 * include @c CLASS_ID_DECL and @c CLASS_ID_IMPL to define its @c CLASS_ID and
 * @c IsInstanceOf() method.
 *
 * Any concrete implementation class should include @c DEFINE_CLASS_ID to
 * define its @c CLASS_ID and @c IsInstanceOf() method.
 */
class ScriptableInterface {
 protected:
  /**
   * Disallow direct deletion.
   */
  virtual ~ScriptableInterface() { }

 public:

  /**
   * This ID uniquely identifies the class.  Each implementation should define
   * this field as a unique number.  You can simply use the first 3 parts of
   * the result of uuidgen.
   */
  static const uint64_t CLASS_ID = 0;

  /**
   * The pseudo id for dynamic properties.
   * @see GetPropertyInfoByName()
   */
  static const int ID_DYNAMIC_PROPERTY = INT_MIN;

  /**
   * The pseudo id for constant properties.
   * @see GetPropertyInfoByName()
   */
  static const int ID_CONSTANT_PROPERTY = 0;

  /**
   * Attach this object to the script engine.
   * Normally if the object is always owned by the native side, the
   * implementation should do nothing in this method.
   *
   * If the ownership can be transfered or shared between the native side
   * and the script side, the implementation should do appropriate things,
   * such as reference counting, etc. to manage the ownership.
   */
  virtual void Attach() = 0;

  /**
   * Detach this object from the script engine.
   * @see Attach()
   */
  virtual void Detach() = 0;

  /**
   * Judge if this instance is of a given class.
   */
  virtual bool IsInstanceOf(uint64_t class_id) const = 0;

  /**
   * Test if this object is 'strict', that is, not allowing the script to
   * assign to a previously undefined property.
   */
  virtual bool IsStrict() const = 0;

  /**
   * Connect a callback @c Slot to the "ondelete" signal.
   * @param slot the callback @c Slot to be called when the @c Scriptable
   *     object is to be deleted.
   * @return the connected @c Connection or @c NULL if fails.
   */
  virtual Connection *ConnectToOnDeleteSignal(Slot0<void> *slot) = 0;

  /**
   * Get the info of a property by its name.
   *
   * Because methods are special properties, if @c name corresponds a method,
   * a prototype of type @c Variant::TYPE_SLOT will be returned, then the
   * caller can get the function details from @c slot_value this prototype.
   *
   * A signal property also expects a script function as the value, and thus
   * also has a prototype of type @c Variant::TYPE_SLOT.
   *
   * @param name the name of the property.
   * @param[out] id return the property's id which can be used in later
   *     @c GetProperty(), @c SetProperty() and @c InvokeMethod() calls.
   *     If the returned id is @c ID_CONSTANT_PROPERTY, the script engine
   *     will treat the property as a constant and the value is returned 
   *     in @a prototype.
   *     If the returned id is @c ID_DYNAMIC_PROPERTY, the script engine
   *     should not register the property any way, and should call
   *     @c GetProperty or @c SetProperty immediately.
   *     Otherwise, the value must be a @b negative number.
   * @param[out] prototype return a prototype of the property value, from
   *     which the script engine can get detailed information.
   * @param[out] is_method @c true if this property corresponds a method.
   *     It's useful to distinguish between methods and signal properties.
   * @return @c true if the property is supported and succeeds.
   */
  virtual bool GetPropertyInfoByName(const char *name,
                                     int *id, Variant *prototype,
                                     bool *is_method) = 0;

  /**
   * Get the info of a property by its id.
   * @param id if negative, it is a property id previously returned from
   *     @c GetPropertyInfoByName(); otherwise it is the array index of a
   *     property.
   * @param[out] prototype return a prototype of the property value, from
   *     which the script engine can get detailed information.
   * @param[out] is_method true if this property corresponds a method.
   * @return @c true if the property is supported and succeeds.
   */
  virtual bool GetPropertyInfoById(int id, Variant *prototype,
                                   bool *is_method) = 0;

  /**
   * Get the value of a property by its id.
   * @param id if negative, it is a property id previously returned from
   *     @c GetPropertyInfoByName(); otherwise it is the array index of a
   *     property.
   * @return the property value, or a @c Variant of type @c Variant::TYPE_VOID
   *     if this property is not supported,
   */
  virtual Variant GetProperty(int id) = 0;

  /**
   * Set the value of a property by its id.
   * @param id if negative, it is a property id previously returned from
   *     @c GetPropertyInfoByName(); otherwise it is the array index of a
   *     property.
   * @param value the property value. The type must be compatible with the
   *     prototype returned from @c GetPropertyInfoByName().
   * @return @c true if the property is supported and succeeds.
   */
  virtual bool SetProperty(int id, Variant value) = 0;
};

/**
 * Used in the declaration section of an interface which inherits
 * @c ScriptableInterface to declare a class id.
 */
#define CLASS_ID_DECL(cls_id)                                                \
  static const uint64_t CLASS_ID = UINT64_C(cls_id);                         \
  virtual bool IsInstanceOf(uint64_t class_id) const = 0;

/**
 * Used after the declaration section of an interface which inherits
 * @c ScriptableInterface to define @c IsInstanceOf() method.
 */
#define CLASS_ID_IMPL(cls, super)                                            \
  inline bool cls::IsInstanceOf(uint64_t class_id) const {                   \
    return class_id == CLASS_ID || super::IsInstanceOf(class_id);            \
  }

/**
 * Used in the declaration section of a class which implements
 * @c ScriptableInterface or an interface inheriting @c ScriptableInterface.
 */
#define DEFINE_CLASS_ID(cls_id, super)                                       \
  static const uint64_t CLASS_ID = UINT64_C(cls_id);                         \
  virtual bool IsInstanceOf(uint64_t class_id) const {                       \
    return class_id == CLASS_ID || super::IsInstanceOf(class_id);            \
  }

inline bool ScriptableInterface::IsInstanceOf(uint64_t class_id) const {
  return class_id == CLASS_ID;
}


/**
 * A macro used to declare default ownership policy, that is, the native side
 * always has the ownership of the scriptable object.
 */
#define DEFAULT_OWNERSHIP_POLICY \
virtual void Attach() { }        \
virtual void Detach() { }

/**
 * A macro used in the declaration section of a @c ScriptableInterface
 * implementation to inline delegate most @c ScriptableInterface methods to
 * another object (normally @c ScriptableHelper).
 * 
 * Full qualified type names are used in this macro to allow the user to use
 * other namespaces.
 */
#define DELEGATE_SCRIPTABLE_INTERFACE(delegate)                               \
virtual ::ggadget::Connection *ConnectToOnDeleteSignal(                       \
    ::ggadget::Slot0<void> *slot) {                                           \
  return (delegate).ConnectToOnDeleteSignal(slot);                            \
}                                                                             \
virtual bool GetPropertyInfoByName(const char *name, int *id,                 \
                                   ::ggadget::Variant *prototype,             \
                                   bool *is_method) {                         \
  return (delegate).GetPropertyInfoByName(name, id, prototype, is_method);    \
}                                                                             \
virtual bool GetPropertyInfoById(int id, ::ggadget::Variant *prototype,       \
                                 bool *is_method) {                           \
  return (delegate).GetPropertyInfoById(id, prototype, is_method);            \
}                                                                             \
virtual ::ggadget::Variant GetProperty(int id) {                              \
  return (delegate).GetProperty(id);                                          \
}                                                                             \
virtual bool SetProperty(int id, ::ggadget::Variant value) {                  \
  return (delegate).SetProperty(id, value);                                   \
}

/**
 * A macro used in the declaration section of a @c ScriptableInterface
 * implementation to declare @c ScriptableInterface methods.
 * 
 * Full qualified type names are used in this macro to allow the user to use
 * other namespaces.
 */
#define SCRIPTABLE_INTERFACE_DECL                                             \
virtual ::ggadget::Connection *ConnectToOnDeleteSignal(                       \
    ::ggadget::Slot0<void> *slot);                                            \
virtual bool GetPropertyInfoByName(const char *name, int *id,                 \
                                   ::ggadget::Variant *prototype,             \
                                   bool *is_method);                          \
virtual bool GetPropertyInfoById(int id, ::ggadget::Variant *prototype,       \
                                 bool *is_method);                            \
virtual ::ggadget::Variant GetProperty(int id);                               \
virtual bool SetProperty(int id, ::ggadget::Variant value);

/**
 * A macro used in the .cc definition of a @c ScriptableInterface
 * implementation to delegate most @c ScriptableInterface methods to
 * another object (normally @c ScriptableHelper)
 * 
 * Full qualified type names are used in this macro to allow the user to use
 * other namespaces.
 */
#define DELEGATE_SCRIPTABLE_INTERFACE_IMPL(class_name, delegate)              \
::ggadget::Connection *class_name::ConnectToOnDeleteSignal(                   \
    ::ggadget::Slot0<void> *slot) {                                           \
  return (delegate).ConnectToOnDeleteSignal(slot);                            \
}                                                                             \
bool class_name::GetPropertyInfoByName(const char *name, int *id,             \
                                       ::ggadget::Variant *prototype,         \
                                       bool *is_method) {                     \
  return (delegate).GetPropertyInfoByName(name, id, prototype, is_method);    \
}                                                                             \
bool class_name::GetPropertyInfoById(int id, ::ggadget::Variant *prototype,   \
                                     bool *is_method) {                       \
  return (delegate).GetPropertyInfoById(id, prototype, is_method);            \
}                                                                             \
::ggadget::Variant class_name::GetProperty(int id) {                          \
  return (delegate).GetProperty(id);                                          \
}                                                                             \
bool class_name::SetProperty(int id, ::ggadget::Variant value) {              \
  return (delegate).SetProperty(id, value);                                   \
}

} // namespace ggadget

#endif // GGADGET_SCRIPTABLE_INTERFACE_H__
