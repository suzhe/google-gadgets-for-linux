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

#include <ggadget/scriptable_interface.h>

namespace ggadget {

class CanvasInterface;
class ElementInterface;
class ElementFactoryInterface;
class ViewInterface;
class MouseEvent;
class DragEvent;

/**
 * Elements is used for storing and managing a set of objects which
 * implement the @c ElementInterface.
 */
class ElementsInterface : public ScriptableInterface {
 public:
  CLASS_ID_DECL(0x09bf1e61ac344e8a);

 protected:
  virtual ~ElementsInterface() { }

 public:
  /**
   * @return number of children.
   */
  virtual int GetCount() const = 0;

  /**
   * Returns the element identified by the index.
   * @param child the index of the child.
   * @return the pointer to the specified element. If the parameter is out of
   *     range, @c NULL is returned.
   */
  virtual ElementInterface *GetItemByIndex(int child) = 0;
  virtual const ElementInterface *GetItemByIndex(int child) const = 0;

  /**
   * Returns the element identified by the name.
   * @param child the name of the child.
   * @return the pointer to the specified element. If multiple elements are
   *     defined with the same name, returns the first one. Returns @c NULL if
   *     no elements match.
   */
  virtual ElementInterface *GetItemByName(const char *child) = 0;
  virtual const ElementInterface *GetItemByName(const char *child) const = 0;

  /**
   * Create a new element and add it to the end of the children list.
   * @param tag_name a string specified the element tag name.
   * @param name the name of the newly created element.
   * @return the pointer to the newly created element, or @c NULL when error
   *     occured.
   */
  virtual ElementInterface *AppendElement(const char *tag_name,
                                          const char *name) = 0;

  /**
   * Create a new element before the specified element.
   * @param tag_name a string specified the element tag name.
   * @param before the newly created element will be inserted before the given
   *     element. If the specified element is not the direct child of the
   *     container or this parameter is @c NULL, this method will insert the
   *     newly created element at the end of the children list.
   * @param name the name of the newly created element.
   * @return the pointer to the newly created element, or @c NULL when error
   *     occured.
   */
  virtual ElementInterface *InsertElement(const char *tag_name,
                                          const ElementInterface *before,
                                          const char *name) = 0;

  /**
   * Create a new element from XML definition and add it to the end of the
   * children list.
   * @param xml the XML definition of the element.
   * @return the pointer to the newly created element, or @c NULL when error
   *     occured.
   */
  virtual ElementInterface *AppendElementFromXML(const char *xml) = 0;

  /**
   * Create a new element from XML definition and insert it before the
   * specified element.
   * @param xml the XML definition of the element.
   * @param before the newly created element will be inserted before the given
   *     element. If the specified element is not the direct child of the
   *     container or this parameter is @c NULL, this method will insert the
   *     newly created element at the end of the children list.
   * @return the pointer to the newly created element, or @c NULL when error
   *     occured.
   */
  virtual ElementInterface *InsertElementFromXML(
      const char *xml, const ElementInterface *before) = 0;

  /**
   * Remove the specified element from the container.
   * @param element the element to remove.
   * @return @c true if removed successfully, or @c false if the specified
   *     element doesn't exists or not the direct child of the container.
   */
  virtual bool RemoveElement(ElementInterface *element) = 0;

  /**
   * Remove all elements from the container.
   */
  virtual void RemoveAllElements() = 0;
};

CLASS_ID_IMPL(ElementsInterface, ScriptableInterface)

} // namespace ggadget

#endif // GGADGET_ELEMENTS_INTERFACE_H__
