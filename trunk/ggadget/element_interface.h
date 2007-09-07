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

#include "scriptable_interface.h"
#include "variant.h"

namespace ggadget {

class ElementsInterface;
class ViewInterface;

/**
 * ElementInterface defines the properties, methods and events exposed on all
 * elements, with specific elements inheriting what's defined here. When a
 * method is said to return an @c element, that means an element defined in the
 * gadget's XML definition, descended from @c BasicElement.
 */
class ElementInterface : public ScriptableInterface {
 public:
  CLASS_ID_DECL(0xe863ac4167fa4bba);

  /** Get the type of the current object. */
  virtual const char *GetTagName() const = 0;
  /** Destroy the current object. */
  virtual void Destroy() = 0;

  /** Get the associated View of the current element. */
  virtual const ViewInterface *GetView() const = 0;
  /** Get the associated View of the current element. */
  virtual ViewInterface *GetView() = 0;

  /**
   * Retrieves a collection that contains the immediate children of this
   *     element.
   */
  virtual const ElementsInterface *GetChildren() const = 0;
  /**
   * Retrieves a collection that contains the immediate children of this
   *     element.
   */
  virtual ElementsInterface *GetChildren() = 0;

  /**
   * Retrieves the cursor to display when the mouse is over this element.
   * @see SetCursor for possible values.
   */
  virtual const char *GetCursor() const = 0;
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
  virtual bool SetCursor(const char *cursor) = 0;

  /**
   * Retrieves whether this element is a target for drag/drop operations.
   * @see set_drop_target.
   */
  virtual bool IsDropTarget() const = 0;
  /**
   * Sets whether this element is a target for drag/drop operations.
   * @param drop_target is true, the ondrag* events will fire when a drag/drop
   *     oeration is initiated by the user.
   */
  virtual bool SetDropTarget(bool drop_target) = 0;

  /**
   * Retrieves whether or not the element is enabled.
   * @see set_enabled.
   */
  virtual bool IsEnabled() const = 0;
  /**
   * Sets whether or not the element is enabled.
   * Disabled elements do not fire any mouse or keyboard events.
   */
  virtual bool SetEnabled(bool enabled) = 0;

  /** Retrieves the name of the element.  */
  virtual const char *GetName() const = 0;

  /**
   * Retrieves the mask bitmap that defines the clipping path for this element.
   */
  virtual const char *GetMask() const = 0;
  /**
   * Sets the mask bitmap that defines the clipping path for this element.
   */
  virtual bool SetMask(const char *mask) const = 0;

  /**
   * Retrieves the width.
   * @see set_width.
   */
  virtual Variant GetWidth() const = 0;
  /**
   * Sets the width.
   * @param width the value can be expressed in pixels or as a percentage of
   * the parent's width.
   */
  virtual bool SetWidth(Variant width) = 0;
  /**
   * Retrieves the height.
   * @see set_height.
   */
  virtual Variant GetHeight() const = 0;
  /**
   * Sets the height.
   * @param height the value can be expressed in pixels or as a percentage of
   *     the parent's height.
   */
  virtual bool SetHeight(Variant height) = 0;
  /**
   * Retrieves the width of the element relative to its parent element, in
   * pixels. This mimics the same-named DHTML property.
   */
  virtual int GetOffsetWidth() const = 0;
  /**
   * Retrieves the height of the element relative to its parent element, in
   * pixels. This mimics the same-named DHTML property.
   */
  virtual int GetOffsetHeight() const = 0;

  /**
   * Retrieves the horizontal position.
   * @see set_x.
   */
  virtual Variant GetX() const = 0;
  /**
   * Sets the horizontal position.
   * @param x the value can be expressed in pixels or as a percentage of the
   *     parent's width.
   */
  virtual bool SetX(Variant x) = 0;
  /**
   * Retrieves the vertical position.
   * @see set_y.
   */
  virtual Variant GetY() const = 0;
  /**
   * Sets the vertical position.
   * @param y the value can be expressed in pixels or as a percentage of the
   *     parent's height.
   */
  virtual bool SetY(Variant y) = 0;
  /**
   * Retrieves the x position of the element relative to its parent element, in
   * pixels. This mimics the same-named DHTML property.
   */
  virtual int GetOffsetX() const = 0;
  /**
   * Retrieves the y position of the element relative to its parent element, in
   * pixels. This mimics the same-named DHTML property.
   */
  virtual int GetOffsetY() const = 0;

  /** Retrieves the horizontal pin. */
  virtual int GetPinX() const = 0;
  /** Sets the horizontal pin. */
  virtual bool SetPinX(int pin_x) = 0;
  /** Retrieves the vertical pin. */
  virtual int GetPinY() const = 0;
  /** Sets the vertical pin. */
  virtual bool SetPinY(int pin_y) = 0;

  /**
   * Retrieves the rotation of the element, in degrees.
   */
  virtual double GetRotation() const = 0;
  /**
   * Sets the rotation of the element, in degrees.
   */
  virtual bool SetRotation(double rotation) = 0;

  /**
   * Retrieves the opacity of the element.
   * @see set_opacity.
   */
  virtual int GetOpacity() const = 0;
  /**
   * Sets the opacity of the element.
   * @param opacity valid range: 0 ~ 255.
   */
  virtual bool SetOpacity(int opacity) = 0;

  /**
   * Retrieves whether or not the element is visible.
   */
  virtual bool IsVisible() const = 0;
  /**
   * Sets whether or not the element is visible.
   */
  virtual bool SetVisible(bool visible) = 0;

  /**
   * Retrieves the parent element.
   * @return the pointer to the parent, or @c NULL if the parent is @c View.
   */
  virtual ElementInterface *GetParentElement() = 0;
  /**
   * Retrieves the parent element.
   * @return the pointer to the parent, or @c NULL if the parent is @c View.
   */
  virtual const ElementInterface *GetParentElement() const = 0;

  /**
   * Retrieves the tooltip displayed when the mouse hovers over this element.
   */
  virtual const char *GetToolTip() const = 0;
  /**
   * Sets the tooltip displayed when the mouse hovers over this element.
   */
  virtual bool SetToolTip(const char *tool_tip) = 0;

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

CLASS_ID_IMPL(ElementInterface, ScriptableInterface)

} // namespace ggadget

#endif // GGADGET_ELEMENT_INTERFACE_H__
