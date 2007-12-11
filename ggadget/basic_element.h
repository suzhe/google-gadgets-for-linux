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
#include <ggadget/element_interface.h>
#include <ggadget/scriptable_helper.h>

namespace ggadget {

class CanvasInterface;
class Elements;
class View;

class BasicElement : public ScriptableHelper<ElementInterface> {
 public:
  CLASS_ID_DECL(0xfd70820c5bbf11dc);

 public:
  BasicElement(BasicElement *parent, View *view,
               const char *tag_name, const char *name, Elements *children);
  virtual ~BasicElement();

 public: // ElementInterface methods.
  virtual const char *GetTagName() const;
  virtual HitTest GetHitTest() const;
  virtual void SetHitTest(HitTest value);
  virtual const ElementsInterface *GetChildren() const;
  virtual ElementsInterface *GetChildren();
  virtual CursorType GetCursor() const;
  virtual void SetCursor(CursorType cursor);
  virtual bool IsDropTarget() const;
  virtual void SetDropTarget(bool drop_target);
  virtual bool IsEnabled() const;
  virtual void SetEnabled(bool enabled);
  virtual const char *GetName() const;
  virtual const char *GetMask() const;
  virtual void SetMask(const char *mask);
  virtual double GetPixelWidth() const;
  virtual double GetPixelHeight() const;
  virtual double GetRelativeWidth() const;
  virtual double GetRelativeHeight() const;
  virtual void SetPixelWidth(double width);
  virtual void SetPixelHeight(double height);
  virtual void SetRelativeWidth(double width);
  virtual void SetRelativeHeight(double height);
  virtual double GetPixelX() const;
  virtual void SetPixelX(double x);
  virtual double GetPixelY() const;
  virtual void SetPixelY(double y);
  virtual double GetRelativeX() const;
  virtual double GetRelativeY() const;
  virtual double GetPixelPinX() const;
  virtual void SetPixelPinX(double pin_x);
  virtual double GetPixelPinY() const;
  virtual void SetPixelPinY(double pin_y);
  virtual void SetRelativeX(double x);
  virtual void SetRelativeY(double y);
  virtual double GetRelativePinX() const;
  virtual void SetRelativePinX(double pin_x);
  virtual double GetRelativePinY() const;
  virtual void SetRelativePinY(double pin_y);
  virtual bool XIsRelative() const;
  virtual bool YIsRelative() const;
  virtual bool WidthIsRelative() const;
  virtual bool HeightIsRelative() const;
  virtual bool PinXIsRelative() const;
  virtual bool PinYIsRelative() const;
  virtual bool WidthIsSpecified() const;
  virtual void ResetWidthToDefault();
  virtual bool HeightIsSpecified() const;
  virtual void ResetHeightToDefault();
  virtual double GetClientWidth();
  virtual double GetClientHeight();

  virtual Variant GetWidth() const;
  virtual void SetWidth(const Variant &width);
  virtual Variant GetHeight() const;
  virtual void SetHeight(const Variant &height);
  virtual Variant GetX() const;
  virtual void SetX(const Variant &y);
  virtual Variant GetY() const;
  virtual void SetY(const Variant &y);
  virtual void ResetXToDefault();
  virtual void ResetYToDefault();

  virtual double GetRotation() const;
  virtual void SetRotation(double rotation);
  virtual double GetOpacity() const;
  virtual void SetOpacity(double opacity);

  virtual bool IsVisible() const;
  virtual void SetVisible(bool visible);

  virtual ElementInterface *GetParentElement();
  virtual const ElementInterface *GetParentElement() const;
  virtual const char *GetTooltip() const;
  virtual void SetTooltip(const char *tooltip);

  virtual void Focus();
  virtual void KillFocus();
  virtual Connection *ConnectEvent(const char *event_name,
                                   Slot0<void> *handler);

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
   */
  virtual void Layout();

  /**
   * Draws the current element to a canvas. The caller does NOT own this canvas
   * and should not free it.
   * @param[out] changed True if the returned canvas is different from that
   *   of the last call, false otherwise.
   * @return A canvas suitable for drawing. NULL if element is not visible.
   */
  const CanvasInterface *Draw(bool *changed);

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
   */
  bool IsPointIn(double x, double y);

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
   * Element implementation should override this method if it supports
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

 public:
  /**
   * Sets the changed bit to true and if visible and this is not called during
   * @c Layout(), requests the view to be redrawn. 
   * Normally don't call this inside a drawing function (i.e. drawing has
   * already started) or there might be extra draw attempts.
   */
  void QueueDraw();

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

 protected:

  /**
   * Draws the element onto the canvas.
   * To be implemented by subclasses.
   * @param canvas the canvas to draw the element on.
   * @param children_canvas the canvas containing composited children.
   */
  virtual void DoDraw(CanvasInterface *canvas,
                      const CanvasInterface *children_canvas) = 0;

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

CLASS_ID_IMPL(BasicElement, ElementInterface)

} // namespace ggadget

#endif // GGADGET_BASIC_ELEMENT_H__
