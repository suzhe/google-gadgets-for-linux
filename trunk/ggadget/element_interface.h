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

#ifndef GGADGET_ELEMENT_INTERFACE_H__
#define GGADGET_ELEMENT_INTERFACE_H__

#include "variant.h"

namespace ggadget {

class ElementsInterface;

/**
 * ElementInterface defines the properties, methods and events exposed on all
 * elements, with specific elements inheriting what's defined here. When a
 * method is said to return an @c element, that means an element defined in the
 * gadget's XML definition, descended from @c BasicElement.
 */
class ElementInterface {
 public:
  /** Get the type of the current object. */
  virtual const char *type() const = 0;
  /** Destroy the current object. */
  virtual void Release() = 0;

  /**
   * Retrieves a collection that contains the immediate children of this
   *     element.
   */
  virtual const ElementsInterface *children() const = 0;
  /**
   * Retrieves a collection that contains the immediate children of this
   *     element.
   */
  virtual ElementsInterface *children() = 0;

  /**
   * Retrieves the cursor to display when the mouse is over this element.
   * @see set_cursor for possible values.
   */
  virtual const char *cursor() const = 0;
  /**
   * Sets the cursor to display when the mouse is over this element.
   * @param cursor possible values:
   *    - @c "arrow"
   *    - @c "ibeam"
   *    - @c "wait"
   *    - @c "cross"
   *    - @c "uparrow"
   *    - @c "size"
   *    - @c "sizenwse"
   *    - @c "sizenesw"
   *    - @c "sizewe"
   *    - @c "sizens"
   *    - @c "sizeall"
   *    - @c "no"
   *    - @c "hand"
   *    - @c "busy"
   *    - @c "help"
   */
  virtual bool set_cursor(const char *cursor) = 0;

  /**
   * Retrieves whether this element is a target for drag/drop operations.
   * @see set_drop_target.
   */
  virtual bool drop_target() const = 0;
  /**
   * Sets whether this element is a target for drag/drop operations.
   * @param drop_target is true, the ondrag* events will fire when a drag/drop
   *     oeration is initiated by the user.
   */
  virtual bool set_drop_target(bool drop_target) = 0;

  /**
   * Retrieves whether or not the element is enabled.
   * @see set_enabled.
   */
  virtual bool enabled() const = 0;
  /**
   * Sets whether or not the element is enabled.
   * Disabled elements do not fire any mouse or keyboard events.
   */
  virtual bool set_enabled(bool enabled) = 0;

  /** Retrieves the name of the element.  */
  virtual const char *name() const = 0;

  /**
   * Retrieves the mask bitmap that defines the clipping path for this element.
   */
  virtual const char *mask() const = 0;
  /**
   * Sets the mask bitmap that defines the clipping path for this element.
   */
  virtual bool set_mask(const char *mask) const = 0;

  /**
   * Retrieves the width.
   * @see set_width.
   */
  virtual Variant width() const = 0;
  /**
   * Sets the width.
   * @param width the value can be expressed in pixels or as a percentage of
   * the parent's width.
   */
  virtual bool set_width(Variant width) = 0;
  /**
   * Retrieves the height.
   * @see set_height.
   */
  virtual Variant height() const = 0;
  /**
   * Sets the height.
   * @param height the value can be expressed in pixels or as a percentage of
   *     the parent's height.
   */
  virtual bool set_height(Variant height) = 0;
  /**
   * Retrieves the width of the element relative to its parent element, in
   * pixels. This mimics the same-named DHTML property.
   */
  virtual int offset_width() const = 0;
  /**
   * Retrieves the height of the element relative to its parent element, in
   * pixels. This mimics the same-named DHTML property.
   */
  virtual int offset_height() const = 0;

