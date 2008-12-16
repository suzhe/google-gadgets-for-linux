/*
  Copyright 2008 Google Inc.

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
#include <ggadget/scriptable_holder.h>
#include <ggadget/view_interface.h>

namespace ggadget {

template <typename R> class Slot0;
template <typename R, typename P1> class Slot1;
class CanvasInterface;
class Elements;
class MenuInterface;
class Rectangle;
class View;
class ClipRegion;

class BasicElement: public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0xfd70820c5bbf11dc, ScriptableInterface);

 public:
  /**
   * Constructor.
   * @param view The View which this element belongs to.
   * @param tag_name Type name of this element, must be static const string.
   * @param name Name of this element.
   * @param allow_children If this element can have children elements.
   */
  BasicElement(View *view, const char *tag_name, const char *name,
               bool allow_children);
  virtual ~BasicElement();

 protected:
  virtual void DoRegister();
  virtual void DoClassRegister();

 public:
  enum FlipMode {
    FLIP_NONE = 0,
    FLIP_HORIZONTAL = 1,
    FLIP_VERTICAL = 2,
    FLIP_BOTH = FLIP_HORIZONTAL | FLIP_VERTICAL
  };

 public:
  /** Get the type name of the current object. */
  const char *GetTagName() const;

  /** Retrieves the name of the element.  */
  std::string GetName() const;

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
  BasicElement *GetParentElement();
  /**
   * Retrieves the parent element.
   * @return the pointer to the parent, or @c NULL if the parent is @c View.
   */
  const BasicElement *GetParentElement() const;

  /**
   * Sets the parent element.
   * Only for @c Elements class or who knows how to keep integrity.
   */
  void SetParentElement(BasicElement *parent);

  /**
   * Gets and sets the index of the element in parent.
   * This is not used to change Z-order etc., but is only used by the parent
   * to let the element cache the current index, which must be synchronized
   * with the element tree structure.
   * Only for @c Elements class which knows how to keep integrity.
   */
  size_t GetIndex() const;
  void SetIndex(size_t index);

  /**
   * Enables or disables canvas cache of this element. It can make rendering
   * faster for complex elements but need more memory.
   */
  void EnableCanvasCache(bool enable);

  /** Checks if the canvas cache is enabled. */
  bool IsCanvasCacheEnabled() const;

 public:
  /**
   * Retrieves the width in pixels.
   * Might be overrided by derived classes to provide fake pixel width.
   */
  virtual double GetPixelWidth() const;
  /** Sets the width in pixels. */
  void SetPixelWidth(double width);
  /**
   * Retrieves the height in pixels.
   * Might be overrided by derived classes to provide fake pixel height,
   * eg. ComboBoxElement.
   */
  virtual double GetPixelHeight() const;
  /** Sets the height in pixels. */
  void SetPixelHeight(double height);

  /** Retrieves the width in relative related to the parent. */
  double GetRelativeWidth() const;
  /** Sets the width in relative related to the parent. */
  void SetRelativeWidth(double width);
  /** Retrieves the height in relative related to the parent. */
  double GetRelativeHeight() const;
  /** Sets the height in relative related to the parent. */
  void SetRelativeHeight(double height);

  /** Retrieves the horizontal position in pixelds. */
  double GetPixelX() const;
  /** Sets the horizontal position in pixels. */
  void SetPixelX(double x);
  /** Retrieves the vertical position in pixels. */
  double GetPixelY() const;
  /** Sets the vertical position in pixels. */
  void SetPixelY(double y);

  /** Retrieves the horizontal position in relative related to the parent. */
  double GetRelativeX() const;
  /** Sets the horizontal position in relative related to the parent. */
  void SetRelativeX(double x);
  /** Retrieves the vertical position in relative related to the parent. */
  double GetRelativeY() const;
  /** Sets the vertical position in relative related to the parent. */
  void SetRelativeY(double y);

  /** Retrieves the horizontal pin in pixels. */
  double GetPixelPinX() const;
  /** Sets the horizontal pin in pixels. */
  void SetPixelPinX(double pin_x);
  /** Retrieves the vertical pin in pixels. */
  double GetPixelPinY() const;
  /** Sets the vertical pin in pixels. */
  void SetPixelPinY(double pin_y);

  /** Retrieves the horizontal pin in relative related to the child. */
  double GetRelativePinX() const;
  /** Sets the horizontal pin in relative related to the child. */
  void SetRelativePinX(double pin_x);
  /** Retrieves the vertical pin in relative related to the child. */
  double GetRelativePinY() const;
  /** Sets the vertical pin in relative related to the child. */
  void SetRelativePinY(double pin_y);

  /** Retrieve whether x is relative to its parent element. */
  bool XIsRelative() const;
  /** Retrieve whether y is relative to its parent element. */
  bool YIsRelative() const;
  /** Retrieve whether width is relative to its parent element. */
  bool WidthIsRelative() const;
  /** Retrieve whether height is relative to its parent element. */
  bool HeightIsRelative() const;
  /** Retrieve whether pin x is relative to its width. */
  bool PinXIsRelative() const;
  /** Retrieve whether pin y is relative to its height. */
  bool PinYIsRelative() const;

  /** Retrieve whether width is explicitly specified. */
  bool WidthIsSpecified() const;
  /** Clear the specified width value and use the default. */
  void ResetWidthToDefault();
  /** Retrieve whether height is explicitly specified. */
  bool HeightIsSpecified() const;
  /** Clear the specified height value and use the default. */
  void ResetHeightToDefault();
  /** Retrieve whether x is explicitly specified. */
  bool XIsSpecified() const;
  /** Clear the specified x value and use the default. */
  void ResetXToDefault();
  /** Retrieve whether y is explicitly specified. */
  bool YIsSpecified() const;
  /** Clear the specified y value and use the default. */
  void ResetYToDefault();

  /**
   * Generic @c Variant version of above x, y, width, height, pin_x, pin_y
   * property getters and setters.
   * @see ParsePixelOrRelative() and GetPixelOrRelative() for details of
   * the variant values.
   */
  Variant GetX() const;
  void SetX(const Variant &x);
  Variant GetY() const;
  void SetY(const Variant &y);
  Variant GetWidth() const;
  void SetWidth(const Variant &width);
  Variant GetHeight() const;
  void SetHeight(const Variant &height);
  Variant GetPinX() const;
  void SetPinX(const Variant &pin_x);
  Variant GetPinY() const;
  void SetPinY(const Variant &pin_y);

  /** Retrieves the rotation of the element, in degrees. */
  double GetRotation() const;
  /** Sets the rotation of the element, in degrees. */
  void SetRotation(double rotation);

  /**
   * Gets and sets the hit-test value for this element.
   *
   * Derived classes can override GetHitTest() to return customized hittest
   * value.
   *
   * GetHitTest() returns a hittest value of specified position.
   * The default behavior is: If IsPointIn(x, y) returns true, then returns the
   * hittest value set by SetHitTest() or HT_CLIENT by default, otherwise
   * returns HT_TRANSPARENT.
   */
  virtual ViewInterface::HitTest GetHitTest(double x, double y) const;
  void SetHitTest(ViewInterface::HitTest value);

  /**
   * Gets and sets the cursor to display when the mouse is over this
   * element.
   */
  ViewInterface::CursorType GetCursor() const;
  void SetCursor(ViewInterface::CursorType cursor);

  /**
   * Gets and sets whether this element is a target for drag/drop operations.
   * If it is a drop target, the ondrag* events will fire when a drag/drop
   * operation is initiated by the user.
   */
  bool IsDropTarget() const;
  void SetDropTarget(bool drop_target);

  /**
   * Gets and sets whether or not the element is enabled.
   * Disabled elements do not fire any mouse or keyboard events, but still
   * fires drag and drop events if it is a drop target.
   */
  bool IsEnabled() const;
  void SetEnabled(bool enabled);

  /**
   * Gets and sets the mask bitmap that defines the clipping path for this
   * element. If a pixel in the mask bitmap is black, the pixel is fully
   * transparent, otherwise the alpha value of the pixel will be applied.
   * @see View::LoadImage()
   */
  Variant GetMask() const;
  void SetMask(const Variant &mask);

  /** Gets and sets opacity (valid range: 0 ~ 1) of the element. */
  double GetOpacity() const;
  void SetOpacity(double opacity);

  /** Gets and sets whether or not the element is visible. */
  bool IsVisible() const;
  void SetVisible(bool visible);

  /**
   * Gets and sets the tooltip displayed when the mouse hovers over this
   * element.
   */
  std::string GetTooltip() const;
  void SetTooltip(const std::string &tooltip);

  /**
   * Shows tooltip of this basicElement immediately.
   * The tooltip will be showed just below this element.
   */
  void ShowTooltip();

  /**
   * Gets and sets the flip mode. Default flip mode is @c FLIP_NONE.
   * If flip mode is not @c FLIP_NONE, flipping occurs within the original
   * rectangle of this element before rotation.
   */
  FlipMode GetFlip() const;
  void SetFlip(FlipMode flip);

  /**
   * Checks whether this element is really visible considering transparency
   * and visibility or the ancestors.
   */
  bool IsReallyVisible() const;

  /**
   * Checks whether this element is really enabled considering visibility of
   * itself and the ancestors.
   */
  bool IsReallyEnabled() const;

  /**
   * Checks if an Element is fully opaque, that is:
   * - The composited opacity upto View equals to 1.0 (255), and
   * - Has a fully opaque background, e.g. a background color or image without
   *   alpha channel, and
   * - Has no mask image.
   *
   * This method calls the virtual method HasOpaqueBackground() to check if the
   * element has an opaque background.
   *
   * The caller should not cache the result of this method, because it might be
   * changed if related element properties are changed.
   */
  bool IsFullyOpaque() const;

 public:
  /**
   * Gives the keyboard focus to the element.
   */
  void Focus();
  /**
   * Removes the keyboard focus from the element.
   */
  void KillFocus();
  /**
   * Checks if the element can be focused with the Tab key.
   */
  virtual bool IsTabStop() const;

 public:
  /** Get the associated View of the current element. */
  const View *GetView() const;
  /** Get the associated View of the current element. */
  View *GetView();

  /** Gets the canvas for the element mask. Returns NULL if no mask is set. */
  const CanvasInterface *GetMaskCanvas() const;

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
   * Sets the changed bit to true and if visible and this is not called during
   * @c Layout(), requests the view to be redrawn.
   * Normally don't call this inside a drawing function (i.e. drawing has
   * already started) or there might be extra draw attempts.
   */
  void QueueDraw();

  /**
   * Like QueueDraw, but only request to redraw part of the element.
   * @param rect The rect region to be redrew, in element's coordinates.
   */
  void QueueDrawRect(const Rectangle &rect);

  /**
   * Like QueueDraw, but only request to redraw part of the element.
   * @param region The region to be redrew, in element's coordinates.
   */
  void QueueDrawRegion(const ClipRegion &region);

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
   * Sets the size changed state to false.
   */
  void ClearSizeChanged();

 public: // Coordination translation methods.
  /**
   * Converts coordinates in this element's space to coordinates in a
   * child element.
   *
   * The default implementation should directly call ParentCoordToChildCoord.
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
   * Converts coordinates in a child's space to coordinates in this element.
   *
   * The default implementation should directly call ChildCoordToParentCoord.
   * BasicElement implementation should override this method if it supports
   * scrolling.
   *
   * @param child a child element of this element.
   * @param x x-coordinate in child element's space to convert.
   * @param y y-coordinate in child element's space to convert.
   * @param[out] self_x parameter to store the converted self x-coordinate.
   * @param[out] self_y parameter to store the converted self y-coordinate.
   */
  virtual void ChildCoordToSelfCoord(const BasicElement *child,
                                     double x, double y,
                                     double *self_x, double *self_y) const;

  /**
   * Converts coordinates in this element's space to coordinates in its
   * parent element or the view if it has no parent.
   *
   * @param x x-coordinate in this element's space to convert.
   * @param y y-coordinate in this element's space to convert.
   * @param[out] parent_x parameter to store the converted parent x-coordinate.
   * @param[out] parent_y parameter to store the converted parent y-coordinate.
   */
  void SelfCoordToParentCoord(double x, double y,
                              double *parent_x, double *parent_y) const;

  /**
   * Converts coordinates in this element's parent space (or view space if it
   * has not parent)  to coordinates in itself.
   *
   * @param parent_x x-coordinate in this element's parent space to convert.
   * @param parent_y y-coordinate in this element's parent space to convert.
   * @param[out] x parameter to store the converted self x-coordinate.
   * @param[out] y parameter to store the converted self y-coordinate.
   */
  void ParentCoordToSelfCoord(double parent_x, double parent_y,
                              double *x, double *y) const;

  /**
   * Converts coordinates in this element's space to coordinates in the top
   * level view.
   *
   * This function uses SelfCoordToParentCoord() and traverses its parents upto
   * the view to calculate the coordinates.
   *
   * @param x x-coordinate in this element's space to convert.
   * @param y y-coordinate in this element's space to convert.
   * @param[out] view_x parameter to store the converted view x-coordinate.
   * @param[out] view_y parameter to store the converted view y-coordinate.
   */
  void SelfCoordToViewCoord(double x, double y,
                            double *view_x, double *view_y) const;

  /**
   * Converts coordinates in the top level view space to coordinates in the
   * element itself.
   *
   * This function uses ParentCoordToSelfCoord() and traverses its parents upto
   * the view to calculate the coordinates.
   *
   * @param view_x x-coordinate in the top level view space to convert.
   * @param view_y y-coordinate in the top level view space to convert.
   * @param[out] view_x parameter to store the converted self x-coordinate.
   * @param[out] view_y parameter to store the converted self y-coordinate.
   */
  void ViewCoordToSelfCoord(double view_x, double view_y,
                            double *self_x, double *self_y) const;

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
   * Get the element's extents information in its view's coordinates.
   * @return the rectangle containing the information.
   */
  Rectangle GetExtentsInView() const;

  /**
   * Get a rectangle's extents information in the view's coordinates.
   * @param rect The rectangle in element's coordinates.
   * @return the rectangle containing the information.
   */
  Rectangle GetRectExtentsInView(const Rectangle &rect) const;

 public: // Event handlers and event related methods.
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
   * @param[out] hittest result of this mouse event. It's the hittest value of
   *     in_element, if there is no in_element, the return value is undefined.
   * @return result of event handling.
   */
  virtual EventResult OnMouseEvent(const MouseEvent &event,
                                   bool direct,
                                   BasicElement **fired_element,
                                   BasicElement **in_element,
                                   ViewInterface::HitTest *hittest);

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
  virtual bool IsPointIn(double x, double y) const;

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

