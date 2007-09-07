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

#ifndef GGADGETS_BASIC_ELEMENT_IMPL_H__
#define GGADGETS_BASIC_ELEMENT_IMPL_H__

#include <string>
#include "element_interface.h"
#include "static_scriptable.h"

namespace ggadget {

class ViewInterface;
class ElementsInterface;
class Elements;

namespace internal {

class BasicElementImpl {
 public:
  BasicElementImpl(ElementInterface *parent,
                   ViewInterface *view,
                   const char *name,
                   ElementInterface *owner);
  ~BasicElementImpl();
  ViewInterface *GetView() const;
  ElementInterface::HitTest GetHitTest() const;
  void SetHitTest(ElementInterface::HitTest value);
  ElementsInterface *GetChildren();
  ElementInterface::CursorType GetCursor() const;
  void SetCursor(ElementInterface::CursorType cursor);
  bool IsDropTarget() const;
  void SetDropTarget(bool drop_target);
  bool IsEnabled() const;
  void SetEnabled(bool enabled);
  const char *GetName() const;
  const char *GetMask() const;
  void SetMask(const char *mask);
  double GetPixelWidth() const;
  void SetPixelWidth(double width);
  double GetPixelHeight() const;
  void SetPixelHeight(double height);
  double GetRelativeWidth() const;
  void SetRelativeWidth(double width);
  double GetRelativeHeight() const;
  void SetRelativeHeight(double height);
  double GetPixelX() const;
  void SetPixelX(double x);
  double GetPixelY() const;
  void SetPixelY(double y);
  double GetRelativeX() const;
  void SetRelativeX(double x);
  double GetRelativeY() const;
  void SetRelativeY(double y);
  double GetPixelPinX() const;
  void SetPixelPinX(double pin_x);
  double GetPixelPinY() const;
  void SetPixelPinY(double pin_y);
  double GetRelativePinX() const;
  void SetRelativePinX(double pin_x);
  double GetRelativePinY() const;
  void SetRelativePinY(double pin_y);
  double GetRotation() const;
  void SetRotation(double rotation);
  double GetOpacity() const;
  void SetOpacity(double opacity);
  bool IsVisible() const;
  void SetVisible(bool visible);
  ElementInterface *GetParentElement() const;
  const char *GetToolTip() const;
  void SetToolTip(const char *tool_tip);
  ElementInterface *AppendElement(const char *tag_name, const char *name);
  ElementInterface *InsertElement(const char *tag_name,
                                  const ElementInterface *before,
                                  const char *name);
  bool RemoveElement(ElementInterface *child);
  void RemoveAllElements();
  void Focus();
  void KillFocus();

  double GetParentWidth() const;
  double GetParentHeight() const;

  bool XIsRelative() const;
  bool YIsRelative() const;
  bool WidthIsRelative() const;
  bool HeightIsRelative() const;
  bool PinXIsRelative() const;
  bool PinYIsRelative() const;

 public:
  void WidthChanged();
  void HeightChanged();

 public:
  ElementInterface *parent_;
  Elements *children_;
  ViewInterface *view_;
  ElementInterface::HitTest hittest_;
  ElementInterface::CursorType cursor_;
  bool drop_target_;
  bool enabled_;
  std::string name_;
  double pin_x_;
  double pin_y_;
  double ppin_x_;
  double ppin_y_;
  bool pin_x_relative_;
  bool pin_y_relative_;
  double rotation_;
  double opacity_;
  bool visible_;
  std::string tool_tip_;
  std::string mask_;
  double width_;
  double height_;
  double x_;
  double y_;
  double pwidth_;
  double pheight_;
  double px_;
  double py_;
  double width_relative_;
  double height_relative_;
  double x_relative_;
  double y_relative_;
  StaticScriptable static_scriptable_;
};

} // namespace internal

} // namespace ggadget

#endif // GGADGETS_BASIC_ELEMENT_IMPL_H__
