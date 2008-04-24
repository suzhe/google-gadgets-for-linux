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
                    ViewHostInterface *real_viewhost)
        : owner_(owner),
          private_view_(NULL),
          view_element_(NULL),
          real_viewhost_(real_viewhost) {
    }
    virtual ~SideBarViewHost() {
      delete view_element_;
      DLOG("SideBarViewHost Dtor: %p", this);
    }
    virtual ViewHostInterface::Type GetType() const {
      return ViewHostInterface::VIEW_HOST_MAIN;
    }
    virtual void Destroy() { delete this; }
    virtual void SetView(ViewInterface *view) {
      if (view_element_) {
        if (!owner_->GetChildren()->RemoveElement(view_element_))
          delete view_element_;
        view_element_ = NULL;
        private_view_ = NULL;
      }
      if (!view) return;
      view_element_ =
          new ViewElement(NULL, owner_, down_cast<View *>(view));
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
      if (GetView()) {
        x += GetView()->GetWidth();
        y += GetView()->GetHeight();
      }
      real_viewhost_->ViewCoordToNativeWidgetCoord(x, y, widget_x, widget_y);
    }
    virtual void QueueDraw() {
      real_viewhost_->QueueDraw();
    }
    virtual void QueueResize() {
      real_viewhost_->QueueResize();
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
    virtual void Dock() {
      ASSERT(false);  // should not be called
    }
    virtual void Undock() {
      real_viewhost_->Undock();
    }
    virtual void Expand() {
      SimpleEvent e(Event::EVENT_POPOUT);
      private_view_->OnOtherEvent(e);
      real_viewhost_->Expand();
    }
    virtual void Unexpand() {
      SimpleEvent e(Event::EVENT_POPIN);
      private_view_->OnOtherEvent(e);
    }
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
    virtual ViewInterface::DebugMode GetDebugMode() const {
      return real_viewhost_->GetDebugMode();
    }

    SideBar::Impl *owner_;
    View *private_view_;
    ViewElement *view_element_;
    ViewHostInterface *real_viewhost_;
  };

  Impl(HostInterface *host, SideBar *owner, ViewHostInterface *view_host)
      : View(view_host, NULL, NULL, NULL),
        host_(host),
        owner_(owner),
        view_host_(view_host),
        null_element_(NULL),
        expand_element_(NULL),
        mouse_move_event_x_(-1),
        mouse_move_event_y_(-1),
        is_drag_event_(false),
        background_(NULL),
        icon_(NULL),
        main_div_(NULL),
        close_slot_(NULL) {
    ASSERT(host);
    SetResizable(ViewInterface::RESIZABLE_TRUE);
    EnableCanvasCache(false);
    SetupDecorator();
  }
  ~Impl() {
  }

 public:
  //FIXME: this method is too big
  virtual EventResult OnMouseEvent(const MouseEvent &event) {
    if (is_drag_event_ && event.GetType() == Event::EVENT_MOUSE_CLICK) {
      DLOG("ignore the click event after drag.");
      is_drag_event_ = false;
      return EVENT_RESULT_UNHANDLED;
    }
    // the mouse down event after expand event should file unexpand event
    if (event.GetType() == Event::EVENT_MOUSE_DOWN && expand_element_) {
      view_host_->Unexpand();
      return EVENT_RESULT_HANDLED;
    }
    EventResult result = View::OnMouseEvent(event);
    if (event.GetType() == Event::EVENT_MOUSE_DOWN &&
        mouse_move_event_x_ < 0 && mouse_move_event_y_ < 0) {
      DLOG("Mouse down at (%f,%f)", event.GetX(), event.GetY());
      mouse_move_event_x_ = event.GetX();
      mouse_move_event_y_ = event.GetY();
    } else if (event.GetType() == Event::EVENT_MOUSE_UP) {
      DLOG("Mouse up at (%f,%f)", event.GetX(), event.GetY());
      ResetState();
    }
    if (result != EVENT_RESULT_UNHANDLED ||
        event.GetButton() != MouseEvent::BUTTON_LEFT ||
        event.GetType() != Event::EVENT_MOUSE_MOVE ||
        !GetMouseOverElement() ||
        !GetMouseOverElement()->IsInstanceOf(ViewElement::CLASS_ID))
      return result;
    if (!GetPopupElement()) {
      if (GetHitTest() == ViewInterface::HT_BOTTOM) {
        double old_height = mouse_move_event_y_ -
                            GetMouseOverElement()->GetPixelY();
        double new_height = event.GetY() - GetMouseOverElement()->GetPixelY();
        double offset = std::abs(new_height - old_height);
        DLOG("old height: %.1lf, new: %.1lf", old_height, new_height);
        int index = GetIndex(GetMouseOverElement());
        if (new_height > old_height && DownResize(index + 1, &offset)) {
          mouse_move_event_y_ = event.GetY();
          ViewElement *element = down_cast<ViewElement *>(GetMouseOverElement());
          element->SetSize(element->GetPixelWidth(),
              element->GetPixelHeight() + offset);
          QueueDraw();
        }
        if (new_height < old_height && UpResize(index, &offset)) {
          mouse_move_event_y_ = event.GetY();
          Layout();
          QueueDraw();
        }
      } else {
        ViewElement *element = down_cast<ViewElement *>(GetMouseOverElement());
        mouse_move_event_x_ = event.GetX() - element->GetPixelX();
        mouse_move_event_y_ = event.GetY() - element->GetPixelY();
        DLOG("position in hanged_element: %fx%f",
            mouse_move_event_x_, mouse_move_event_y_);
        // Send fake mouse up event to the view element so that we
        // can start to drag the element. Note: no mouse click event
        // is sent in this case, to prevent unwanted action after
        // window move.
        MouseEvent e(Event::EVENT_MOUSE_UP,
            mouse_move_event_x_, mouse_move_event_y_, 0, 0,
            MouseEvent::BUTTON_LEFT, event.GetModifier());
        element->HandleMouseEvent(e);
        is_drag_event_ = true;
        element->SetOpacity(kOpacityFactor);
        SetPopupElement(element);
      }
    } else {
      // check if an undock event happens
      if (event.GetX() - mouse_move_event_x_ < -GetWidth() ||
          event.GetX() - mouse_move_event_x_ > GetWidth()) {
        is_drag_event_ = false;
        down_cast<ViewElement *>(GetMouseOverElement())->
          GetChildView()->GetViewHost()->Undock();
        ResetState();
        return EVENT_RESULT_HANDLED;
      } else {
        // check if need re-layout the order the gadgets
        BasicElement *previous, *next;
        GetInsertPoint(static_cast<int>(event.GetY()),
            GetPopupElement(), &previous, &next);
        if (previous != GetPopupElement() && next != GetPopupElement()) {
          DLOG("Insert %p previous %p next %p, event: %.1lfx%.1lf",
              GetPopupElement(), previous, next, event.GetX(), event.GetY());
          main_div_->GetChildren()->InsertElement(GetPopupElement(), next);
          Layout();
        }
        GetPopupElement()->SetPixelY(event.GetY() - mouse_move_event_y_);
        // GetPopupElement()->SetPixelX(event.GetX() - mouse_move_event_x_);
      }
      QueueDraw();
    }
    return EVENT_RESULT_HANDLED;
  }
  virtual void Draw(CanvasInterface *canvas) {
    View::Draw(canvas);
  }
  virtual bool OnAddContextMenuItems(MenuInterface *menu) {
    if (GetMouseOverElement() &&
        GetMouseOverElement()->IsInstanceOf(ViewElement::CLASS_ID)) {
      ViewElement *ele = down_cast<ViewElement *>(GetMouseOverElement());
      ele->GetChildView()->GetGadget()->OnAddCustomMenuItems(menu);
    }
    View::OnAddContextMenuItems(menu);
    return true;
  }
  virtual bool OnSizing(double *width, double *height) {
    return kSideBarMinWidth < *width && *width < kSideBarMaxWidth;
  }
  virtual void SetSize(double width, double height) {
    View::SetSize(width, height);

    background_->SetPixelWidth(width);
    background_->SetPixelHeight(height);
    background_->SetOpacity(kOpacityFactor);

    main_div_->SetPixelWidth(width - 2 * kBoderWidth);
    main_div_->SetPixelHeight(height - 2 * kBoderWidth - kIconHeight);

    button_array_[0]->SetPixelX(width - 3 * kIconHeight - 2 - kBoderWidth);
    button_array_[1]->SetPixelX(width - 2 * kIconHeight - 1 - kBoderWidth);
    button_array_[2]->SetPixelX(width - kIconHeight - kBoderWidth);

    border_array_[2]->SetPixelX(width - kBoderWidth);
    border_array_[1]->SetPixelY(height - kBoderWidth);

    border_array_[0]->SetPixelWidth(width - 2 * kBoderWidth);
    border_array_[1]->SetPixelWidth(width - 2 * kBoderWidth);
    border_array_[2]->SetPixelHeight(height - 2 * kBoderWidth);
    border_array_[3]->SetPixelHeight(height - 2 * kBoderWidth);
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
  }
  //TODO: refactor this method
  void SetupDecorator() {
    background_ = new DivElement(NULL, this, NULL);
    GetChildren()->InsertElement(background_, NULL);
    background_->SetBackgroundMode(DivElement::BACKGROUND_MODE_STRETCH_MIDDLE);
    background_->SetBackground(LoadGlobalImageAsVariant(kVDMainBackground));

    Variant border_h = LoadGlobalImageAsVariant(kVDBorderH);
    Variant border_v = LoadGlobalImageAsVariant(kVDBorderV);
    for (int i = 0; i < 4; ++i) {
      ImgElement *img = new ImgElement(NULL, this, NULL);
      border_array_[i] = img;
      GetChildren()->InsertElement(img, NULL);
    }
    border_array_[0]->SetSrc(border_h);
    border_array_[1]->SetSrc(border_h);
    border_array_[2]->SetSrc(border_v);
    border_array_[3]->SetSrc(border_v);

    border_array_[0]->SetPixelHeight(kBoderWidth);
    border_array_[1]->SetPixelHeight(kBoderWidth);
    border_array_[2]->SetPixelWidth(kBoderWidth);
    border_array_[3]->SetPixelWidth(kBoderWidth);

    border_array_[1]->SetFlip(ImgElement::FLIP_HORIZONTAL);
    border_array_[2]->SetFlip(ImgElement::FLIP_VERTICAL);

    border_array_[0]->SetPixelX(kBoderWidth);
    border_array_[1]->SetPixelX(kBoderWidth);
    border_array_[2]->SetPixelY(kBoderWidth);
    border_array_[3]->SetPixelY(kBoderWidth);

    border_array_[0]->SetHitTest(HT_TOP);
    border_array_[1]->SetHitTest(HT_BOTTOM);
    border_array_[2]->SetHitTest(HT_RIGHT);
    border_array_[3]->SetHitTest(HT_LEFT);

    // FIXME: choose proper cursor type~~
    for (int i = 0; i < 4; ++i)
      border_array_[i]->SetCursor(ViewInterface::CURSOR_SIZE);

    SetupButtons();

    main_div_ = new DivElement(NULL, this, NULL);
    GetChildren()->InsertElement(main_div_, NULL);
    main_div_->SetPixelX(kBoderWidth);
    main_div_->SetPixelY(kBoderWidth + kIconHeight);
  }
  void SetupButtons() {
    icon_ = new ImgElement(NULL, this, NULL);
    GetChildren()->InsertElement(icon_, NULL);
    icon_->SetSrc(LoadGlobalImageAsVariant(kSideBarIcon));
    icon_->SetPixelX(kBoderWidth);
    icon_->SetPixelY(kBoderWidth);

    button_array_[0] = new ButtonElement(NULL, this, NULL);
    button_array_[0]->SetImage(LoadGlobalImageAsVariant(kSBButtonAddUp));
    button_array_[0]->SetDownImage(LoadGlobalImageAsVariant(kSBButtonAddDown));
    button_array_[0]->SetOverImage(LoadGlobalImageAsVariant(kSBButtonAddOver));
    button_array_[0]->SetPixelY(kBoderWidth + (kIconHeight - kButtonWidth) / 2);
    GetChildren()->InsertElement(button_array_[0], NULL);

    button_array_[1] = new ButtonElement(NULL, this, NULL);
    button_array_[1]->SetImage(LoadGlobalImageAsVariant(kSBButtonConfigUp));
    button_array_[1]->SetDownImage(LoadGlobalImageAsVariant(kSBButtonConfigDown));
    button_array_[1]->SetOverImage(LoadGlobalImageAsVariant(kSBButtonConfigOver));
    button_array_[1]->SetPixelY(kBoderWidth + (kIconHeight - kButtonWidth) / 2);
    GetChildren()->InsertElement(button_array_[1], NULL);

    button_array_[2] = new ButtonElement(NULL, this, NULL);
    button_array_[2]->SetImage(LoadGlobalImageAsVariant(kSBButtonCloseUp));
    button_array_[2]->SetDownImage(LoadGlobalImageAsVariant(kSBButtonCloseDown));
    button_array_[2]->SetOverImage(LoadGlobalImageAsVariant(kSBButtonCloseOver));
    button_array_[2]->SetPixelY(kBoderWidth + (kIconHeight - kButtonWidth) / 2);
    GetChildren()->InsertElement(button_array_[2], NULL);
  }
  // TODO: refactor the duplicate method in decorated_view_host.cc
  Variant LoadGlobalImageAsVariant(const char *img) {
    std::string data;
    if (GetGlobalFileManager()->ReadFile(img, &data)) {
      ScriptableBinaryData *binary =
        new ScriptableBinaryData(data.c_str(), data.size());
      if (binary) {
        DLOG("Load image %s success.", img);
        return Variant(binary);
      }
    }
    LOG("Load image %s failed. Return NULL", img);
    return Variant();
  }
  int GetIndex(const BasicElement *element) const {
    ASSERT(element->IsInstanceOf(ViewElement::CLASS_ID));
    for (int i = 0; i < main_div_->GetChildren()->GetCount(); ++i)
      if (element == main_div_->GetChildren()->GetItemByIndex(i)) return i;
    return -1;
  }
  void GetInsertPoint(int y, BasicElement *insertee,
      BasicElement **previous, BasicElement **next) {
    BasicElement *e = NULL;
    if (next) *next = NULL;
    for (int i = 0; i < main_div_->GetChildren()->GetCount(); ++i) {
      if (previous) *previous = e;
      e = main_div_->GetChildren()->GetItemByIndex(i);
      if (insertee == e) continue;
      double middle = e->GetPixelY() + e->GetPixelHeight() / 2;
      if (y - main_div_->GetPixelY() < middle) {
        if (next) *next = e;
        return;
      }
    }
    if (previous) *previous = e;
  }
  void Layout() {
    DLOG("Layout in Sidebar, element number: %d", main_div_->GetChildren()->GetCount());
    double height = kSperator;
    for (int i = 0; i < main_div_->GetChildren()->GetCount(); ++i) {
      ViewElement *element =
        down_cast<ViewElement *>(main_div_->GetChildren()->GetItemByIndex(i));
      if (!element->IsEnabled() && element != expand_element_) continue;
      double x = GetWidth(), y = element->GetPixelHeight();
      if (element->IsVisible() && element->OnSizing(&x, &y)) {
        element->SetSize(x, y);
      }
      element->SetPixelX(0);
      element->SetPixelY(height);
      height += element->GetPixelHeight() + kSperator;
      DLOG("Element(%p,view:%p)'s pos: (%.1lf,%.1lf) size: %.1lfx%.1lf:",
           element, element->GetChildView(),
           element->GetPixelX(), element->GetPixelY(),
           element->GetPixelWidth(), element->GetPixelHeight());
    }
    QueueDraw();
  }
  ViewElement *FindViewElementByView(View *view) {
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
  void InsertNullElement(int y, View *view) {
    ASSERT(view);
    if (null_element_ && null_element_->GetChildView() != view) {
      // only one null element is allowed
      main_div_->GetChildren()->RemoveElement(null_element_);
    }
    BasicElement *pre, *next;
    if (!null_element_) {
      null_element_ = new ViewElement(main_div_, this, view);
      null_element_->SetPixelHeight(view->GetHeight());
      null_element_->SetVisible(false);
    }
    GetInsertPoint(y, null_element_, &pre, &next);
    if (pre != null_element_ && next != null_element_) {
      main_div_->GetChildren()->InsertElement(null_element_, next);
      Layout();
    }
  }
  void ClearNullElement() {
    if (null_element_) {
      main_div_->GetChildren()->RemoveElement(null_element_);
      null_element_ = NULL;
      Layout();
    }
  }
  bool Dock(int insert_point, View *view, bool force_insert) {
    ASSERT(!FindViewElementByView(view));
    // This is the behavior in GDWin, we may change it here
    if (!force_insert && GetBlankHeight() < view->GetHeight()) return false;
    ViewHostInterface *view_host =
        host_->NewViewHost(ViewHostInterface::VIEW_HOST_MAIN);
    ViewHostInterface *old = view->SwitchViewHost(view_host);
    if (old) old->Destroy();
    ViewElement *ele = new ViewElement(main_div_, this,
        down_cast<View *>(view_host->GetView()));
    ele->SetPixelHeight(ele->GetChildView()->GetHeight());
    BasicElement *element;
    GetInsertPoint(insert_point, ele, NULL, &element);
    main_div_->GetChildren()->InsertElement(ele, element);
    DLOG("Sidebar: Docked view(%p)", view);
    Layout();
    SimpleEvent e(Event::EVENT_DOCK);
    ele->HandleOtherEvent(e);
    return true;
  }
  bool Undock(View *view) {
    ViewElement *element = FindViewElementByView(view);
    DLOG("Sidebar: Undock view(%p) in element(%p)", view, element);
    if (!element) return false;
    SimpleEvent e(Event::EVENT_UNDOCK);
    element->HandleOtherEvent(e);
    if (!main_div_->GetChildren()->RemoveElement(element))
      delete element;
    Layout();
    return true;
  }
  void Expand(View *view) {
    if (expand_element_) expand_element_->SetEnabled(true);
    expand_element_ = FindViewElementByView(view);
    expand_element_->SetEnabled(false);
  }
  void Unexpand() {
    ASSERT(expand_element_);
    if (!expand_element_) return;
    expand_element_->GetChildView()->GetViewHost()->Unexpand();
    expand_element_->SetEnabled(true);
    expand_element_ = NULL;
  }
  bool UpResize(int index, double *offset) {
    double count = 0;
    while (*offset > count && index >= 0) {
      DLOG("index: %d, offset: %f", index, *offset);
      ViewElement *element = down_cast<ViewElement *>(
          main_div_->GetChildren()->GetItemByIndex(index));
      double w = element->GetPixelWidth();
      double h = element->GetPixelHeight() + count - *offset;
      if (element->OnSizing(&w, &h)) {
        double diff = std::min(element->GetPixelHeight() - h, *offset - count);
        DLOG("original: %.1lfx%.1lf, new: %.1lfx%.1lf, diff: %f",
             element->GetPixelWidth(), element->GetPixelHeight(), w, h, diff);
        element->SetPixelHeight(element->GetPixelHeight() - diff);
        count += diff;
      }
      index--;
    }
    if (count == 0) return false;
    *offset = count;
    return true;
  }
  bool DownResize(int index, double *offset) {
    double blank = GetBlankHeight();
    double count = 0;
    if (blank > 0) {
      count = std::min(blank, *offset);
      for (int i = index; i < main_div_->GetChildren()->GetCount(); ++i) {
        BasicElement *element = main_div_->GetChildren()->GetItemByIndex(i);
        element->SetPixelY(element->GetPixelY() + count);
      }
    }
    while (*offset > count && index < main_div_->GetChildren()->GetCount()) {
      ViewElement *element = down_cast<ViewElement *>(
          main_div_->GetChildren()->GetItemByIndex(index));
      double w = element->GetPixelWidth();
      double h = element->GetPixelHeight() + *offset - count;
      if (element->OnSizing(&w, &h) &&
          w == element->GetPixelWidth() && h < element->GetPixelHeight()) {
        double diff = std::min(element->GetPixelHeight() - h, *offset - count);
        element->SetPixelHeight(element->GetPixelHeight() - diff);
        element->SetPixelY(element->GetPixelY() + diff);
        count += diff;
      }
      index++;
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
  ViewElement *expand_element_;

  double mouse_move_event_x_;
  double mouse_move_event_y_;
  double is_drag_event_;

  std::vector<Connection *> connections_;

  // elements of sidebar decorator
  DivElement *background_;
  ImgElement *icon_;
  DivElement *main_div_;
  ButtonElement *button_array_[3];
  ImgElement *border_array_[4];

  Slot0<void> *close_slot_;

  static const int kSperator = 2;
  static const int kMouseMoveThreshold = 2;
  static const double kOpacityFactor = 0.618;
  static const double kSideBarMinWidth = 50;
  static const double kSideBarMaxWidth = 999;
  static const double kBoderWidth = 3;
  static const double kButtonWidth = 18;
  static const double kIconHeight = 22;
};

SideBar::SideBar(HostInterface *host, ViewHostInterface *view_host)
  : impl_(new Impl(host, this, view_host)) {
  impl_->SetSize(220, 640);
}

SideBar::~SideBar() {
  delete impl_;
  impl_ = NULL;
}

ViewHostInterface *SideBar::NewViewHost(ViewHostInterface::Type type) {
  ASSERT(type == ViewHostInterface::VIEW_HOST_MAIN);
  return new Impl::SideBarViewHost(impl_, type, impl_->view_host_);
}

ViewHostInterface *SideBar::GetViewHost() const {
  return impl_->GetViewHost();
}

void SideBar::InsertNullElement(int y, View *view) {
  return impl_->InsertNullElement(y, view);
}

void SideBar::ClearNullElement() {
  impl_->ClearNullElement();
}

bool SideBar::Dock(int insert_point, View *view, bool force_insert) {
  return impl_->Dock(insert_point, view, force_insert);
}

bool SideBar::Undock(View *view) {
  return impl_->Undock(view);
}

void SideBar::Expand(View *view) {
  impl_->Expand(view);
}

void SideBar::Unexpand(View *view) {
  impl_->Unexpand();
}

ViewElement *SideBar::GetMouseOverElement() const {
  BasicElement *element = impl_->GetMouseOverElement();
  if (element && element->IsInstanceOf(ViewElement::CLASS_ID))
    return down_cast<ViewElement *>(element);
  return NULL;
}

void SideBar::GetPointerPosition(double *x, double *y) const {
  if (impl_->mouse_move_event_x_ > 0 || impl_->mouse_move_event_y_ > 0) {
    *x = impl_->mouse_move_event_x_;
    *y = impl_->mouse_move_event_y_;
  }
}

void SideBar::SetAddGadgetSlot(Slot0<void> *slot) {
  impl_->connections_.push_back(
      impl_->button_array_[0]->ConnectOnClickEvent(slot));
}

void SideBar::SetMenuSlot(Slot0<void> *slot) {
  impl_->connections_.push_back(
      impl_->button_array_[1]->ConnectOnClickEvent(slot));
}

void SideBar::SetCloseSlot(Slot0<void> *slot) {
  impl_->connections_.push_back(
      impl_->button_array_[2]->ConnectOnClickEvent(slot));
}

}  // namespace ggadget
