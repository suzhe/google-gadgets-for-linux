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
#include "elements_interface.h"

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
class Elements : public ElementsInterface {
 public:
  /** Create an Elements object and assign the given factory to it. */
  Elements(ElementFactoryInterface *factory, ElementInterface *owner);

  /** Not virtual because no inheritation to this class is allowed. */
  ~Elements();

 public:
  /** @see ElementsInterface::GetCount */
  virtual int GetCount() const;

  /** @see ElementsInterface::GetItemByIndex */
  virtual ElementInterface *GetItemByIndex(int child);

  /** @see ElementsInterface::GetItemByIndex */
  virtual ElementInterface *GetItemByName(const char *child);

  /** @see ElementsInterface::GetItemByIndex */
  virtual const ElementInterface *GetItemByIndex(int child) const;

  /** @see ElementsInterface::GetItemByIndex */
  virtual const ElementInterface *GetItemByName(const char *child) const;

  /**
   * Create a new Element and add it to the end of the children list.
   * @param tag_name a string specified the element tag name.
   * @param name the name of the newly created element.
   * @return the pointer to the newly created element, or @c NULL when error
   *     occured.
   */
  ElementInterface *AppendElement(const char *tag_name, const char *name);

  /**
   * Create a new Element before the specified element.
   * @param tag_name a string specified the element tag name.
   * @param before the newly created element will be inserted before the given
   *     element. If the specified element is not the direct child of the
   *     container or this parameter is @c NULL, this method will insert the
   *     newly created element at the end of the children list.
   * @param name the name of the newly created element.
   * @return the pointer to the newly created element, or @c NULL when error
   *     occured.
   */
  ElementInterface *InsertElement(const char *tag_name,
                                  const ElementInterface *before,
                                  const char *name);

  /**
   * Remove the specified element from the container.
   * @param element the element to remove.
   * @return @c true if removed successfully, or @c false if the specified
   *     element doesn't exists or not the direct child of the container.
   */
  bool RemoveElement(ElementInterface *element);

  /**
   * Remove all elements from the container.
   */
  void RemoveAllElements();

 private:
  internal::ElementsImpl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(Elements);
};

} // namespace ggadget

#endif // GGADGET_ELEMENTS_H__