public: // Other overridable public methods.
  /** Gets the client width (pixel width - width of scrollbar etc. if any). */
  virtual double GetClientWidth() const;
  /** Gets the client height (pixel width - height of scrollbar etc. if any). */
  virtual double GetClientHeight() const;

  /**
   * Adjusts the layout (e.g. size, position, etc.) of this element and its
   * children. This method is called just before @c Draw().
   * The implementation of this method of the derived classes must call through
   * that of the base classes first.
   */
  virtual void Layout();

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

  /**
   * Checks if a child is in its parent's visible area.
   * The default implemetation always returns true. Derived classes shall
   * overwrite this method to implement the correct behavior.
   *
   * @param child a pointer to a child element of this element.
   * @return Returns true if the specified child is in a visible area of this
   *         element.
   */
  virtual bool IsChildInVisibleArea(const BasicElement *child) const;

  /**
   * Checks if an Element has opaque background.
   *
   * The default implementation always returns false. Derived classes shall
   * overwrite this method to implement the correct behavior.
   *
   * This function is mainly for drawing optimization, so it's safe to leave
   * the default implementation unchanged if the derived class can't
   * determine if its background is fully opaque or not.
   *
   * This function is mainly called by IsReallyOpaque().
   */
  virtual bool HasOpaqueBackground() const;

 public:
  /** Enum used by ParsePixelOrRelative() below. */
  enum ParsePixelOrRelativeResult {
    PR_PIXEL,
    PR_RELATIVE,
    PR_UNSPECIFIED,
    PR_INVALID = -1
  };

  /**
   * Parses an Variant into either a absolute value (@c Variant::TYPE_INTEGER
   * or @c Variant::TYPE_DOUBLE) or an relative percentage value
   * (@c Variant::TYPE_STRING).
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
  bool IsDesignerMode() const;
  void SetDesignerMode(bool designer_mode);

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
  Connection *ConnectOnContextMenuEvent(Slot0<void> *handler);

  /**
   * Special signal which will be emitted when this element or any of its
   * children is changed.
   */
  Connection *ConnectOnContentChanged(Slot0<void> *handler);

 protected:
  /**
   * Called by the descendant classes when the onsize event should be fired.
   *
   * This method posts the onsize event to main loop, so that it'll be fired
   * in the next main loop iteration.
   */
  void PostSizeEvent();

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
  virtual void DoDraw(CanvasInterface *canvas);

  /**
   * To be overriden by a subclass if it need to handle mouse events.
   * @return EVENT_RESULT_UNHANDLED by default.
   */
  virtual EventResult HandleMouseEvent(const MouseEvent &event);
  /**
   * To be overriden by a subclass if it need to handle dragging events.
   * @return EVENT_RESULT_UNHANDLED by default.
   */
  virtual EventResult HandleDragEvent(const DragEvent &event);
  /**
   * To be overriden by a subclass if it need to handle keyboard events.
   * @return EVENT_RESULT_UNHANDLED by default.
   */
  virtual EventResult HandleKeyEvent(const KeyboardEvent &event);
  /**
   * To be overriden by a subclass if it need to handle other events.
   * @return EVENT_RESULT_UNHANDLED by default.
   */
  virtual EventResult HandleOtherEvent(const Event &event);

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

typedef ScriptableHolder<BasicElement> ElementHolder;

} // namespace ggadget

#endif // GGADGET_BASIC_ELEMENT_H__
