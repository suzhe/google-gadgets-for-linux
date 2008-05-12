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

#include <vector>
#include <list>

#include "sidebar.h"

#include "button_element.h"
#include "canvas_interface.h"
#include "div_element.h"
#include "element_factory.h"
#include "elements.h"
#include "gadget_consts.h"
#include "gadget.h"
#include "file_manager_factory.h"
#include "host_interface.h"
#include "img_element.h"
#include "math_utils.h"
#include "scriptable_binary_data.h"
#include "view_element.h"
#include "view.h"

namespace ggadget {

class SideBar::Impl : public View {
 public:
  class SideBarViewHost : public ViewHostInterface {
   public:
    SideBarViewHost(SideBar::Impl *owner,
                    ViewHostInterface::Type type,
                    ViewHostInterface *real_viewhost,
                    double height)
        : owner_(owner),
          private_view_(NULL),
          view_element_(NULL),
          real_viewhost_(real_viewhost),
          height_(height) {
    }
    virtual ~SideBarViewHost() {
      if (view_element_ && !owner_->RemoveViewElement(view_element_)) {
        delete view_element_;
      }
      view_element_ = NULL;
      DLOG("SideBarViewHost Dtor: %p", this);
    }
    virtual ViewHostInterface::Type GetType() const {
      return ViewHostInterface::VIEW_HOST_MAIN;
    }
    virtual void Destroy() { delete this; }
    virtual void SetView(ViewInterface *view) {
      if (view_element_) {
        if (!owner_->RemoveViewElement(view_element_))
          delete view_element_;
        view_element_ = NULL;
        private_view_ = NULL;
      }
      if (!view) return;
      view_element_ =
          new ViewElement(owner_->main_div_, owner_, down_cast<View *>(view),
                          true);
      // insert the view element in proper place
      owner_->InsertViewElement(height_, view_element_);
      private_view_ = view_element_->GetChildView();
      QueueDraw();
    }
    virtual ViewInterface *GetView() const {
      return private_view_;
    }
    virtual GraphicsInterface *NewGraphics() const {
      return real_viewhost_->NewGraphics();
    }
    virtual void *GetNativeWidget() const {
      return owner_->GetNativeWidget();
    }
    virtual void ViewCoordToNativeWidgetCoord(double x, double y,
                                              double *widget_x,
                                              double *widget_y) const {
      double vx = x, vy = y;
      if (view_element_)
        view_element_->SelfCoordToViewCoord(x, y, &vx, &vy);
      if (real_viewhost_)
        real_viewhost_->ViewCoordToNativeWidgetCoord(vx, vy, widget_x, widget_y);
    }
    virtual void NativeWidgetCoordToViewCoord(
        double x, double y, double *view_x, double *view_y) const {
      double vx = x, vy = y;
      if (real_viewhost_)
        real_viewhost_->NativeWidgetCoordToViewCoord(x, y, &vx, &vy);
      if (view_element_)
        view_element_->ViewCoordToSelfCoord(vx, vy, view_x, view_y);
    }
    virtual void QueueDraw() {
      if (view_element_)
        view_element_->QueueDraw();
    }
    virtual void QueueResize() {
      resize_event_();
    }
    virtual void EnableInputShapeMask(bool /* enable */) {
      // Do nothing.
    }
    virtual void SetResizable(ViewInterface::ResizableMode mode) {}
    virtual void SetCaption(const char *caption) {}
    virtual void SetShowCaptionAlways(bool always) {}
    virtual void SetCursor(int type) {
      real_viewhost_->SetCursor(type);
    }
    virtual void SetTooltip(const char *tooltip) {
      real_viewhost_->SetTooltip(tooltip);
    }
    virtual bool ShowView(bool modal, int flags,
                          Slot1<void, int> *feedback_handler) {
      if (view_element_) {
        view_element_->SetEnabled(true);
        QueueDraw();
      }
      if (feedback_handler) {
        (*feedback_handler)(flags);
        delete feedback_handler;
      }
      return true;
    }
    virtual void CloseView() {
      if (view_element_) {
        view_element_->SetEnabled(false);
        QueueDraw();
      }
    }
    virtual bool ShowContextMenu(int button) {
      DLOG("Sidebar viewhost's ShowContextMenu");
      return real_viewhost_->ShowContextMenu(button);
    }
    virtual void BeginResizeDrag(int button, ViewInterface::HitTest hittest) {}
    virtual void BeginMoveDrag(int button) {}
    virtual void Alert(const char *message) {
      real_viewhost_->Alert(message);
    }
    virtual bool Confirm(const char *message) {
      return real_viewhost_->Confirm(message);
    }
    virtual std::string Prompt(const char *message,
                               const char *default_value) {
      return real_viewhost_->Prompt(message, default_value);
    }
    virtual int GetDebugMode() const {
      return real_viewhost_->GetDebugMode();
    }
    Connection *ConnectOnResize(Slot0<void> *slot) {
      return resize_event_.Connect(slot);
    }
   private:
    SideBar::Impl *owner_;
    View *private_view_;
    ViewElement *view_element_;
    ViewHostInterface *real_viewhost_;
    EventSignal resize_event_;
    double height_;
  };

