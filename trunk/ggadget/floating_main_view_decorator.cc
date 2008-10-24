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

#include "floating_main_view_decorator.h"

#include <string>
#include <algorithm>
#include "logger.h"
#include "common.h"
#include "elements.h"
#include "gadget_consts.h"
#include "gadget.h"
#include "signals.h"
#include "slot.h"
#include "view.h"
#include "view_host_interface.h"
#include "div_element.h"
#include "img_element.h"
#include "messages.h"
#include "menu_interface.h"

namespace ggadget {

static const double kVDMainBorderWidth = 6;
static const double kVDMainBackgroundOpacity = 0.618;

class FloatingMainViewDecorator::Impl {
  struct ResizeBorderInfo {
    double x;           // relative x
    double y;           // relative y
    double pin_x;       // relative pin x
    double pin_y;       // relative pin y
    double width;       // pixel width < 0 means relative width = 1.0
    double height;      // pixel height < 0 means relative height = 1.0
    ViewInterface::CursorType cursor;
    ViewInterface::HitTest hittest;
  };

  enum ResizeBorderId {
    RESIZE_LEFT = 0,
    RESIZE_TOP,
    RESIZE_RIGHT,
    RESIZE_BOTTOM,
    RESIZE_TOP_LEFT,
    RESIZE_BOTTOM_LEFT,
    RESIZE_TOP_RIGHT,
    RESIZE_BOTTOM_RIGHT,
    NUMBER_OF_RESIZE_BORDERS
  };

  static const ResizeBorderInfo kResizeBordersInfo[];

 public:
  Impl(FloatingMainViewDecorator *owner, bool transparent_background)
    : owner_(owner),
      show_decorator_(false),
      transparent_(transparent_background),
      background_(new ImgElement(owner, NULL)),
      resize_border_(new DivElement(owner, NULL)),
      zoom_corner_(new DivElement(owner, NULL)) {
    background_->SetSrc(Variant(transparent_ ? kVDMainBackgroundTransparent :
                                kVDMainBackground));
    background_->SetOpacity(transparent_ ? 1 : kVDMainBackgroundOpacity);
    background_->SetVisible(false);
    background_->SetStretchMiddle(true);
    background_->EnableCanvasCache(true);
    background_->SetEnabled(false);
    owner->InsertDecoratorElement(background_, true);

    // Setup resize borders.
    for (size_t i = 0; i < NUMBER_OF_RESIZE_BORDERS; ++i) {
      BasicElement *elm = new BasicElement(owner, NULL, NULL, false);
      const ResizeBorderInfo *info = &kResizeBordersInfo[i];
      elm->SetRelativeX(info->x);
      elm->SetRelativeY(info->y);
      elm->SetRelativePinX(info->pin_x);
      elm->SetRelativePinY(info->pin_y);
      if (info->width > 0)
        elm->SetPixelWidth(info->width);
      else
        elm->SetRelativeWidth(1);
      if (info->height > 0)
        elm->SetPixelHeight(info->height);
      else
        elm->SetRelativeHeight(1);
      elm->SetCursor(info->cursor);
      elm->SetHitTest(info->hittest);
      resize_border_->GetChildren()->InsertElement(elm, NULL);
    }
    resize_border_->SetVisible(false);
    resize_border_->SetEnabled(false);
    owner->InsertDecoratorElement(resize_border_, false);

    // Setup zoom corner
    ImgElement *corner_img = new ImgElement(owner, NULL);
    corner_img->SetSrc(Variant(kVDBottomRightCorner));
    corner_img->SetVisible(true);
    corner_img->SetEnabled(false);
    corner_img->SetHitTest(ViewInterface::HT_BOTTOMRIGHT);
    corner_img->SetCursor(ViewInterface::CURSOR_SIZENWSE);
    zoom_corner_->GetChildren()->InsertElement(corner_img, NULL);
    zoom_corner_->SetVisible(false);
    zoom_corner_->SetPixelWidth(corner_img->GetSrcWidth());
    zoom_corner_->SetPixelHeight(corner_img->GetSrcHeight());
    zoom_corner_->SetRelativeX(1);
    zoom_corner_->SetRelativeY(1);
    zoom_corner_->SetRelativePinX(1);
    zoom_corner_->SetRelativePinY(1);
    zoom_corner_->SetHitTest(ViewInterface::HT_BOTTOMRIGHT);
    zoom_corner_->SetCursor(ViewInterface::CURSOR_SIZENWSE);
    owner->InsertDecoratorElement(zoom_corner_, false);

    if (!transparent_) {
      background_->SetVisible(true);
      background_->SetPixelX(0);
      background_->SetPixelY(0);
      background_->SetRelativeWidth(1);
      background_->SetRelativeHeight(1);
      resize_border_->SetPixelX(0);
      resize_border_->SetPixelY(0);
      resize_border_->SetRelativeWidth(1);
      resize_border_->SetRelativeHeight(1);
    }
  }

