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
#include "canvas_interface.h"
#include "element_interface.h"
#include "elements.h"
#include "signal.h"

namespace ggadget {

class ViewInterface;
class ElementFactoryInterface;

namespace internal {

class BasicElementImpl {
 public:
  BasicElementImpl(ElementInterface *parent,
                   ViewInterface *view,
                   ElementFactoryInterface *element_factory,
                   const char *name,
                   BasicElement *owner);
  ~BasicElementImpl();
  ViewInterface *GetView() const;
  ElementInterface::HitTest GetHitTest() const;
  void SetHitTest(ElementInterface::HitTest value);
  Elements *GetChildren();
  ElementInterface::CursorType GetCursor() const;
  void SetCursor(ElementInterface::CursorType cursor);
  bool IsDropTarget() const;
  void SetDropTarget(bool drop_target);
  bool IsEnabled() const;
  void SetEnabled(bool enabled);
  const char *GetName() const;
  const char *GetMask() const;
  void SetMask(const char *mask);
  const CanvasInterface *GetMaskCanvas();
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
  const char *GetTooltip() const;
  void SetTooltip(const char *tooltip);
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

  Variant GetWidth() const;
  void SetWidth(const Variant &width);
  Variant GetHeight() const;
  void SetHeight(const Variant &height);
  Variant GetX() const;
  void SetX(const Variant &x);
  Variant GetY() const;
  void SetY(const Variant &y);
  Variant GetPinX() const;
  void SetPinX(const Variant &pin_x);
  Variant GetPinY() const;
  void SetPinY(const Variant &pin_y);

  void HostChanged();
  const CanvasInterface *Draw(bool *changed);
  void SetUpCanvas();
  
 public:
  void WidthChanged();
  void HeightChanged();

 public:
  ElementInterface *parent_;
  Elements children_;
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
  std::string tooltip_;
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
  CanvasInterface *canvas_;
  CanvasInterface *mask_canvas_;
  bool visibility_changed_;
  bool changed_;
  bool position_changed_;
  
  EventSignal onclick_event_;
  EventSignal ondblclick_event_;
  EventSignal ondragdrop_event_;
  EventSignal ondragout_event_;
  EventSignal ondragover_event_;
  EventSignal onfocusin_event_;
  EventSignal onfocusout_event_;
  EventSignal onkeydown_event_;
  EventSignal onkeypress_event_;
  EventSignal onkeyup_event_;
  EventSignal onmousedown_event_;
  EventSignal onmousemove_event_;
  EventSignal onmouseout_event_;
  EventSignal onmouseover_event_;
  EventSignal onmouseup_event_;
  EventSignal onmousewheel_event_;
};

} // namespace internal

} // namespace ggadget

#endif // GGADGETS_BASIC_ELEMENT_IMPL_H__
