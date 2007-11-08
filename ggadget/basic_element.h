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

class ViewInterface;
class Elements;

class BasicElement : public ScriptableHelper<ElementInterface> {
 public:
  CLASS_ID_DECL(0xfd70820c5bbf11dc);

 public:
  BasicElement(ElementInterface *parent,
               ViewInterface *view,
               const char *tag_name,
               const char *name,
               bool is_container);
  virtual ~BasicElement();

 public:
  virtual void Destroy();
  virtual const char *GetTagName() const;
  virtual ViewInterface *GetView();
  virtual const ViewInterface *GetView() const;
  virtual HitTest GetHitTest() const;
  virtual void SetHitTest(HitTest value);
  virtual const Elements *GetChildren() const;
  virtual Elements *GetChildren();
  virtual CursorType GetCursor() const;
  virtual void SetCursor(CursorType cursor);
  virtual bool IsDropTarget() const;
  virtual void SetDropTarget(bool drop_target);
  virtual bool IsEnabled() const;
  virtual void SetEnabled(bool enabled);
  virtual const char *GetName() const;
  virtual const char *GetMask() const;
  virtual void SetMask(const char *mask);
  virtual const CanvasInterface *GetMaskCanvas();
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

  virtual bool XIsRelative() const;
  virtual bool YIsRelative() const;
  virtual bool WidthIsRelative() const;
  virtual bool HeightIsRelative() const;
  virtual bool PinXIsRelative() const;
  virtual bool PinYIsRelative() const;
  virtual bool WidthIsSpecified() const;
  virtual bool HeightIsSpecified() const;

  virtual const CanvasInterface *Draw(bool *changed);

  virtual void OnParentWidthChange(double width);
  virtual void OnParentHeightChange(double height);

  virtual bool OnMouseEvent(MouseEvent *event, bool direct,
                            ElementInterface **fired_element);
  virtual bool IsMouseEventIn(MouseEvent *event);
  virtual bool OnKeyEvent(KeyboardEvent *event);
  virtual bool OnOtherEvent(Event *event);

  virtual bool IsPositionChanged() const;
  virtual void ClearPositionChanged();

  virtual void SelfCoordToChildCoord(ElementInterface *child,
                                     double x, double y,
                                     double *child_x, double *child_y);

  /** 
   * Sets the changed bit to true and if visible, 
   * requests the view to be redrawn. 
   */
  virtual void QueueDraw();
  
  /** Called by child classes when the default size changed. */
  void OnDefaultSizeChanged();

 protected:
  /**
   * Draws the element onto the canvas.
   * To be implemented by subclasses.
   * @param canvas the canvas to draw the element on.
   * @param children_canvas the canvas containing composited children.
   */
  virtual void DoDraw(CanvasInterface *canvas,
                      const CanvasInterface *children_canvas) = 0;

  /**
   * Return the default size of the element in pixels.
   * The default size is used when no "width" or "height" property is specified
   * for the element.
   */
  virtual void GetDefaultSize(double *width, double *height) const;

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(BasicElement);
};

CLASS_ID_IMPL(BasicElement, ElementInterface)

} // namespace ggadget

#endif // GGADGET_BASIC_ELEMENT_H__
