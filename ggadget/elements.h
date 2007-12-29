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

#include <ggadget/common.h>
#include <ggadget/event.h>
#include <ggadget/elements_interface.h>
#include <ggadget/scriptable_helper.h>

namespace ggadget {

class BasicElement;
class CanvasInterface;
class View;

/**
 * Elements is used for storing and managing a set of objects which
 * implement the @c ElementInterface.
 */
class Elements : public ScriptableHelper<ElementsInterface> {
 public:
  DEFINE_CLASS_ID(0xe3bdb064cb794282, ElementsInterface)

  /**
   * Create an Elements object and assign the given factory to it.
   * @param factory the factory used to create the child elements.
   * @param owner the parent element. Can be @c null for top level elements
   *     owned directly by a view.
   * @param view the containing view.
   */
  Elements(ElementFactoryInterface *factory, BasicElement *owner, View *view);

  virtual ~Elements();

 public: // ElementsInterface methods.
  virtual int GetCount() const;
  virtual ElementInterface *GetItemByIndex(int child);
  virtual const ElementInterface *GetItemByIndex(int child) const;
  virtual ElementInterface *GetItemByName(const char *child);
  virtual const ElementInterface *GetItemByName(const char *child) const;
  virtual ElementInterface *AppendElement(const char *tag_name,
                                          const char *name);
  virtual ElementInterface *InsertElement(const char *tag_name,
                                          const ElementInterface *before,
                                          const char *name);
  virtual ElementInterface *AppendElementFromXML(const char *xml);
  virtual ElementInterface *InsertElementFromXML(
      const char *xml, const ElementInterface *before);
  virtual bool RemoveElement(ElementInterface *element);
  virtual void RemoveAllElements();

 public:
  /**
   * Adjusts the layout (e.g. size, position, etc.) of children.
   * This method is called just before @c Draw().
   */
  void Layout();

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
   * @param[out] fired_element the element who processed the event, or
   *     @c NULL if no one.
   * @param[out] in_element the child element where the mouse is in (including
   *     disabled child elements, but not invisible child elements).
   * @return result of event handling.
   */
  EventResult OnMouseEvent(const MouseEvent &event,
                           BasicElement **fired_element,
                           BasicElement **in_element);

  /**
   * Handler of the drag and drop events.
   * @param event the darg and drop event.
   * @param[out] fired_element the element who processed the event, or
   *     @c NULL if no one.
   * @return result of event handling.
   */
  EventResult OnDragEvent(const DragEvent &event,
                          BasicElement **fired_element);

  /**
   * Sets if the drawing contents can be scrolled within the parent.
   */
  void SetScrollable(bool scrollable);

  /**
   * Gets the maximum extents of children.
   * If not scrollable, the returned size equals to the size of the parent
   * element (or view if no parent). If scrollable, the returned value is the
   * actual max extents of children.
   */
  void GetChildrenExtents(double *width, double *height);

  /**
   * Sets a redraw mark, so that all things and children will be redrawed
   * during the next call of Draw().
   */
  void MarkRedraw();

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(Elements);
};

} // namespace ggadget

#endif // GGADGET_ELEMENTS_H__
