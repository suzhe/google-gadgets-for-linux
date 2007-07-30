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

#ifndef GGADGET_ELEMENTS_H__
#define GGADGET_ELEMENTS_H__

#include "common.h"

namespace ggadget {

namespace internal {

class ElementsImpl;

} // namespace internal

class ElementInterface;
class ElementFactoryInterface;

/**
 * Elements is used for storing and managing a set of objects which
 * implement the @c ElementInterface.
 */
class Elements {
 public:
  /** Create an Elements object and assign the given factory to it. */
  Elements(ElementFactoryInterface *factory, ElementInterface *owner);

  /** Not virtual because no inheritation to this class is allowed. */
  ~Elements();

 public:
  /** Get the children count. */
  int GetCount() const;

  /**
   * Get children by index.
   * @param index the position of the element.
   * @return the pointer to the specified element, or @c NULL if the index is
   *     out of range.
   */
  ElementInterface *GetItem(int index);

  /**
   * Get children by index.
   * @param index the position of the element.
   * @return the pointer to the specified element, or @c NULL if the index is
   *     out of range.
   */
  const ElementInterface *GetItem(int index) const;

  /**
   * Create a new Element and add it to the end of the children list.
   * @param type a string specified the element type.
   * @return the pointer to the newly created element, or @c NULL when error
   *     occured.
   */
  ElementInterface *AppendElement(const char *type);

  /**
   * Create a new Element before the specified element.
   * @param type a string specified the element type.
   * @param before the newly created element will be inserted before the given
   *     element. If the specified element is not the direct child of the
   *     container or this parameter is @c NULL, this method will insert the
   *     newly created element at the end of the children list.
   * @return the pointer to the newly created element, or @c NULL when error
   *     occured.
   */
  ElementInterface *InsertElement(const char *type,
                                  const ElementInterface *before);

  /**
   * Remove the specified element from the container.
   * @param element the element to remove.
   * @return @c true if removed successfully, or @c false if the specified
   *     element doesn't exists or not the direct child of the container.
   */
  bool RemoveElement(const ElementInterface *element);

 private:
  internal::ElementsImpl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(Elements);
};

} // namespace ggadget

#endif // GGADGET_ELEMENTS_H__
