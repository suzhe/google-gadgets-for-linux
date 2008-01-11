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

#ifndef GGADGET_BASIC_ELEMENT_H__
#define GGADGET_BASIC_ELEMENT_H__

#include <ggadget/common.h>
#include <ggadget/scriptable_helper.h>
#include <ggadget/view_host_interface.h>

namespace ggadget {

class CanvasInterface;
class Elements;
class MenuInterface;
class View;

class BasicElement : public ScriptableHelper<ScriptableInterface> {
 public:
  DEFINE_CLASS_ID(0xfd70820c5bbf11dc, ScriptableInterface);

 public:
  BasicElement(BasicElement *parent, View *view,
               const char *tag_name, const char *name, bool children);
  virtual ~BasicElement();

 public:
  // TODO: If need this work, perhaps we should define it in ViewHostInterface.
  enum HitTest {
    HT_DEFAULT,
    HT_TRANSPARENT,
    HT_NOWHERE,
    HT_CLIENT,
    HT_CAPTION,
    HT_SYSMENU,
    HT_SIZE,
    HT_MENU,
    HT_HSCROLL,
    HT_VSCROLL,
    HT_MINBUTTON,
    HT_MAXBUTTON,
    HT_LEFT,
    HT_RIGHT,
    HT_TOP,
    HT_TOPLEFT,
    HT_TOPRIGHT,
    HT_BOTTOM,
    HT_BOTTOMLEFT,
    HT_BOTTOMRIGHT,
    HT_BORDER,
    HT_OBJECT,
    HT_CLOSE,
    HT_HELP
  };

 public:
  /** Get the type of the current object. */
  virtual std::string GetTagName() const;

  /** Retrieves the name of the element.  */
  virtual std::string GetName() const;

  /**
   * Retrieves a collection that contains the immediate children of this
   * element.
   */
  virtual const Elements *GetChildren() const;
  /**
   * Retrieves a collection that contains the immediate children of this
   * element.
   */
  virtual Elements *GetChildren();

  /**
   * Retrieves the parent element.
   * @return the pointer to the parent, or @c NULL if the parent is @c View.
   */
  virtual BasicElement *GetParentElement();
  /**
   * Retrieves the parent element.
   * @return the pointer to the parent, or @c NULL if the parent is @c View.
   */
  virtual const BasicElement *GetParentElement() const;

 public:
  /** Retrieves the width in pixels. */
  virtual double GetPixelWidth() const;
  /** Sets the width in pixels. */
  virtual void SetPixelWidth(double width);
  /** Retrieves the height in pixels. */
  virtual double GetPixelHeight() const;
  /** Sets the height in pixels. */
  virtual void SetPixelHeight(double height);

  /** Retrieves the width in relative related to the parent. */
  virtual double GetRelativeWidth() const;
  /** Sets the width in relative related to the parent. */
  virtual void SetRelativeWidth(double width);
  /** Retrieves the height in relative related to the parent. */
  virtual double GetRelativeHeight() const;
  /** Sets the height in relative related to the parent. */
  virtual void SetRelativeHeight(double height);

  /** Retrieves the horizontal position in pixelds. */
  virtual double GetPixelX() const;
  /** Sets the horizontal position in pixels. */
  virtual void SetPixelX(double x);
  /** Retrieves the vertical position in pixels. */
  virtual double GetPixelY() const;
  /** Sets the vertical position in pixels. */
  virtual void SetPixelY(double y);

  /** Retrieves the horizontal position in relative related to the parent. */
  virtual double GetRelativeX() const;
  /** Sets the horizontal position in relative related to the parent. */
  virtual void SetRelativeX(double x);
  /** Retrieves the vertical position in relative related to the parent. */
  virtual double GetRelativeY() const;
  /** Sets the vertical position in relative related to the parent. */
  virtual void SetRelativeY(double y);

  /** Retrieves the horizontal pin in pixels. */
  virtual double GetPixelPinX() const;
  /** Sets the horizontal pin in pixels. */
  virtual void SetPixelPinX(double pin_x);
  /** Retrieves the vertical pin in pixels. */
  virtual double GetPixelPinY() const;
  /** Sets the vertical pin in pixels. */
  virtual void SetPixelPinY(double pin_y);

  /** Retrieves the horizontal pin in relative related to the child. */
  virtual double GetRelativePinX() const;
  /** Sets the horizontal pin in relative related to the child. */
  virtual void SetRelativePinX(double pin_x);
  /** Retrieves the vertical pin in relative related to the child. */
  virtual double GetRelativePinY() const;
  /** Sets the vertical pin in relative related to the child. */
  virtual void SetRelativePinY(double pin_y);