  Impl(HostInterface *host, SideBar *owner, ViewHostInterface *view_host)
      : View(view_host, NULL, NULL, NULL),
        host_(host),
        owner_(owner),
        view_host_(view_host),
        null_element_(NULL),
        popout_element_(NULL),
        blank_height_(0),
        mouse_move_event_x_(-1),
        mouse_move_event_y_(-1),
        hit_element_bottom_(false),
        hit_element_normal_part_(false),
        hit_sidebar_border_(false),
        background_(NULL),
        icon_(NULL),
        main_div_(NULL) {
    ASSERT(host);
    SetResizable(ViewInterface::RESIZABLE_TRUE);
    //EnableCanvasCache(false);
    SetupDecorator();
  }
  ~Impl() {
  }

 public:
  virtual EventResult OnMouseEvent(const MouseEvent &event) {
    // the mouse down event after expand event should file unexpand event
    if (ShouldFirePopInEvent(event)) {
      popin_event_();
      return EVENT_RESULT_HANDLED;
    }

    EventResult result = EVENT_RESULT_UNHANDLED;
    // don't sent mouse event to view elements when layouting or resizing
    if (!hit_element_bottom_ && !hit_sidebar_border_)
      result = View::OnMouseEvent(event);

    if (result != EVENT_RESULT_UNHANDLED ||
        event.GetButton() != MouseEvent::BUTTON_LEFT)
      return result;

    int index = 0;
    double x, y;
    BasicElement *element = NULL;
    ViewElement *focused = owner_->GetMouseOverElement();
    switch (event.GetType()) {
      case Event::EVENT_MOUSE_DOWN:
        if (GetHitTest() == HT_LEFT || GetHitTest() == HT_RIGHT) {
          hit_sidebar_border_ = true;
          return EVENT_RESULT_UNHANDLED;
        }
        if (!focused) return result;

        DLOG("Mouse down at (%f,%f)", event.GetX(), event.GetY());
        mouse_move_event_x_ = event.GetX();
        mouse_move_event_y_ = event.GetY();
        focused->ViewCoordToSelfCoord(event.GetX(), event.GetY(), &x, &y);
        switch (focused->GetHitTest(x, y)) {
          case HT_BOTTOM:
            hit_element_bottom_ = true;
            // record the original height of each view elements
            for (; index < main_div_->GetChildren()->GetCount(); ++index) {
              element = main_div_->GetChildren()->GetItemByIndex(index);
              elements_height_.push_back(element->GetPixelHeight());
            }
            if (element) {
              blank_height_ = main_div_->GetPixelHeight() -
                  element->GetPixelY() - element->GetPixelHeight();
            }
            break;
          case HT_CLIENT:
            if (focused != popout_element_)
              hit_element_normal_part_ = true;
            break;
          default:
            break;
        }
        return result;
        break;
      case Event::EVENT_MOUSE_UP:
        DLOG("Mouse up at (%f,%f)", event.GetX(), event.GetY());
        ResetState();
        return result;
        break;
      case Event::EVENT_MOUSE_MOVE:  // handle it soon
        if (!focused)
          return result;
        break;
      default:
        return result;
    }

    double offset = mouse_move_event_y_ - event.GetY();
    if (hit_element_bottom_) {
      // set cursor so that user understand that we are still in layout process
      SetCursor(CURSOR_SIZENS);
      int index = GetIndex(focused);
      if (offset < 0) {
        DownResize(false, index + 1, &offset);
        UpResize(true, index, &offset);
        DownResize(true, index + 1, &offset);
        QueueDraw();
      } else {
        UpResize(true, index, &offset);
        Layout();
      }
    } else if (hit_element_normal_part_ &&
               (std::abs(offset) > kMouseMoveThreshold ||
                std::abs(event.GetX() - mouse_move_event_x_) >
                kMouseMoveThreshold)) {
      undock_event_();
      ResetState();
    } else if (hit_sidebar_border_) {
      return EVENT_RESULT_UNHANDLED;
    }
    return EVENT_RESULT_HANDLED;
  }

