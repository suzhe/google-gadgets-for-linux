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

#ifndef GGADGET_ELEMENTS_INTERFACE_H__
#define GGADGET_ELEMENTS_INTERFACE_H__

#include "variant.h"

namespace ggadget {

class ElementInterface;

/**
 * ElementsInterface is used for storing and managing a set of objects which
 * implement the @c ElementInterface.
 */
class ElementsInterface {
 public:
  /** Get the children count. */
  virtual int GetCount() const = 0;

  /**
   * Returns the element identified by the name or index.
   * @param child the index or the name of the child.
   * @return the pointer to the specified element. If the parameter is provided
   *     as integer and is out of range, @c NULL is returned. If the parameter
   *     is provided as string and multiple elements are defined with the same
   *     name, returns the first one. Returns @c NULL if no elements match.
   */
  virtual ElementInterface *GetItem(Variant child) = 0;

  /**
   * Returns the element identified by the name or index.
   * @param child the index or the name of the child.
   * @return the pointer to the specified element. If the parameter is provided
   *     as integer and is out of range, @c NULL is returned. If the parameter
   *     is provided as string and multiple elements are defined with the same
   *     name, returns the first one. Returns @c NULL if no elements match.
   */
  virtual const ElementInterface *GetItem(Variant child) const = 0;
};

} // namespace ggadget

#endif // GGADGET_ELEMENTS_INTERFACE_H__