  /** Retrieves the rotation of the element, in degrees. */
  virtual double GetRotation() const;
  /** Sets the rotation of the element, in degrees. */
  virtual void SetRotation(double rotation);

  /** Retrieve whether x is relative to its parent element. */
  virtual bool XIsRelative() const;
  /** Retrieve whether y is relative to its parent element. */
  virtual bool YIsRelative() const;
  /** Retrieve whether width is relative to its parent element. */
  virtual bool WidthIsRelative() const;
  /** Retrieve whether height is relative to its parent element. */
  virtual bool HeightIsRelative() const;
  /** Retrieve whether pin x is relative to its width. */
  virtual bool PinXIsRelative() const;
  /** Retrieve whether pin y is relative to its height. */
  virtual bool PinYIsRelative() const;

  /** Retrieve whether width is explicitly specified. */
  virtual bool WidthIsSpecified() const;
  /** Clear the specified width value and use the default. */
  virtual void ResetWidthToDefault();
  /** Retrieve whether height is explicitly specified. */
  virtual bool HeightIsSpecified() const;
  /** Clear the specified height value and use the default. */
  virtual void ResetHeightToDefault();
  /** Retrieve whether x is explicitly specified. */
  virtual bool XIsSpecified() const;
  /** Clear the specified x value and use the default. */
  virtual void ResetXToDefault();
  /** Retrieve whether y is explicitly specified. */
  virtual bool YIsSpecified() const;
  /** Clear the specified y value and use the default. */
  virtual void ResetYToDefault();

  /** Gets the client width (pixel width - width of scrollbar etc. if any). */
  virtual double GetClientWidth() const;
  /** Gets the client height (pixel width - height of scrollbar etc. if any). */
  virtual double GetClientHeight() const;

  /** Retrieves the hit-test value for this element. */
  HitTest GetHitTest() const;
  /** Sets the hit-test value for this element. */
  void SetHitTest(HitTest value);

  /** Retrieves the cursor to display when the mouse is over this element. */
  ViewHostInterface::CursorType GetCursor() const;
  /** Sets the cursor to display when the mouse is over this element. */
  void SetCursor(ViewHostInterface::CursorType cursor);

  /**
   * Retrieves whether this element is a target for drag/drop operations.
   */
  bool IsDropTarget() const;
  /**
   * Sets whether this element is a target for drag/drop operations.
   * @param drop_target is true, the ondrag* events will fire when a drag/drop
   *     oeration is initiated by the user.
   */
  void SetDropTarget(bool drop_target);

  /**
   * Retrieves whether or not the element is enabled.
   */
  bool IsEnabled() const;
  /**
   * Sets whether or not the element is enabled.
   * Disabled elements do not fire any mouse or keyboard events.
   */
  void SetEnabled(bool enabled);

  /**
   * Retrieves the mask bitmap that defines the clipping path for this element.
   */
  std::string GetMask() const;
  /**
   * Sets the mask bitmap that defines the clipping path for this element.
   */
  void SetMask(const char *mask);

  /**
   * Retrieves the opacity of the element.
   */
  double GetOpacity() const;
  /**
   * Sets the opacity of the element.
   * @param opacity valid range: 0 ~ 1.
   */
  void SetOpacity(double opacity);

  /**
   * Retrieves whether or not the element is visible.
   */
  bool IsVisible() const;
  /**
   * Sets whether or not the element is visible.
   */
  void SetVisible(bool visible);

  /**
   * Retrieves the tooltip displayed when the mouse hovers over this element.
   */
  std::string GetTooltip() const;
  /**
   * Sets the tooltip displayed when the mouse hovers over this element.
   */
  void SetTooltip(const char *tooltip);

  /**
   * Checks whether this element is really visible considering transparency
   * and visibility or the ancestors.
   */
  bool ReallyVisible() const;

  /**
   * Checks whether this element is really enabled considering visibility of
   * itself and the ancestors.
   */
  bool ReallyEnabled() const;

 public:
  /**
   * Gives the keyboard focus to the element.
   */
  void Focus();
  /**
   * Removes the keyboard focus from the element.
   */
  void KillFocus();

 public:
  /**
   * Gets and sets whether this element is an implicit child of its parent.
   * An implicit child is created by its parent merely for implementation.
   * It is invisible from the script and the view/element hierarchy.
   * For example, the div element contains an implicit scrollbar child.
   */
  bool IsImplicit() const;
  void SetImplicit(bool implicit);

  /** Get the associated View of the current element. */
  const View *GetView() const;
  /** Get the associated View of the current element. */
  View *GetView();

  /** Gets the canvas for the element mask. Returns NULL if no mask is set. */
  const CanvasInterface *GetMaskCanvas();

  /**
   * Adjusts the layout (e.g. size, position, etc.) of this element and its
   * children. This method is called just before @c Draw().
   * The implementation of this method of the derived classes must call through
   * that of the base classes first.
   */
  virtual void Layout();

