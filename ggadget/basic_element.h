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

#ifndef GGADGETS_BASIC_ELEMENT_H__
#define GGADGETS_BASIC_ELEMENT_H__

#include "common.h"
#include "element_interface.h"
#include "scriptable_helper.h"

namespace ggadget {

class ViewInterface;
class Elements;

class BasicElement : public ElementInterface {
 public:
  CLASS_ID_DECL(0xfd70820c5bbf11dc);

 public:
  BasicElement(ElementInterface *parent,
               ViewInterface *view,
               const char *name,
               bool is_container);
  virtual ~BasicElement();

 public:
  virtual void Destroy();
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
  virtual void SetPixelWidth(double width);
  virtual double GetPixelHeight() const;
  virtual void SetPixelHeight(double height);
  virtual double GetRelativeWidth() const;
  virtual double GetRelativeHeight() const;
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
  virtual void SetRelativeWidth(double width);
  virtual void SetRelativeHeight(double height);
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
  
  /** 
   * Notifies the element and its children that the host has changed.
   * Classes overriding this should remember to call children_.HostChanged().
   */ 
  virtual void HostChanged();
  
  virtual const CanvasInterface *Draw(bool *changed);
  
  /** 
   * Call this when drawing to initialize and prepare a canvas of the right
   * height and width for drawing.
   */
  void SetUpCanvas();
  
  /**
   * Checks to see if the current element has changed and needs to be redrawn.
   * Note that it does not check any child elements, so the element may still
   * need to be redrawn even if this method returns false.
   * Also, this method does not consider visiblity changes, or changes in 
   * position in relation to the parent.
   * @return true if the element has changed, false otherwise.
   */
  virtual bool IsSelfChanged() const;
  /** Sets the self changed state to false. */
  virtual void ClearSelfChanged();
  /** 
   * Checks to see if visibility of the element has changed since the last draw.
   */
  virtual bool IsVisibilityChanged() const;
  /** Sets the visibility changed state to false. */
  virtual void ClearVisibilityChanged();
  
  virtual bool IsPositionChanged() const;
  virtual void ClearPositionChanged();

  virtual void OnParentWidthChange(double width);
  virtual void OnParentHeightChange(double height);

  DEFAULT_OWNERSHIP_POLICY
  DELEGATE_SCRIPTABLE_INTERFACE(scriptable_helper_)
  virtual bool IsStrict() const { return true; }

 protected:
  DELEGATE_SCRIPTABLE_REGISTER(scriptable_helper_)

 private:
  class Impl;
  Impl *impl_;
  ScriptableHelper scriptable_helper_;
  DISALLOW_EVIL_CONSTRUCTORS(BasicElement);
};

CLASS_ID_IMPL(BasicElement, ElementInterface)

} // namespace ggadget

#endif // GGADGETS_BASIC_ELEMENT_H__
