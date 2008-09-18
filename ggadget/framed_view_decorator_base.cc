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

#include "framed_view_decorator_base.h"

#include <string>
#include <algorithm>
#include "logger.h"
#include "common.h"
#include "elements.h"
#include "gadget_consts.h"
#include "gadget.h"
#include "main_loop_interface.h"
#include "signals.h"
#include "slot.h"
#include "view.h"
#include "view_host_interface.h"
#include "button_element.h"
#include "div_element.h"
#include "img_element.h"
#include "label_element.h"
#include "text_frame.h"
#include "menu_interface.h"

namespace ggadget {
static const double kVDFramedBorderWidth = 6;
static const double kVDFramedCaptionMargin = 1;
static const double kVDFramedActionMargin = 1;

class FramedViewDecoratorBase::Impl {
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
    RESIZE_TOP = 0,
    RESIZE_LEFT,
    RESIZE_BOTTOM,
    RESIZE_RIGHT,
    RESIZE_TOP_LEFT,
    RESIZE_TOP_RIGHT,
    RESIZE_BOTTOM_LEFT,
    RESIZE_BOTTOM_RIGHT,
    NUMBER_OF_RESIZE_BORDERS
  };

  static const ResizeBorderInfo kResizeBordersInfo[];
 public:
  Impl(FramedViewDecoratorBase *owner)
    : owner_(owner),
      top_(new ImgElement(NULL, owner, NULL)),
      background_(new ImgElement(NULL, owner, NULL)),
      bottom_(new ImgElement(NULL, owner, NULL)),
      resize_border_(new DivElement(NULL, owner, NULL)),
      caption_(new LabelElement(NULL, owner, NULL)),
      close_button_(new ButtonElement(NULL, owner, NULL)),
      action_div_(new DivElement(NULL, owner, NULL)) {
    top_->SetSrc(Variant(kVDFramedTop));
    top_->SetStretchMiddle(true);
    top_->SetPixelX(0);
    top_->SetPixelY(0);
    top_->SetRelativeWidth(1);
    top_->SetVisible(true);
    owner->InsertDecoratorElement(top_, true);

    background_->SetSrc(Variant(kVDFramedBackground));
    background_->SetStretchMiddle(true);
    background_->SetPixelX(0);
    background_->SetPixelY(top_->GetSrcHeight());
    background_->SetRelativeWidth(1);
    background_->EnableCanvasCache(true);
    owner->InsertDecoratorElement(background_, true);

    bottom_->SetSrc(Variant(kVDFramedBottom));
    bottom_->SetStretchMiddle(true);
    bottom_->SetPixelX(0);
    bottom_->SetRelativeY(1);
    bottom_->SetRelativePinY(1);
    bottom_->SetRelativeWidth(1);
    bottom_->SetVisible(false);
    owner_->InsertDecoratorElement(bottom_, true);

    // Setup resize borders.
    for (size_t i = 0; i < NUMBER_OF_RESIZE_BORDERS; ++i) {
      BasicElement *elm =
          new BasicElement(resize_border_, owner, NULL, NULL, false);
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
    resize_border_->SetPixelX(0);
    resize_border_->SetPixelY(0);
    resize_border_->SetRelativeWidth(1);
    resize_border_->SetRelativeHeight(1);
    resize_border_->SetVisible(true);
    resize_border_->SetEnabled(true);
    owner->InsertDecoratorElement(resize_border_, true);

    caption_->GetTextFrame()->SetColor(Color::kBlack, 1);
    caption_->GetTextFrame()->SetWordWrap(false);
    caption_->GetTextFrame()->SetTrimming(
        CanvasInterface::TRIMMING_CHARACTER_ELLIPSIS);
    caption_->SetPixelX(kVDFramedBorderWidth + kVDFramedCaptionMargin);
    caption_->SetPixelY(kVDFramedBorderWidth + kVDFramedCaptionMargin);
    caption_->ConnectOnClickEvent(
        NewSlot(owner, &FramedViewDecoratorBase::OnCaptionClicked));
    caption_->SetEnabled(false);
    owner->InsertDecoratorElement(caption_, true);

    close_button_->SetPixelY(kVDFramedBorderWidth);
    close_button_->SetImage(Variant(kVDFramedCloseNormal));
    close_button_->SetOverImage(Variant(kVDFramedCloseOver));
    close_button_->SetDownImage(Variant(kVDFramedCloseDown));
    close_button_->ConnectOnClickEvent(
        NewSlot(owner, &FramedViewDecoratorBase::OnCloseButtonClicked));
    owner->InsertDecoratorElement(close_button_, true);
    close_button_->Layout();

    action_div_->SetVisible(false);
    action_div_->SetRelativePinX(1);
    action_div_->SetRelativePinY(1);
    owner->InsertDecoratorElement(action_div_, true);
  }

  void SetShowActionArea(bool show) {
    if (show) {
      bottom_->SetVisible(true);
      action_div_->SetVisible(true);
      background_->SetSrc(Variant(kVDFramedMiddle));
    } else {
      bottom_->SetVisible(false);
      action_div_->SetVisible(false);
      background_->SetSrc(Variant(kVDFramedBackground));
    }
    // Caller shall call UpdateViewSize();
  }

  void LayoutActionArea() {
    Elements *elements = action_div_->GetChildren();
    double width = 0;
    double height = 0;
    int count = elements->GetCount();
    for (int i = 0; i < count; ++i) {
      BasicElement *elm = elements->GetItemByIndex(i);
      elm->Layout();
      if (elm->IsVisible()) {
        elm->SetPixelY(0);
        elm->SetPixelX(width);
        width += (elm->GetPixelWidth() + kVDFramedActionMargin);
        height = std::max(height, elm->GetPixelHeight());
      }
    }
    action_div_->SetPixelWidth(width);
    action_div_->SetPixelHeight(height);
  }

  void DoLayout() {
    double width = owner_->GetWidth();
    double height = owner_->GetHeight();
    double caption_width, caption_height;
    double top_height;

    close_button_->SetPixelX(width - kVDFramedBorderWidth -
                             close_button_->GetPixelWidth());
    caption_width = close_button_->GetPixelX() - caption_->GetPixelX() -
        kVDFramedCaptionMargin;
    caption_->SetPixelWidth(caption_width);
    caption_->GetTextFrame()->GetExtents(caption_width, &caption_width,
                                         &caption_height);
    top_height = top_->GetSrcHeight();

    // Only allow displaying two lines of caption.
    if (caption_height > top_height - kVDFramedBorderWidth-
        kVDFramedCaptionMargin * 2) {
      double simple_caption_height;
      caption_->GetTextFrame()->GetSimpleExtents(&caption_width,
                                                 &simple_caption_height);
      caption_height = std::min(simple_caption_height * 2, caption_height);
      top_height = caption_height + kVDFramedBorderWidth +
          kVDFramedCaptionMargin * 2 + 1;
    }

    caption_->SetPixelHeight(caption_height);
    top_->SetPixelHeight(top_height);

    background_->SetPixelY(top_height);
    if (bottom_->IsVisible()) {
      bottom_->SetPixelHeight(action_div_->GetPixelHeight() +
                              kVDFramedBorderWidth + kVDFramedActionMargin * 2);
      background_->SetPixelHeight(height - top_height -
                                  bottom_->GetPixelHeight());
    } else {
      background_->SetPixelHeight(height - top_height);
    }

    if (action_div_->IsVisible()) {
      action_div_->SetPixelX(width - kVDFramedBorderWidth -
                             kVDFramedActionMargin);
      action_div_->SetPixelY(height - kVDFramedBorderWidth -
                             kVDFramedActionMargin);
    }

    resize_border_->SetVisible(
        owner_->GetChildViewResizable() == ViewInterface::RESIZABLE_TRUE);
  }

 public:
  FramedViewDecoratorBase *owner_;
  ImgElement *top_;
  ImgElement *background_;
  ImgElement *bottom_;
  DivElement *resize_border_;
  LabelElement *caption_;
  ButtonElement *close_button_;
  DivElement *action_div_;
};

const FramedViewDecoratorBase::Impl::ResizeBorderInfo
FramedViewDecoratorBase::Impl::kResizeBordersInfo[] = {
  { 0, 0, 0, 0, -1, kVDFramedBorderWidth,
    ViewInterface::CURSOR_SIZENS, ViewInterface::HT_TOP },
  { 0, 0, 0, 0, kVDFramedBorderWidth, -1,
    ViewInterface::CURSOR_SIZEWE, ViewInterface::HT_LEFT },
  { 0, 1, 0, 1, -1, kVDFramedBorderWidth,
    ViewInterface::CURSOR_SIZENS, ViewInterface::HT_BOTTOM },
  { 1, 0, 1, 0, kVDFramedBorderWidth, -1,
    ViewInterface::CURSOR_SIZEWE, ViewInterface::HT_RIGHT },
  { 0, 0, 0, 0, kVDFramedBorderWidth, kVDFramedBorderWidth,
    ViewInterface::CURSOR_SIZENWSE, ViewInterface::HT_TOPLEFT },
  { 1, 0, 1, 0, kVDFramedBorderWidth, kVDFramedBorderWidth,
    ViewInterface::CURSOR_SIZENESW, ViewInterface::HT_TOPRIGHT },
  { 0, 1, 0, 1, kVDFramedBorderWidth, kVDFramedBorderWidth,
    ViewInterface::CURSOR_SIZENESW, ViewInterface::HT_BOTTOMLEFT },
  { 1, 1, 1, 1, kVDFramedBorderWidth, kVDFramedBorderWidth,
    ViewInterface::CURSOR_SIZENWSE, ViewInterface::HT_BOTTOMRIGHT }
};

FramedViewDecoratorBase::FramedViewDecoratorBase(ViewHostInterface *host,
                                                 const char *option_prefix)
  : ViewDecoratorBase(host, option_prefix, false, false),
    impl_(new Impl(this)) {
  GetViewHost()->EnableInputShapeMask(false);
}

FramedViewDecoratorBase::~FramedViewDecoratorBase() {
  delete impl_;
  impl_ = NULL;
}

void FramedViewDecoratorBase::SetCaptionClickable(bool clicked) {
  if (clicked) {
    impl_->caption_->GetTextFrame()->SetColor(Color(0, 0, 1), 1);
    impl_->caption_->GetTextFrame()->SetUnderline(true);
    impl_->caption_->SetEnabled(true);
    impl_->caption_->SetCursor(CURSOR_HAND);
  } else {
    impl_->caption_->GetTextFrame()->SetColor(Color::kBlack, 1);
    impl_->caption_->GetTextFrame()->SetUnderline(false);
    impl_->caption_->SetEnabled(false);
    impl_->caption_->SetCursor(CURSOR_DEFAULT);
  }
}

bool FramedViewDecoratorBase::IsCaptionClickable() const {
  return impl_->caption_->IsEnabled();
}

void FramedViewDecoratorBase::SetCaptionWordWrap(bool wrap) {
  impl_->caption_->GetTextFrame()->SetWordWrap(wrap);
  DoLayout();
  UpdateViewSize();
}

bool FramedViewDecoratorBase::IsCaptionWordWrap() const {
  return impl_->caption_->GetTextFrame()->IsWordWrap();
}

void FramedViewDecoratorBase::AddActionElement(BasicElement *element) {
  ASSERT(element);
  if (!impl_->action_div_->IsVisible())
    impl_->SetShowActionArea(true);
  impl_->action_div_->GetChildren()->InsertElement(element, NULL);
  impl_->LayoutActionArea();
  DoLayout();
  UpdateViewSize();
}

void FramedViewDecoratorBase::RemoveActionElements() {
  if (impl_->action_div_->IsVisible())
    impl_->SetShowActionArea(false);
  impl_->action_div_->GetChildren()->RemoveAllElements();
  DoLayout();
  UpdateViewSize();
}

bool FramedViewDecoratorBase::OnAddContextMenuItems(MenuInterface *menu) {
  ViewDecoratorBase::OnAddContextMenuItems(menu);
  // Don't show system menu item.
  return false;
}

void FramedViewDecoratorBase::SetResizable(ResizableMode resizable) {
  ViewDecoratorBase::SetResizable(resizable);
  impl_->resize_border_->SetVisible(resizable == RESIZABLE_TRUE);
}

void FramedViewDecoratorBase::SetCaption(const char *caption) {
  impl_->caption_->GetTextFrame()->SetText(caption);
  ViewDecoratorBase::SetCaption(caption);
}

void FramedViewDecoratorBase::OnChildViewChanged() {
  View *child = GetChildView();
  if (child)
    impl_->caption_->GetTextFrame()->SetText(child->GetCaption());
}

void FramedViewDecoratorBase::DoLayout() {
  // Call through parent's DoLayout();
  ViewDecoratorBase::DoLayout();
  impl_->DoLayout();
}

void FramedViewDecoratorBase::GetMargins(double *top, double *left,
                                         double *bottom, double *right) const {
  *top = impl_->background_->GetPixelY();
  *bottom = impl_->bottom_->IsVisible() ? impl_->bottom_->GetPixelHeight() :
      kVDFramedBorderWidth;
  *left = kVDFramedBorderWidth;
  *right = kVDFramedBorderWidth;
}

void FramedViewDecoratorBase::GetMinimumClientExtents(double *width,
                                                      double *height) const {
  *height = 0;
  if (impl_->action_div_->IsVisible())
    *width = impl_->action_div_->GetPixelWidth() + kVDFramedActionMargin * 2;
  else
    *width = 0;
}

void FramedViewDecoratorBase::OnCaptionClicked() {
}

void FramedViewDecoratorBase::OnCloseButtonClicked() {
  PostCloseSignal();
}

} // namespace ggadget