  /**
   * Draws the element to a specified canvas.
   * The canvas should already be prepared for this element to be drawn
   * directly without any transformation, except the opacity.
   * This function is intent to be called by the element's parent.
   * Derived classes shall override DoDraw() function to perform the real draw
   * task.
   * The element shall not clear the whole canvas, because it might be shared
   * among all elements.
   * @param canvas A canvas on which the content of the element shall be drawn.
   */
  void Draw(CanvasInterface *canvas);

  /**
   * Handler of the mouse events. Normally subclasses should not override this
   * method except it needs special event handling.
   * Override @c HandleMouseEvent() if need to process mouse events.
   * @param event the mouse event.
   * @param direct if @c true, this event is sent to the element directly, so
   *     it should not dispatch it to its children.
   * @param[out] fired_element the element who processed the event, or
   *     @c NULL if no one.
   * @param[out] in_element the child element where the mouse is in (including
   *     disabled child elements, but not invisible child elements).
   * @return result of event handling.
   */
  virtual EventResult OnMouseEvent(const MouseEvent &event,
                                   bool direct,
                                   BasicElement **fired_element,
                                   BasicElement **in_element);

  /**
   * Handler of the drag and drop events.
   * Normally subclasses should not override this method.
   * Override @c HandleDragEvent() if need to process dragging events.
   * @param event the darg and drop event.
   * @param direct if @c true, this event is sent to the element directly, so
   *     it should not dispatch it to its children.
   * @param[out] fired_element the element who processed the event, or
   *     @c NULL if no one.
   * @return result of event handling.
   */
  virtual EventResult OnDragEvent(const DragEvent &event,
                                  bool direct,
                                  BasicElement **fired_element);

  /**
   * Check if the position of a position event (mouse or drag) is in the
   * element.
   * The default implementation of BasicElement checks if the point is
   * inside the rectangle boundary and mask of the element.
   * Derived class may override this method to do more refined check, such as
   * checking the opacity of the point as well.
   * Derived class must call this method of its parent first, then does more
   * check when parent method returns true.
   */
  virtual bool IsPointIn(double x, double y);

  /**
   * Handler of the keyboard events.
   * Normally subclasses should not override this method.
   * Override @c HandleKeyEvent() if need to process keyboard events.
   * @param event the keyboard event.
   * @return result of event handling.
   */
  virtual EventResult OnKeyEvent(const KeyboardEvent &event);

  /**
   * Handler for other events.
   * Normally subclasses should not override this method.
   * Override @c HandleKeyEvent() if need to process other events.
   * @param event the event.
   * @return result of event handling.
   */
  virtual EventResult OnOtherEvent(const Event &event);

  /**
   * Called to let this element add customized context menu items.
   * @return @c false if the handler doesn't want the default menu items shown.
   *     If no menu item is added in this handler, and @c false is returned,
   *     the host won't show the whole context menu.
   */
  virtual bool OnAddContextMenuItems(MenuInterface *menu);

  /**
   * Called when this element is no longer the current popup element.
   * (There's no need for a popup on message right now.)
   */
  virtual void OnPopupOff();

  /**
   * Checks to see if position of the element has changed relative to the
   * parent since the last draw. Specifically, this checks for changes in
   * x, y, pinX, pinY and rotation.
   */
  bool IsPositionChanged() const;

  /**
   * Sets the position changed state to false.
   */
  void ClearPositionChanged();

  /**
   * Checks to see if size of the element has changed since the last draw.
   * Unlike @c IsPositionChanged(), this flag is cleared in Draw() because
   * Draw() will be always called if size changed.
   */
  bool IsSizeChanged() const;

  /**
   * Converts coordinates in a this element's space to coordinates in a
   * child element.
   *
   * The default implementation should directly call ParentCoorddToChildCoord.
   * BasicElement implementation should override this method if it supports
   * scrolling.
   *
   * @param child a child element of this element.
   * @param x x-coordinate in this element's space to convert.
   * @param y y-coordinate in this element's space to convert.
   * @param[out] child_x parameter to store the converted child x-coordinate.
   * @param[out] child_y parameter to store the converted child y-coordinate.
   */
  virtual void SelfCoordToChildCoord(const BasicElement *child,
                                     double x, double y,
                                     double *child_x, double *child_y) const;

  /**
   * Delegates to @c Elements::SetScrollable().
   * @return @c false if this element is not a container.
   */
  bool SetChildrenScrollable(bool scrollable);

  /**
   * Delegates to @c Elements::GetChildrenExtents().
   * @return @c false if this element is not a container.
   */
  bool GetChildrenExtents(double *width, double *height);

