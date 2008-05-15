/*
  Copyright 2008 Google Inc.

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
#include <ggadget/variant.h>
#include <ggadget/slot.h>

namespace ggadget {

class Connection;
class RegisterableInterface;

/**
 * Object interface that can be called from script languages.
 * Normally an object need not to implement this interface directly, but
 * inherits from @c ScriptableHelper.
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
  static const int kDynamicPropertyId = INT_MIN;

  /**
   * The pseudo id for constant properties.
   * @see GetPropertyInfoByName()
   */
  static const int kConstantPropertyId = INT_MIN + 1;

  /**
   * Gets the class id of this object. For debugging purpose only.
   */
  virtual uint64_t GetClassId() const = 0;

  /**
   * Adds a reference to this object.
   */
  virtual void Ref() = 0;

  /**
   * Removes a reference from this object.
   * @param transient if @c true, the reference will be removed transiently,
   *     that is, the object will not be deleted even if reference count
   *     reaches zero (i.e. the object is floating). This is useful before
   *     returning an object from a function.
   */
  virtual void Unref(bool transient = false) = 0;

  /**
   * Gets the current reference count.
   */
  virtual int GetRefCount() const = 0;

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
   * Connect a callback which will be called when @c Ref() or @c Unref() is
   * called.
   * @param slot the callback. The parameters of the slot are:
   *     - the reference count before change.
   *     - 1 or -1 indicating whether the reference count is about to be
   *       increased or decreased; or 0 if the object is about to be deleted.
   * @return the connected @c Connection.
   */
  virtual Connection *ConnectOnReferenceChange(Slot2<void, int, int> *slot) = 0;

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
   *     If the returned id is @c kConstantPropertyId, the script engine
   *     will treat the property as a constant and the value is returned
   *     in @a prototype.
   *     If the returned id is @c kDynamicPropertyId, the script engine
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
   *             which the script engine can get detailed information.
   * @param[out] is_method true if this property corresponds a method.
   * @param[out] name the name of the property. The returned value is a
   *             constant string, which shall not be freed or modified.
   * @return @c true if the property is supported and succeeds.
   */
  virtual bool GetPropertyInfoById(int id, Variant *prototype,
                                   bool *is_method,
                                   const char **name) = 0;

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
  virtual bool SetProperty(int id, const Variant &value) = 0;

  /**
   * Gets and clears the current pending exception.
   * The script adapter will call this method after each call of
   * @c GetPropertyInfoById(), @c GetPropertyInfoByName(), @c GetProperty()
   * and @c SetProoperty().
   * @param clear if @c true, the pending exception will be cleared.
   * @return the pending exception.
   */
  virtual ScriptableInterface *GetPendingException(bool clear) = 0;

  /**
   * The first param is the return value of this call back. Return false if the
   * call back do not want to enumerate any more.
   * The second param is the id of the property in the ScriptableInterface
   * object.
   * The third and fourth param is the name and the value of the property.
   * And the last param indicate that if the property is a method.
   */
  typedef Slot4<bool, int, const char *, const Variant &, bool>
      EnumeratePropertiesCallback;
  /**
   * Enumerate all known properties.
   * @param callback it will be called for each property. The parameters are
   *     id, name, current value and a bool indicating if the property is a
   *     method. The callback should return @c false if it doesn't want to
   *     continue. The callback will be automatically deleted before this
   *     method returns, so the caller can use @c NewSlot for the parameter.
   * @return @c false if the callback returns @c false.
   */
  virtual bool EnumerateProperties(EnumeratePropertiesCallback *callback) = 0;

  /**
   * The first param is the return value of this call back. Return false if the
   * call back do not want to enumerate any more.
   * The second param is the index of the element in the ScriptableInterface
   * object.
   * The third param is the value of the element.
   */
  typedef Slot2<bool, int, const Variant &> EnumerateElementsCallback;
  /**
   * Enumerate all known elements (i.e. properties that can be accessed by
   * non-negative array indexes).
   * @param callback it will be called for each property. The parameters are
   *     id, and current value. The callback should return @c false if it
   *     doesn't want to continue. The callback will be automatically deleted
   *     before this method returns, so the caller can use @c NewSlot for
   *     the parameter.
   * @return @c false if the callback returns @c false.
   */
  virtual bool EnumerateElements(EnumerateElementsCallback *callback) = 0;

  /**
   * Returns the @c RegisterableInterface pointer if this object supports it,
   * otherwise returns @c NULL.
   */
  virtual RegisterableInterface *GetRegisterable() = 0;

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
  }                                                                          \
  virtual uint64_t GetClassId() const { return UINT64_C(cls_id); }

inline bool ScriptableInterface::IsInstanceOf(uint64_t class_id) const {
  return class_id == CLASS_ID;
}

} // namespace ggadget

#endif // GGADGET_SCRIPTABLE_INTERFACE_H__