  virtual bool OnAddContextMenuItems(MenuInterface *menu) {
    if (GetMouseOverElement() &&
        GetMouseOverElement()->IsInstanceOf(ViewElement::CLASS_ID)) {
      GetMouseOverElement()->OnAddContextMenuItems(menu);
    } else {
      return system_menu_event_(menu);
    }
    return true;
  }
  virtual bool OnSizing(double *width, double *height) {
    return kSideBarMinWidth < *width && *width < kSideBarMaxWidth;
  }
  virtual void SetSize(double width, double height) {
    View::SetSize(width, height);

    main_div_->SetPixelWidth(width - 2 * kBoderWidth);
    main_div_->SetPixelHeight(height - 2 * kBoderWidth - kIconHeight);

    button_array_[0]->SetPixelX(width - 3 * kIconHeight - 2 - kBoderWidth);
    button_array_[1]->SetPixelX(width - 2 * kIconHeight - 1 - kBoderWidth);
    button_array_[2]->SetPixelX(width - kIconHeight - kBoderWidth);

    border_array_[0]->SetPixelHeight(height);
    border_array_[1]->SetPixelHeight(height);
    border_array_[1]->SetPixelX(width - kBoderWidth);

    Layout();
  }
  void ResetState() {
    if (GetPopupElement()) {
      GetPopupElement()->SetOpacity(1);
      SetPopupElement(NULL);
      Layout();
    }
    mouse_move_event_x_ = -1;
    mouse_move_event_y_ = -1;
    hit_element_bottom_ = false;
    hit_element_normal_part_ = false;
    hit_sidebar_border_ = false;
    blank_height_ = 0;
    elements_height_.clear();
  }
  void SetupDecorator() {
    background_ = new ImgElement(NULL, this, NULL);
    background_->SetSrc(Variant(kVDMainBackground));
    background_->SetStretchMiddle(true);
    background_->SetOpacity(kOpacityFactor);
    background_->SetPixelX(0);
    background_->SetPixelY(0);
    background_->SetRelativeWidth(1);
    background_->SetRelativeHeight(1);
    background_->EnableCanvasCache(true);
    GetChildren()->InsertElement(background_, NULL);

    for (int i = 0; i < 2; ++i) {
      DivElement *div = new DivElement(NULL, this, NULL);
      div->SetPixelWidth(kBoderWidth);
      div->SetPixelY(0);
      div->SetCursor(CURSOR_SIZEWE);
      border_array_[i] = div;
      GetChildren()->InsertElement(div, NULL);
    }
    border_array_[0]->SetHitTest(HT_LEFT);
    border_array_[1]->SetHitTest(HT_RIGHT);

    SetupButtons();

    main_div_ = new DivElement(NULL, this, NULL);
    GetChildren()->InsertElement(main_div_, NULL);
    main_div_->SetPixelX(kBoderWidth);
    main_div_->SetPixelY(kBoderWidth + kIconHeight);
  }
  void SetupButtons() {
    icon_ = new ImgElement(NULL, this, NULL);
    GetChildren()->InsertElement(icon_, NULL);
    icon_->SetSrc(Variant(kSideBarIcon));
    icon_->EnableCanvasCache(true);
    icon_->SetPixelX(kBoderWidth);
    icon_->SetPixelY(kBoderWidth);

    button_array_[0] = new ButtonElement(NULL, this, NULL);
    button_array_[0]->SetImage(Variant(kSBButtonAddUp));
    button_array_[0]->SetDownImage(Variant(kSBButtonAddDown));
    button_array_[0]->SetOverImage(Variant(kSBButtonAddOver));
    button_array_[0]->SetPixelY(kBoderWidth + (kIconHeight - kButtonWidth) / 2);
    GetChildren()->InsertElement(button_array_[0], NULL);

    button_array_[1] = new ButtonElement(NULL, this, NULL);
    button_array_[1]->SetImage(Variant(kSBButtonConfigUp));
    button_array_[1]->SetDownImage(Variant(kSBButtonConfigDown));
    button_array_[1]->SetOverImage(Variant(kSBButtonConfigOver));
    button_array_[1]->SetPixelY(kBoderWidth + (kIconHeight - kButtonWidth) / 2);
    button_array_[1]->ConnectOnClickEvent(NewSlot(
        this, &Impl::HandleConfigureButtonClick));
    GetChildren()->InsertElement(button_array_[1], NULL);

    button_array_[2] = new ButtonElement(NULL, this, NULL);
    button_array_[2]->SetImage(Variant(kSBButtonCloseUp));
    button_array_[2]->SetDownImage(Variant(kSBButtonCloseDown));
    button_array_[2]->SetOverImage(Variant(kSBButtonCloseOver));
    button_array_[2]->SetPixelY(kBoderWidth + (kIconHeight - kButtonWidth) / 2);
    GetChildren()->InsertElement(button_array_[2], NULL);
  }
  bool ShouldFirePopInEvent(const MouseEvent &event) {
    if (!popout_element_ ||
        event.GetType() != MouseEvent::EVENT_MOUSE_DOWN ||
        GetMouseOverElement() == popout_element_)
      return false;
    return true;
  }
  void HandleConfigureButtonClick() {
    view_host_->ShowContextMenu(MouseEvent::BUTTON_LEFT);
  }
  int GetIndex(const BasicElement *element) const {
    ASSERT(element->IsInstanceOf(ViewElement::CLASS_ID));
    for (int i = 0; i < main_div_->GetChildren()->GetCount(); ++i)
      if (element == main_div_->GetChildren()->GetItemByIndex(i)) return i;
    return -1;
  }
  // Get the insert point. The height is in the sidebar coordinates system.
  void GetInsertPoint(double height, BasicElement *insertee,
      BasicElement **previous, BasicElement **next) {
    BasicElement *e = NULL;
    if (next) *next = NULL;
    for (int i = 0; i < main_div_->GetChildren()->GetCount(); ++i) {
      if (previous) *previous = e;
      e = main_div_->GetChildren()->GetItemByIndex(i);
      if (insertee == e) continue;
      double middle = e->GetPixelY() + e->GetPixelHeight() / 2;
      if (height - main_div_->GetPixelY() < middle) {
        if (next) *next = e;
        return;
      }
    }
    if (previous) *previous = e;
  }
  void InsertViewElement(double height, BasicElement *element) {
    ASSERT(element && element->IsInstanceOf(ViewElement::CLASS_ID));
    BasicElement *pre, *next;
    GetInsertPoint(height, element, &pre, &next);
    if (pre != element && next != element) {
      main_div_->GetChildren()->InsertElement(element, next);
      Layout();
    }
  }
  bool RemoveViewElement(BasicElement *element) {
    ASSERT(element && element->IsInstanceOf(ViewElement::CLASS_ID));
    bool r = main_div_->GetChildren()->RemoveElement(element);
    if (r) Layout();
    return r;
  }
  void Layout() {
    double height = kSperator;
    for (int i = 0; i < main_div_->GetChildren()->GetCount(); ++i) {
      ViewElement *element =
        down_cast<ViewElement *>(main_div_->GetChildren()->GetItemByIndex(i));
      double x = main_div_->GetPixelWidth(), y = element->GetPixelHeight();
      if (element->IsVisible() && element->OnSizing(&x, &y)) {
        element->SetSize(x, y);
      }
      element->SetPixelX(0);
      element->SetPixelY(ceil(height));
      height += element->GetPixelHeight() + kSperator;
      double sx, sy;
      element->SelfCoordToViewCoord(0, 0, &sx, &sy);
    }
    QueueDraw();
  }
  ViewElement *FindViewElementByView(ViewInterface *view) const {
    for (int i = 0; i < main_div_->GetChildren()->GetCount(); ++i) {
      ViewElement *element = down_cast<ViewElement *>(
          main_div_->GetChildren()->GetItemByIndex(i));
      // they may be not exactly the same view, but they should be owned by
      // the same gadget...
      if (element->GetChildView()->GetGadget() == view->GetGadget())
        return element;
    }
    return NULL;
  }
  void InsertNullElement(double height, ViewInterface *view) {
    ASSERT(view);
    // only one null element is allowed
    if (null_element_ && null_element_->GetChildView() != view) {
      if (!main_div_->GetChildren()->RemoveElement(null_element_))
        delete null_element_;
      null_element_ = NULL;
    }
    if (!null_element_) {
      null_element_ = new ViewElement(main_div_, this, down_cast<View *>(view),
                                      true);
      null_element_->SetPixelHeight(view->GetHeight());
      null_element_->SetVisible(false);
    }
    InsertViewElement(height, null_element_);
  }
  void ClearNullElement() {
    if (null_element_) {
      main_div_->GetChildren()->RemoveElement(null_element_);
      null_element_ = NULL;
      Layout();
    }
  }
  // *offset could be any value
  bool UpResize(bool do_resize, int index, double *offset) {
    double sign = *offset > 0 ? 1 : -1;
    double count = 0;
    while (*offset * sign > count * sign && index >= 0) {
      ViewElement *element = down_cast<ViewElement *>(
          main_div_->GetChildren()->GetItemByIndex(index));
      double w = element->GetPixelWidth();
      double h = elements_height_[index] + count - *offset;
      // don't send non-positive resize request
      if (h <= .0) h = 1;
      if (element->OnSizing(&w, &h)) {
        double diff = std::min(sign * (elements_height_[index] - h),
                               sign * (*offset - count)) * sign;
        if (do_resize)
          element->SetSize(w, elements_height_[index] - diff);
        count += diff;
      } else {
        double oh = element->GetPixelHeight();
        double diff = std::min(sign * (elements_height_[index] - oh),
                               sign * (*offset - count)) * sign;
        if (diff > 0) count += diff;
      }
      index--;
    }
    if (do_resize)
      // recover upmost elemnts' size
      while (index >= 0) {
        ViewElement *element = down_cast<ViewElement *>(
            main_div_->GetChildren()->GetItemByIndex(index));
        element->SetSize(main_div_->GetPixelWidth(), elements_height_[index]);
        index--;
      }
    DLOG("original: at last off: %.1lf, count: %.1lf", *offset, count);
    if (count == 0) return false;
    *offset = count;
    return true;
  }
  bool DownResize(bool do_resize, int index, double *offset) {
    double count = 0;
    if (blank_height_ > 0) count = std::max(-blank_height_, *offset);
    while (*offset < count && index < main_div_->GetChildren()->GetCount()) {
      ViewElement *element = down_cast<ViewElement *>(
          main_div_->GetChildren()->GetItemByIndex(index));
      double w = element->GetPixelWidth();
      double h = elements_height_[index] + *offset - count;
      // don't send non-positive resize request
      if (h <= .0) h = 1;
      if (element->OnSizing(&w, &h) && h < elements_height_[index]) {
        double diff = std::min(elements_height_[index] - h, count - *offset);
        if (do_resize) {
          element->SetSize(w, elements_height_[index] - diff);
        }
        count -= diff;
      } else {
        double oh = element->GetPixelHeight();
        double diff = std::min(elements_height_[index] - oh, count - *offset);
        if (diff > 0) count -= diff;
      }
      index++;
    }
    if (do_resize) {
      // recover upmost elemnts' size
      while (index < main_div_->GetChildren()->GetCount()) {
        ViewElement *element = down_cast<ViewElement *>(
            main_div_->GetChildren()->GetItemByIndex(index));
        element->SetSize(main_div_->GetPixelWidth(), elements_height_[index]);
        index++;
      }
      Layout();
    }
    if (count == 0) return false;
    *offset = count;
    return true;
  }
  inline double GetBlankHeight() const {
    int index = main_div_->GetChildren()->GetCount();
    if (!index) return GetHeight();
    BasicElement *element = main_div_->GetChildren()->GetItemByIndex(index - 1);
    return GetHeight() - element->GetPixelY() - element->GetPixelHeight();
  }