  // Returns true if the decorator border shall be shown.
  bool UpdateResizeBorder() {
    // Update resize border visibility.
    Elements *children = resize_border_->GetChildren();
    ResizableMode resizable = owner_->GetChildViewResizable();
    bool minimized = owner_->IsMinimized();
    bool vertical = ((resizable == RESIZABLE_TRUE ||
                      resizable == RESIZABLE_KEEP_RATIO) &&
                     !minimized);
    bool horizontal = (resizable == RESIZABLE_TRUE ||
                       resizable == RESIZABLE_KEEP_RATIO ||
                       minimized);
    bool both = vertical && horizontal;
    children->GetItemByIndex(RESIZE_TOP)->SetVisible(vertical);
    children->GetItemByIndex(RESIZE_BOTTOM)->SetVisible(vertical);
    children->GetItemByIndex(RESIZE_LEFT)->SetVisible(horizontal);
    children->GetItemByIndex(RESIZE_RIGHT)->SetVisible(horizontal);
    children->GetItemByIndex(RESIZE_TOP_LEFT)->SetVisible(both);
    children->GetItemByIndex(RESIZE_TOP_RIGHT)->SetVisible(both);
    children->GetItemByIndex(RESIZE_BOTTOM_LEFT)->SetVisible(both);
    children->GetItemByIndex(RESIZE_BOTTOM_RIGHT)->SetVisible(both);

    if (vertical || horizontal) {
      // Update resize border size.
      double left, top, right, bottom;
      bool specified = false;
      View *child = owner_->GetChildView();
      if (!minimized && child)
        specified = child->GetResizeBorder(&left, &top, &right, &bottom);

      if (!specified) {
        left = kVDMainBorderWidth;
        top = kVDMainBorderWidth;
        right = kVDMainBorderWidth;
        bottom = kVDMainBorderWidth;
      }

      children->GetItemByIndex(RESIZE_LEFT)->SetPixelWidth(left);
      children->GetItemByIndex(RESIZE_TOP)->SetPixelHeight(top);
      children->GetItemByIndex(RESIZE_RIGHT)->SetPixelWidth(right);
      children->GetItemByIndex(RESIZE_BOTTOM)->SetPixelHeight(bottom);
      children->GetItemByIndex(RESIZE_TOP_LEFT)->SetPixelWidth(left);
      children->GetItemByIndex(RESIZE_TOP_LEFT)->SetPixelHeight(top);
      children->GetItemByIndex(RESIZE_TOP_RIGHT)->SetPixelWidth(right);
      children->GetItemByIndex(RESIZE_TOP_RIGHT)->SetPixelHeight(top);
      children->GetItemByIndex(RESIZE_BOTTOM_LEFT)->SetPixelWidth(left);
      children->GetItemByIndex(RESIZE_BOTTOM_LEFT)->SetPixelHeight(bottom);
      children->GetItemByIndex(RESIZE_BOTTOM_RIGHT)->SetPixelWidth(right);
      children->GetItemByIndex(RESIZE_BOTTOM_RIGHT)->SetPixelHeight(bottom);

      return !specified;
    }
    return false;
  }