  /**
   * Sets the changed bit to true and if visible and this is not called during
   * @c Layout(), requests the view to be redrawn.
   * Normally don't call this inside a drawing function (i.e. drawing has
   * already started) or there might be extra draw attempts.
   */
  void QueueDraw();

  /**
   * Sets a redraw mark, so that all things and children will be redrawed
   * during the next call of Draw().
   *
   * Derived class must implement this function if it has private children,
   * to make sure that its children will be marked as well.
   * The derived implementation must call through this function of its parent.
   *
   * The element doesn't need to call QueueDraw() in this function, the upper
   * level will take care of it.
   */
  virtual void MarkRedraw();

 public:
  /** Enum used by ParsePixelOrRelative() below. */
  enum ParsePixelOrRelativeResult {
    PR_PIXEL,
    PR_RELATIVE,
    PR_UNSPECIFIED,
    PR_INVALID = -1,
  };

  /**
   * Parses an Variant into either a absolute value or
   * an relative percentage value.
   */
  static ParsePixelOrRelativeResult ParsePixelOrRelative(const Variant &input,
                                                         double *output);
  /**
   * Returns a Variant depending on whether the input is either absolute
   * pixel value or a relative percentage value.
   */
  static Variant GetPixelOrRelative(bool is_relative, bool is_specified,
                                    double pixel, double relative);

 public:
  // Event handler connection methods.
  Connection *ConnectOnClickEvent(Slot0<void> *handler);
  Connection *ConnectOnDblClickEvent(Slot0<void> *handler);
  Connection *ConnectOnRClickEvent(Slot0<void> *handler);
  Connection *ConnectOnRDblClickEvent(Slot0<void> *handler);
  Connection *ConnectOnDragDropEvent(Slot0<void> *handler);
  Connection *ConnectOnDragOutEvent(Slot0<void> *handler);
  Connection *ConnectOnDragOverEvent(Slot0<void> *handler);
  Connection *ConnectOnFocusInEvent(Slot0<void> *handler);
  Connection *ConnectOnFocusOutEvent(Slot0<void> *handler);
  Connection *ConnectOnKeyDownEvent(Slot0<void> *handler);
  Connection *ConnectOnKeyPressEvent(Slot0<void> *handler);
  Connection *ConnectOnKeyUpEvent(Slot0<void> *handler);
  Connection *ConnectOnMouseDownEvent(Slot0<void> *handler);
  Connection *ConnectOnMouseMoveEvent(Slot0<void> *handler);
  Connection *ConnectOnMouseOverEvent(Slot0<void> *handler);
  Connection *ConnectOnMouseOutEvent(Slot0<void> *handler);
  Connection *ConnectOnMouseUpEvent(Slot0<void> *handler);
  Connection *ConnectOnMouseWheelEvent(Slot0<void> *handler);
  Connection *ConnectOnSizeEvent(Slot0<void> *handler);

 protected:
  /**
   * Draws all children elements onto the specified canvas.
   * This function is intend to be called inside DoDraw() function of derived
   * class to draw its children elements.
   */
  void DrawChildren(CanvasInterface *canvas);

  /**
   * Draws the element onto the canvas.
   * To be implemented by subclasses. If the element has children,
   * DrawChildren() function shall be called inside this function to draw
   * all children.
   * @param canvas the canvas to draw the element on.
   */
  virtual void DoDraw(CanvasInterface *canvas) = 0;

  /** To be overriden by a subclass if it need to handle mouse events. */
  virtual EventResult HandleMouseEvent(const MouseEvent &event) {
    return EVENT_RESULT_UNHANDLED;
  }
  /** To be overriden by a subclass if it need to handle dragging events. */
  virtual EventResult HandleDragEvent(const DragEvent &event) {
    return EVENT_RESULT_UNHANDLED;
  }
  /** To be overriden by a subclass if it need to handle keyboard events. */
  virtual EventResult HandleKeyEvent(const KeyboardEvent &event) {
    return EVENT_RESULT_UNHANDLED;
  }
  /** To be overriden by a subclass if it need to handle other events. */
  virtual EventResult HandleOtherEvent(const Event &event) {
    return EVENT_RESULT_UNHANDLED;
  }

  /**
   * Return the default size of the element in pixels.
   * The default size is used when no "width" or "height" property is specified
   * for the element.
   * The default value of default size is (0,0).
   */
  virtual void GetDefaultSize(double *width, double *height) const;

  /**
   * Return the default position of the element in pixels.
   * The default position is used when no "x" or "y" property is specified
   * for the element.
   * The default value is (0,0).
   */
  virtual void GetDefaultPosition(double *x, double *y) const;

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(BasicElement);
};

} // namespace ggadget

#endif // GGADGET_BASIC_ELEMENT_H__