 public:
  HostInterface *host_;
  SideBar *owner_;
  ViewHostInterface *view_host_;

  ViewElement *null_element_;
  ViewElement *popout_element_;

  std::vector<double> elements_height_;
  double blank_height_;
  double mouse_move_event_x_;
  double mouse_move_event_y_;
  bool hit_element_bottom_;
  bool hit_element_normal_part_;
  bool hit_sidebar_border_;

  // elements of sidebar decorator
  ImgElement *background_;
  ImgElement *icon_;
  DivElement *main_div_;
  ButtonElement *button_array_[3];
  DivElement *border_array_[2];

  Signal1<bool, MenuInterface *> system_menu_event_;
  EventSignal size_event_;
  EventSignal undock_event_;
  EventSignal popin_event_;

  static const int kSperator = 5;
  static const int kMouseMoveThreshold = 2;
  static const double kOpacityFactor = 0.618;
  static const double kSideBarMinWidth = 50;
  static const double kSideBarMaxWidth = 999;
  static const double kBoderWidth = 3;
  static const double kButtonWidth = 18;
  static const double kIconHeight = 22;
};

const int SideBar::Impl::kSperator;
const int SideBar::Impl::kMouseMoveThreshold;
const double SideBar::Impl::kOpacityFactor;
const double SideBar::Impl::kSideBarMinWidth;
const double SideBar::Impl::kSideBarMaxWidth;
const double SideBar::Impl::kBoderWidth;
const double SideBar::Impl::kButtonWidth;
const double SideBar::Impl::kIconHeight;

SideBar::SideBar(HostInterface *host, ViewHostInterface *view_host)
  : impl_(new Impl(host, this, view_host)) {
}

SideBar::~SideBar() {
  delete impl_;
  impl_ = NULL;
}

ViewHostInterface *SideBar::NewViewHost(double height) {
  Impl::SideBarViewHost *vh =
      new Impl::SideBarViewHost(impl_, ViewHostInterface::VIEW_HOST_MAIN,
                                impl_->view_host_, height);
  vh->ConnectOnResize(NewSlot(impl_, &Impl::Layout));
  return vh;
}

ViewHostInterface *SideBar::GetViewHost() const {
  return impl_->GetViewHost();
}

void SideBar::SetSize(double width, double height) {
  impl_->SetSize(width, height);
}

double SideBar::GetWidth() const {
  return impl_->GetWidth();
}

double SideBar::GetHeight() const {
  return impl_->GetHeight();
}

void SideBar::InsertNullElement(double height, ViewInterface *view) {
  return impl_->InsertNullElement(height, view);
}

void SideBar::ClearNullElement() {
  impl_->ClearNullElement();
}

void SideBar::Layout() {
  impl_->Layout();
}

ViewElement *SideBar::GetMouseOverElement() const {
  BasicElement *element = impl_->GetMouseOverElement();
  if (element && element->IsInstanceOf(ViewElement::CLASS_ID))
    return down_cast<ViewElement *>(element);
  return NULL;
}

ViewElement *SideBar::FindViewElementByView(ViewInterface *view) const {
  return impl_->FindViewElementByView(view);
}

ViewElement *SideBar::SetPopoutedView(ViewInterface *view) {
  if (view)
    impl_->popout_element_ = impl_->FindViewElementByView(view);
  else
    impl_->popout_element_ = NULL;
  return impl_->popout_element_;
}

Connection *SideBar::ConnectOnUndock(Slot0<void> *slot) {
  return impl_->undock_event_.Connect(slot);
}

Connection *SideBar::ConnectOnPopIn(Slot0<void> *slot) {
  return impl_->popin_event_.Connect(slot);
}

Connection *SideBar::ConnectOnAddGadget(Slot0<void> *slot) {
  return impl_->button_array_[0]->ConnectOnClickEvent(slot);
}

Connection *SideBar::ConnectOnMenuOpen(Slot1<bool, MenuInterface *> *slot) {
  return impl_->system_menu_event_.Connect(slot);
}

Connection *SideBar::ConnectOnClose(Slot0<void> *slot) {
  return impl_->button_array_[2]->ConnectOnClickEvent(slot);
}

Connection *SideBar::ConnectOnSizeEvent(Slot0<void> *slot) {
  return impl_->ConnectOnSizeEvent(slot);
}

}  // namespace ggadget
