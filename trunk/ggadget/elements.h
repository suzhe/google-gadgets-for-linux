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
#include "scriptable_interface.h"

namespace ggadget {

namespace internal {

class ElementsImpl;

} // namespace internal

class CanvasInterface;
class ElementInterface;
class ElementFactoryInterface;
class ViewInterface;

/**
 * Elements is used for storing and managing a set of objects which
 * implement the @c ElementInterface.
 */
class Elements : public ScriptableInterface {
 public:
  DEFINE_CLASS_ID(0xe3bdb064cb794282, ScriptableInterface)

  /**
   * Create an Elements object and assign the given factory to it.
   * @param factory the factory used to create the child elements.
   * @param owner the parent element. Can be @c null for top level elements
   *     owned directly by a view.
   * @param view the containing view. 
   */
  Elements(ElementFactoryInterface *factory,
           ElementInterface *owner,
           ViewInterface *view);

  /** Not virtual because no inheritation to this class is allowed. */
  ~Elements();

 public:  
  /** @see ElementsInterface::GetCount */
  int GetCount() const;

  /** @see ElementsInterface::GetItemByIndex */
  ElementInterface *GetItemByIndex(int child);

  /** @see ElementsInterface::GetItemByIndex */
  ElementInterface *GetItemByName(const char *child);

  /** @see ElementsInterface::GetItemByIndex */
  const ElementInterface *GetItemByIndex(int child) const;

  /** @see ElementsInterface::GetItemByIndex */
  const ElementInterface *GetItemByName(const char *child) const;

  /**
   * Create a new element and add it to the end of the children list.
   * @param tag_name a string specified the element tag name.
   * @param name the name of the newly created element.
   * @return the pointer to the newly created element, or @c NULL when error
   *     occured.
   */
  ElementInterface *AppendElement(const char *tag_name, const char *name);

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
  ElementInterface *InsertElement(const char *tag_name,
                                  const ElementInterface *before,
                                  const char *name);

  /**
   * Create a new element from XML definition and add it to the end of the
   * children list.
   * @param xml the XML definition of the element.
   * @return the pointer to the newly created element, or @c NULL when error
   *     occured.
   */
  ElementInterface *AppendElementFromXML(const char *xml);

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
  ElementInterface *InsertElementFromXML(const char *xml,
                                         const ElementInterface *before);

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
  
  /** Notifies all children that the host has changed. */
  void HostChanged();
  
  /** 
   * Notifies all children using relative positioning that the 
   * parent's width changed. 
   * @param width new width of the parent in pixels
   */
  void OnParentWidthChange(double width);
  
  /** 
   * Notifies all children using relative positioning that the 
   * parent's height changed.
   * @param height new height of the parent in pixels 
   */
  void OnParentHeightChange(double height);
  
  /**
   * Draw all the elements in this object onto a canvas that has the same size
   * as that of the parent.
   * @param[out] changed false if the returned canvas is unchanged, true otherwise
   * @return canvas with the elements drawn. NULL if GetCount() == 0.
   */
  const CanvasInterface *Draw(bool *changed);
    
  DEFAULT_OWNERSHIP_POLICY
  SCRIPTABLE_INTERFACE_DECL
  virtual bool IsStrict() const { return true; }

 private:
  internal::ElementsImpl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(Elements);
};

} // namespace ggadget

#endif // GGADGET_ELEMENTS_H__