  void UpdateDecoratorVisibility() {
    bool show_border = UpdateResizeBorder();
    if (show_decorator_) {
      ResizableMode resizable = owner_->GetChildViewResizable();
      if (resizable == RESIZABLE_TRUE || owner_->IsMinimized()) {
        // background will always be shown if it's not transparent.
        if (transparent_)
          background_->SetVisible(show_border);
        resize_border_->SetVisible(true);
        zoom_corner_->SetVisible(false);
      } else {
        // background for transparent mode is only visible when view is
        // resizable.
        if (transparent_)
          background_->SetVisible(false);
        resize_border_->SetVisible(false);
        zoom_corner_->SetVisible(true);
      }
    } else {
      if (transparent_)
        background_->SetVisible(false);
      resize_border_->SetVisible(false);
      zoom_corner_->SetVisible(false);
    }
  }

  // Returns the edge besides the button box.
  double *GetBackgroundMargins(double *left, double *top,
                               double *right, double *bottom) {
    ButtonBoxPosition button_position = owner_->GetButtonBoxPosition();
    ButtonBoxOrientation button_orientation = owner_->GetButtonBoxOrientation();

    *left = 0;
    *top = 0;
    *right = 0;
    *bottom = 0;

    double *btn_edge = NULL;
    double btn_margin = 0;
    double btn_width, btn_height;
    owner_->GetButtonBoxSize(&btn_width, &btn_height);
    if (button_orientation == HORIZONTAL) {
      btn_margin = btn_height;
      if (button_position == TOP_LEFT || button_position == TOP_RIGHT)
        btn_edge = top;
      else
        btn_edge = bottom;
    } else {
      btn_margin = btn_width;
      if (button_position == TOP_LEFT || button_position == BOTTOM_LEFT)
        btn_edge = left;
      else
        btn_edge = right;
    }

    *btn_edge = btn_margin;
    return btn_edge;
  }

  bool IsChildViewHasResizeBorder() {
    View *child = owner_->GetChildView();
    double left, top, right, bottom;
    return child && child->GetResizeBorder(&left, &top, &right, &bottom);
  }

  void DockMenuCallback(const char *) {
    on_dock_signal_();
  }

 public:
  FloatingMainViewDecorator *owner_;

  bool show_decorator_;
  bool transparent_;

  ImgElement *background_;
  DivElement *resize_border_;
  DivElement *zoom_corner_;

