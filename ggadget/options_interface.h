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

#ifndef GGADGET_OPTIONS_INTERFACE_H__
#define GGADGET_OPTIONS_INTERFACE_H__

#include "common.h"
#include "variant.h"

namespace ggadget {

class Connection;
template <typename R, typename P1> class Slot1;

class OptionsInterface {
 public:
  virtual ~OptionsInterface() { }

  /**
   * Connects a handler which will be called when any option changed.
   * The name of the changed option will be sent as the parameter.
   */
  virtual Connection *ConnectOnOptionChanged(
      Slot1<void, const char *> *handler) = 0;

  /**
   * @return the number of items in the options.
   */
  virtual size_t GetCount() = 0;

  /**
   * Adds an item to the options if it doesn't already exist.
   */
  virtual void Add(const char *name, const Variant &value) = 0;

  /**
   * @return @c true if the value of @a name has been set.
   */
  virtual bool Exists(const char *name) = 0;

  /**
   * @return the default value of the @a name.
   */
  virtual Variant GetDefaultValue(const char *name) = 0;

  /**
   * Sets the default value of the name.
   */
  virtual void PutDefaultValue(const char *name, const Variant &value) = 0;

  /**
   * @return the value associated with the name.
   */
  virtual Variant GetValue(const char *name) = 0;

  /**
   * Sets the value associated with the name, creating the entry if one
   * doesn't already exist for the name.
   */
  virtual void PutValue(const char *name, const Variant &value) = 0;

  /**
   * Removes the value from the options.
   */
  virtual void Remove(const char *name) = 0;

  /**
   * Removes all values from the options.
   */
  virtual void RemoveAll() = 0;
};

} // namespace ggadget

#endif  // GGADGET_OPTIONS_INTERFACE_H__