  /**
   * Retrieves the horizontal position.
   * @see set_x.
   */
  virtual Variant x() const = 0;
  /**
   * Sets the horizontal position.
   * @param x the value can be expressed in pixels or as a percentage of the
   *     parent's width.
   */
  virtual bool set_x(Variant x) = 0;
  /**
   * Retrieves the vertical position.
   * @see set_y.
   */
  virtual Variant y() const = 0;
  /**
   * Sets the vertical position.
   * @param y the value can be expressed in pixels or as a percentage of the
   *     parent's height.
   */
  virtual bool set_y(Variant y) = 0;
  /**
   * Retrieves the x position of the element relative to its parent element, in
   * pixels. This mimics the same-named DHTML property.
   */
  virtual int offset_x() const = 0;
  /**
   * Retrieves the y position of the element relative to its parent element, in
   * pixels. This mimics the same-named DHTML property.
   */
  virtual int offset_y() const = 0;

  /** Retrieves the horizontal pin. */
  virtual int pin_x() const = 0;
  /** Sets the horizontal pin. */
  virtual bool set_pin_x(int pin_x) = 0;
  /** Retrieves the vertical pin. */
  virtual int pin_y() const = 0;
  /** Sets the vertical pin. */
  virtual bool set_pin_y(int pin_y) = 0;

  /**
   * Retrieves the rotation of the element, in degrees.
   */
  virtual double rotation() const = 0;
  /**
   * Sets the rotation of the element, in degrees.
   */
  virtual bool set_rotation(double rotation) = 0;

  /**
   * Retrieves the opacity of the element.
   * @see set_opacity.
   */
  virtual int opacity() const = 0;
  /**
   * Sets the opacity of the element.
   * @param opacity valid range: 0 ~ 255.
   */
  virtual bool set_opacity(int opacity) = 0;

  /**
   * Retrieves whether or not the element is visible.
   */
  virtual bool visible() const = 0;
  /**
   * Sets whether or not the element is visible.
   */
  virtual bool set_visible(bool visible) = 0;

  /**
   * Retrieves the parent element.
   * @return the pointer to the parent, or @c NULL if the parent is @c View.
   */
  virtual ElementInterface *parent_element() = 0;
  /**
   * Retrieves the parent element.
   * @return the pointer to the parent, or @c NULL if the parent is @c View.
   */
  virtual const ElementInterface *parent_element() const = 0;

  /**
   * Retrieves the tooltip displayed when the mouse hovers over this element.
   */
  virtual const char *tool_tip() const = 0;
  /**
   * Sets the tooltip displayed when the mouse hovers over this element.
   */
  virtual bool set_tool_tip(const char *tool_tip) = 0;

 public:
  /**
   * Appends an element as the last child of this element.
   * This method is appropriate only for elements (such as div, listbox, and
   * listitem) that contain other elements.
   * @return the newly created element or @c NULL if this method is not
   *     allowed.
   */
  virtual ElementInterface *AppendElement(const char *tag_name) = 0;
  /**
   * Insert an element immediately before the specified element.
   * This method is appropriate only for elements (such as div, listbox, and
   * listitem) that contain other elements.
   * If the specified element is not the direct child of this element, the
   * newly created element will be append as the last child of this element.
   * @return the newly created element or @c NULL if this method is not
   *     allowed.
   */
  virtual ElementInterface *InsertElement(const char *tag_name,
                                          const ElementInterface *before) = 0;
  /**
   * Remove the specified child from this element.
   * @param child the element to remove.
   * @return @c true if removed successfully, or @c false if the specified
   *     element doesn't exists or not the direct child of the container.
   */
  virtual bool RemoveElement(ElementInterface *child) = 0;
  /**
   * Removes and destroys all immediate children of this element.
   * This method is appropriate only for elements (such as div, listbox, and
   * listitem) that contain other elements.
   */
  virtual void RemoveAllElements() = 0;
  /**
   * Gives the keyboard focus to the element.
   */
  virtual void Focus() = 0;
  /**
   * Removes the keyboard focus from the element.
   */
  virtual void KillFocus() = 0;
};

};

#endif // GGADGET_ELEMENT_INTERFACE_H__