  Signal0<void> on_dock_signal_;
};

const FloatingMainViewDecorator::Impl::ResizeBorderInfo
FloatingMainViewDecorator::Impl::kResizeBordersInfo[] = {
  { 0, 0, 0, 0, kVDMainBorderWidth, -1,
    ViewInterface::CURSOR_SIZEWE, ViewInterface::HT_LEFT },
  { 0, 0, 0, 0, -1, kVDMainBorderWidth,
    ViewInterface::CURSOR_SIZENS, ViewInterface::HT_TOP },
  { 1, 0, 1, 0, kVDMainBorderWidth, -1,
    ViewInterface::CURSOR_SIZEWE, ViewInterface::HT_RIGHT },
  { 0, 1, 0, 1, -1, kVDMainBorderWidth,
    ViewInterface::CURSOR_SIZENS, ViewInterface::HT_BOTTOM },
  { 0, 0, 0, 0, kVDMainBorderWidth, kVDMainBorderWidth,
    ViewInterface::CURSOR_SIZENWSE, ViewInterface::HT_TOPLEFT },
  { 0, 1, 0, 1, kVDMainBorderWidth, kVDMainBorderWidth,
    ViewInterface::CURSOR_SIZENESW, ViewInterface::HT_BOTTOMLEFT },
  { 1, 0, 1, 0, kVDMainBorderWidth, kVDMainBorderWidth,
    ViewInterface::CURSOR_SIZENESW, ViewInterface::HT_TOPRIGHT },
  { 1, 1, 1, 1, kVDMainBorderWidth, kVDMainBorderWidth,
    ViewInterface::CURSOR_SIZENWSE, ViewInterface::HT_BOTTOMRIGHT }
};

FloatingMainViewDecorator::FloatingMainViewDecorator(
    ViewHostInterface *host, bool transparent_background)
  : MainViewDecoratorBase(host, "main_view_floating", false, false,
                          transparent_background),
    impl_(new Impl(this, transparent_background)) {
}

FloatingMainViewDecorator::~FloatingMainViewDecorator() {
  delete impl_;
  impl_ = NULL;
}

Connection *FloatingMainViewDecorator::ConnectOnDock(Slot0<void> *slot) {
  return impl_->on_dock_signal_.Connect(slot);
}

void FloatingMainViewDecorator::SetResizable(ResizableMode resizable) {
  MainViewDecoratorBase::SetResizable(resizable);
  impl_->UpdateDecoratorVisibility();
}

void FloatingMainViewDecorator::DoLayout() {
  // Call through parent's DoLayout();
  MainViewDecoratorBase::DoLayout();

  double left, top, right, bottom;
  double width = GetWidth();
  double height = GetHeight();
  impl_->GetBackgroundMargins(&left, &top, &right, &bottom);

  if (impl_->transparent_) {
    impl_->background_->SetPixelX(left);
    impl_->background_->SetPixelY(top);
    impl_->background_->SetPixelWidth(width - left - right);
    impl_->background_->SetPixelHeight(height - top - bottom);
  }

  if (impl_->transparent_ || impl_->IsChildViewHasResizeBorder()) {
    impl_->resize_border_->SetPixelX(left);
    impl_->resize_border_->SetPixelY(top);
    impl_->resize_border_->SetPixelWidth(width - left - right);
    impl_->resize_border_->SetPixelHeight(height - top - bottom);
  }

  impl_->UpdateDecoratorVisibility();
}

void FloatingMainViewDecorator::GetMargins(double *left, double *top,
                                           double *right, double *bottom) const {
  double *btn_edge = impl_->GetBackgroundMargins(left, top, right, bottom);
  double border_margin =
      (impl_->IsChildViewHasResizeBorder() ? 0 : kVDMainBorderWidth);

  *left += border_margin;
  *top += border_margin;
  *right += border_margin;
  *bottom += border_margin;

  if (!impl_->transparent_) {
    if (IsMinimized())
      *btn_edge = border_margin;
    else
      *btn_edge -= border_margin;
  }
}

void FloatingMainViewDecorator::OnAddDecoratorMenuItems(MenuInterface *menu) {
  int priority = MenuInterface::MENU_ITEM_PRI_DECORATOR;

  AddCollapseExpandMenuItem(menu);

  if (impl_->on_dock_signal_.HasActiveConnections()) {
    menu->AddItem(GM_("MENU_ITEM_DOCK_TO_SIDEBAR"), 0, 0,
                  NewSlot(impl_, &Impl::DockMenuCallback), priority);
  }

  if (!IsMinimized() && !IsPoppedOut()) {
    AddZoomMenuItem(menu);
  }

  MainViewDecoratorBase::OnAddDecoratorMenuItems(menu);
}

void FloatingMainViewDecorator::OnShowDecorator() {
  impl_->show_decorator_ = true;
  impl_->UpdateDecoratorVisibility();
  SetButtonBoxVisible(true);
  GetViewHost()->EnableInputShapeMask(false);
}

void FloatingMainViewDecorator::OnHideDecorator() {
  impl_->show_decorator_ = false;
  impl_->UpdateDecoratorVisibility();
  SetButtonBoxVisible(false);
  GetViewHost()->EnableInputShapeMask(true);
}

} // namespace ggadget
