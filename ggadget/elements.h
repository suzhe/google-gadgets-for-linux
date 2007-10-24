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
#include "scriptable_helper.h"

namespace ggadget {

namespace internal {

class ElementsImpl;

} // namespace internal

class CanvasInterface;
class ElementInterface;
class ElementFactoryInterface;
class ViewInterface;
class MouseEvent;

/**
 * Elements is used for storing and managing a set of objects which
 * implement the @c ElementInterface.
 */
class Elements : public ScriptableHelper<ScriptableInterface> {
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
  /**
   * @return number of children.
   */
  int GetCount() const;

  /**
   * Returns the element identified by the index.
   * @param child the index of the child.
   * @return the pointer to the specified element. If the parameter is out of
   *     range, @c NULL is returned.
   */
  ElementInterface *GetItemByIndex(int child);
  const ElementInterface *GetItemByIndex(int child) const;

  /**
   * Returns the element identified by the name.
   * @param child the name of the child.
   * @return the pointer to the specified element. If multiple elements are
   *     defined with the same name, returns the first one. Returns @c NULL if
   *     no elements match.
   */
  ElementInterface *GetItemByName(const char *child);
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

  /**
   * Notifies all children using relative positioning that the
   * parent's width changed.
   * @param width new width of the parent in pixels.
   */
  void OnParentWidthChange(double width);

  /**
   * Notifies all children using relative positioning that the
   * parent's height changed.
   * @param height new height of the parent in pixels.
   */
  void OnParentHeightChange(double height);

  /**
   * Draw all the elements in this object onto a canvas that has the same size
   * as that of the parent.
   * @param[out] changed @c false if the returned canvas is unchanged, @c true
   *     otherwise.
   * @return canvas with the elements drawn. @c NULL if
   *     <code>GetCount() == 0</code>.
   */
  const CanvasInterface *Draw(bool *changed);

  /**
   * Handler of the mouse events.
   * @param event the mouse event.
   * @param[out] fired_event the element who processed the event, or
   *     @c NULL if no one.
   * @return @c false to disable the default handling of this event, or
   *     @c true otherwise.
   */
  bool OnMouseEvent(MouseEvent *event, ElementInterface **fired_element);

  /**
   * Sets if the drawing contents can be scrolled within the parent.
   */
  void SetScrollable(bool scrollable);

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(Elements);
};

} // namespace ggadget

#endif // GGADGET_ELEMENTS_H__
