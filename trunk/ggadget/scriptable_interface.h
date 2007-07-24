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

#include "variant.h"

namespace ggadget {

class Connection;
template <typename R> class Slot0;

/**
 * Object interface that can be called from script languages.
 * Only objects with dynamic properties or methods need to directly
 * implement this interface.  Other objects should use @c StaticScriptable.
 */
class ScriptableInterface {
 public:
  /**
   * Connect a callback @c Slot to the "ondelete" signal.
   * @param slot the callback @c Slot to be called when the @c Scriptable
   *     object is to be deleted.
   * @return the connected @c Connection or @c NULL if fails.
   */
  virtual Connection *ConnectToOnDeleteSignal(Slot0<void> *slot) = 0;

  /**
   * Get the info of a property by its name.
   * Because methods are special properties, if @c name corresponds a method,
   * a prototype of type @c Variant::TYPE_SLOT will be returned, then the
   * caller can get the function details from the @c Slot value this prototype.
   * A property expecting a script function also has a prototype of type
   * @c Variant::TYPE_SLOT.
   *
   * @param name the name of the property.
   * @param[out] id return the property's id which can be used in later
   *     @c GetProperty(), @c SetProperty() and @c InvokeMethod() calls.
   *     The value must be a @b negative number.
   * @param[out] prototype return a prototype of the property value, from
   *     which the script engine can get detailed information.
   * @param[out] is_method @c true if this property corresponds a method.
   *     It's useful to distinguish between methods and call back properties.
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

} // namespace ggadget

#endif // GGADGET_SCRIPTABLE_INTERFACE_H__
