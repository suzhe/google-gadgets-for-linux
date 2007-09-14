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

#include "element_interface.h"

namespace ggadget {

namespace internal {

class BasicElementImpl;

}

class ViewInterface;
class Elements;

class BasicElement : public ElementInterface {
 public:
  CLASS_ID_DECL(0xfd70820c5bbf11dc);

 public:
  BasicElement(ElementInterface *parent,
               ViewInterface *view,
               const char *name);
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
  virtual const char *GetToolTip() const;
  virtual void SetToolTip(const char *tool_tip);
  virtual ElementInterface *AppendElement(const char *tag_name,
                                          const char *name);
  virtual ElementInterface *InsertElement(const char *tag_name,
                                          const ElementInterface *before,
                                          const char *name);
  virtual bool RemoveElement(ElementInterface *child);
  virtual void RemoveAllElements();
  virtual void Focus();
  virtual void KillFocus();

  virtual bool XIsRelative() const;
  virtual bool YIsRelative() const;
  virtual bool WidthIsRelative() const;
  virtual bool HeightIsRelative() const;
  virtual bool PinXIsRelative() const;
  virtual bool PinYIsRelative() const;

  virtual bool IsStrict() const { return true; }
  SCRIPTABLE_INTERFACE_DECL
  DEFAULT_OWNERSHIP_POLICY

 private:
  internal::BasicElementImpl *impl_;
};

CLASS_ID_IMPL(BasicElement, ElementInterface)

} // namespace ggadget

#endif // GGADGETS_BASIC_ELEMENT_H__
