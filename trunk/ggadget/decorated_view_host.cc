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
#include "decorated_view_host.h"
#include "button_element.h"
#include "div_element.h"
#include "elements.h"
#include "event.h"
#include "file_manager_factory.h"
#include "gadget_consts.h"
#include "gadget.h"
#include "graphics_interface.h"
#include "host_interface.h"
#include "image_interface.h"
#include "img_element.h"
#include "main_loop_interface.h"
#include "messages.h"
#include "scriptable_binary_data.h"
#include "signals.h"
#include "slot.h"
#include "view.h"
#include "view_element.h"
#include "copy_element.h"
#include "menu_interface.h"
#include "label_element.h"
#include "text_frame.h"
#include "messages.h"

namespace ggadget {

static const double kVDMainBorderWidth = 6;
static const double kVDMainToolbarHeight = 19;
static const double kVDMainButtonWidth = 19;
static const double kVDMainCornerSize = 16;
static const double kVDMainMinimizedHeight = 26;
static const double kVDMainIconHeight = 32;
static const double kVDMainIconWidth = 32;
static const double kVDMainIconMarginH = 4;
static const double kVDMainCaptionMarginV = 2;
static const double kVDMainCaptionMarginH = 4;
static const double kVDExpandedBorderWidth = 6;
static const double kVDDetailsBorderWidth = 6;
static const double kVDDetailsButtonHeight = 22;
static const double kVDDetailsButtonMargin = 2;

static const unsigned int kVDShowTimeout = 200;
static const unsigned int kVDHideTimeout = 500;

class DecoratedViewHost::Impl {
 public:
  // Base class for all kinds of view decorators.
  class ViewDecoratorBase : public View {
   public:
    ViewDecoratorBase(ViewHostInterface *host,
                      bool allow_x_margin,
                      bool allow_y_margin)
      : View(host, NULL, NULL, NULL),
        allow_x_margin_(allow_x_margin),
        allow_y_margin_(allow_y_margin),
        child_view_(NULL),
        view_element_(new ViewElement(NULL, this, NULL)) {
      view_element_->SetVisible(true);
      GetChildren()->InsertElement(view_element_, NULL);
      view_element_->ConnectOnSizeEvent(
          NewSlot(this, &ViewDecoratorBase::UpdateViewSize));
      // Always resizable.
      View::SetResizable(RESIZABLE_TRUE);
      //View::EnableCanvasCache(false);
    }
    virtual ~ViewDecoratorBase() {}

    ViewElement *GetViewElement() {
      return view_element_;
    }

    void SetChildView(View *child_view) {
      if (child_view_ != child_view) {
        child_view_ = child_view;
        view_element_->SetChildView(child_view);
        ChildViewChanged();
        // UpdateViewSize() will be triggered by ViewElement's OnSizeEvent.
      }
    }

    View *GetChildView() const {
      return child_view_;
    }

    void SetChildViewVisible(bool visible) {
      if (IsChildViewVisible() != visible) {
        view_element_->SetVisible(visible);
        // UpdateViewSize() should be called explicitly by derived classes.
      }
    }

    bool IsChildViewVisible() const {
      return view_element_->IsVisible();
    }

    void SetChildViewScale(double scale) {
      view_element_->SetScale(scale);
      // UpdateViewSize() will be triggered by ViewElement's OnSizeEvent.
    }

    void SetAllowXMargin(bool allow) {
      if (allow_x_margin_ != allow) {
        allow_x_margin_ = allow;
        UpdateViewSize();
      }
    }

    void SetAllowYMargin(bool allow) {
      if (allow_y_margin_ != allow) {
        allow_y_margin_ = allow;
        UpdateViewSize();
      }
    }

    void UpdateViewSize() {
      DLOG("DecoratedView::UpdateViewSize()");
      double left, right, top, bottom;
      GetMargins(&left, &right, &top, &bottom);
      double width = GetWidth();
      double height = GetHeight();
      double cw = width - left - right;
      double ch = height - top - bottom;
      GetClientExtents(&cw, &ch);

      cw += (left + right);
      ch += (top + bottom);

      if (SetViewSize(GetWidth(), GetHeight(), cw, ch))
        Layout();
    }

    void Layout() {
      if (IsChildViewVisible()) {
        double left, right, top, bottom;
        GetMargins(&left, &right, &top, &bottom);
        double vw = view_element_->GetPixelWidth();
        double vh = view_element_->GetPixelHeight();
        DLOG("DecoratedView::Layout() ChildSize(%lf, %lf)", vw, vh);
        double cw = GetWidth() - left - right;
        double ch = GetHeight() - top - bottom;
        DLOG("DecoratedView::Layout() ClientSize(%lf, %lf)", cw, ch);
        cw = left + (cw - vw) / 2.0;
        ch = top + (ch - vh) / 2.0;
        DLOG("Layout DecoratedView: MoveChildTo(%lf, %lf)", cw, ch);
        view_element_->SetPixelX(cw);
        view_element_->SetPixelY(ch);
      }
      DoLayout();
    }

    class SignalPostCallback : public WatchCallbackInterface {
     public:
      SignalPostCallback(const Signal0<void> *signal) : signal_(signal) {}
      virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
        (*signal_)();
        return false;
      }
      virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
        delete this;
      }
      const Signal0<void> *signal_;
    };

    // Helper function to post a signal to main loop.
    void PostSignal(const Signal0<void> *signal) {
      GetGlobalMainLoop()->AddTimeoutWatch(0, new SignalPostCallback(signal));
    }

   public:
    // Overridden methods.
    virtual Gadget *GetGadget() const {
      return child_view_ ? child_view_->GetGadget() : NULL;
    }

    virtual bool OnAddContextMenuItems(MenuInterface *menu) {
      return child_view_ ? child_view_->OnAddContextMenuItems(menu) : false;
    }

    virtual EventResult OnOtherEvent(const Event &event) {
      View::OnOtherEvent(event);
      return child_view_ ? child_view_->OnOtherEvent(event) :
          EVENT_RESULT_UNHANDLED;
    }

    virtual bool OnSizing(double *width, double *height) {
      ASSERT(width && height);
      if (*width <= 0 || *height <= 0)
        return false;

      double left, right, top, bottom;
      double cw, ch;
      GetMargins(&left, &right, &top, &bottom);
      GetMinimumClientExtents(&cw, &ch);

      bool result = true;
      cw = std::max(*width - left - right, cw);
      ch = std::max(*height - top - bottom, ch);
      if (IsChildViewVisible()) {
        result = view_element_->OnSizing(&cw, &ch);
      } else {
        result = OnClientSizing(&cw, &ch);
      }
      cw += (left + right);
      ch += (top + bottom);

      if (*width < cw || !allow_x_margin_)
        *width = cw;
      if (*height < ch || !allow_y_margin_)
        *height = ch;
      return result;
    }

    virtual void SetResizable(ResizableMode /* resizable */) {
      // Do nothing.
    }

    virtual std::string GetCaption() const {
      return child_view_ ? child_view_->GetCaption() : View::GetCaption();
    }

    virtual void SetWidth(double width) {
      SetSize(width, GetHeight());
    }

    virtual void SetHeight(double height) {
      SetSize(GetWidth(), height);
    }

    virtual void SetSize(double width, double height) {
      if (GetWidth() == width && GetHeight() == height)
        return;

      DLOG("DecoratedView::SetSize(%lf, %lf)", width, height);
      double left, right, top, bottom;
      double cw, ch;
      GetMargins(&left, &right, &top, &bottom);
      GetMinimumClientExtents(&cw, &ch);

      if (IsChildViewVisible()) {
        double vw = std::max(width - left - right, cw);
        double vh = std::max(height - top - bottom, ch);
        view_element_->SetSize(vw, vh);

        // Call SetViewSize directly here to make sure that
        // allow_x_margin_ and allow_y_margin_ can take effect.
        cw = std::max(view_element_->GetPixelWidth(), cw);
        ch = std::max(view_element_->GetPixelHeight(), ch);
      } else {
        cw = std::max(width - left - right, cw);
        ch = std::max(height - top - bottom, ch);
      }

      cw += (left + right);
      ch += (top + bottom);
      if (SetViewSize(width, height, cw, ch))
        Layout();
    }

   public:
    virtual bool ShowDecoratedView(bool modal, int flags,
                                   Slot1<void, int> *feedback_handler) {
      // Nothing else. Derived class shall override this method to do more
      // things.
      return ShowView(modal, flags, feedback_handler);
    }

    virtual void CloseDecoratedView() {
      // Derived class shall override this method to do more things.
      CloseView();
    }

   protected:
    // To be implemented by derived classes to report suitable client size when
    // child view is not visible.
    virtual bool OnClientSizing(double *width, double *height) {
      return true;
    }

    // To be implemented by derived classes, which will be called when child
    // view is changed.
    virtual void ChildViewChanged() {
      // Nothing to do.
    }

    // To be implemented by derived classes, when the window size is changed.
    virtual void DoLayout() {
      // Nothing to do.
    }

    virtual void GetMargins(double *left, double *right,
                            double *top, double *bottom) {
      *left = 0;
      *right = 0;
      *top = 0;
      *bottom = 0;
    }

    virtual void GetMinimumClientExtents(double *width, double *height) {
      *width = 0;
      *height = 0;
    }

    // Derived classes shall override this method to return current client
    // size.
    virtual void GetClientExtents(double *width, double *height) {
      *width = view_element_->GetPixelWidth();
      *height = view_element_->GetPixelHeight();
    }

   private:
    // Returns true if the view size was changed.
    bool SetViewSize(double req_w, double req_h, double min_w, double min_h) {
      if (req_w < min_w || !allow_x_margin_)
        req_w = min_w;
      if (req_h < min_h || !allow_y_margin_)
        req_h = min_h;

      if (req_w != GetWidth() || req_h != GetHeight()) {
        DLOG("DecoratedView::SetViewSize(%lf, %lf)", req_w, req_h);
        View::SetSize(req_w, req_h);
        return true;
      }
      return false;
    }

   private:
    bool allow_x_margin_;
    bool allow_y_margin_;
    View *child_view_;
    ViewElement *view_element_;
  };
  // End of ViewDecoratorBase.

  // Decorator for main view in sidebar or standalone mode.
  class NormalMainViewDecorator : public ViewDecoratorBase {
    struct ButtonInfo {
      const char *tooltip;
      const char *normal;
      const char *over;
      const char *down;

      void (NormalMainViewDecorator::*handler)(void);
    };

    enum ButtonId {
      BACK = 0,
      FORWARD,
      TOGGLE_EXPANDED,
      MENU,
      CLOSE,
      N_BUTTONS
    };

    static const ButtonInfo kButtonsInfo[];

   public:
    NormalMainViewDecorator(ViewHostInterface *view_host,
                            DecoratedViewHost::Impl *owner,
                            bool sidebar, bool transparent)
      : ViewDecoratorBase(view_host, sidebar, false),
        owner_(owner),
        sidebar_(sidebar),
        transparent_(transparent),
        minimized_(false),
        popped_out_(false),
        mouseover_(false),
        update_visibility_timer_(0),
        hittest_(HT_CLIENT),
        child_resizable_(RESIZABLE_TRUE),
        background_(NULL),
        bottom_(NULL),
        buttons_div_(NULL),
        minimized_bkgnd_(NULL),
        icon_(NULL),
        caption_(NULL),
        snapshot_(NULL),
        plugin_flags_connection_(NULL),
        original_child_view_(NULL) {
      // The initialization sequence of following elements must not be changed.
      // Sidebar mode doesn't have background.
      if (!sidebar_) {
        background_ = new ImgElement(NULL, this, NULL);
        background_->SetSrc(Variant(transparent_ ?
                                    kVDMainBackgroundTransparent :
                                    kVDMainBackground));
        background_->SetStretchMiddle(true);
        background_->SetPixelX(0);
        background_->SetPixelY(transparent_ ? kVDMainToolbarHeight : 0);
        background_->EnableCanvasCache(true);
        background_->SetVisible(false);
        GetChildren()->InsertElement(background_, GetViewElement());
      }

      // For standalone mode, bottom right corner will be used.
      // For sidebar mode, bottom line will be used.
      bottom_ = new ImgElement(NULL, this, NULL);
      bottom_->SetSrc(Variant(sidebar_ ? kVDMainSidebarBottom :
                              kVDBottomRightCorner));
      bottom_->SetRelativePinY(1);
      bottom_->SetRelativeY(1);
      if (!sidebar_) {
        bottom_->SetRelativePinX(1);
        bottom_->SetRelativeX(1);
        bottom_->SetHitTest(HT_BOTTOMRIGHT);
        bottom_->SetCursor(CURSOR_SIZENWSE);
      } else {
        bottom_->SetRelativeWidth(1);
        bottom_->SetHitTest(HT_BOTTOM);
        bottom_->SetCursor(CURSOR_SIZENS);
      }
      bottom_->SetVisible(false);
      GetChildren()->InsertElement(bottom_, NULL);

      if (transparent_) {
        minimized_bkgnd_= new ImgElement(NULL, this, NULL);
        minimized_bkgnd_->SetSrc(Variant(kVDMainBackgroundMinimized));
        minimized_bkgnd_->SetStretchMiddle(true);
        minimized_bkgnd_->SetPixelHeight(kVDMainMinimizedHeight);
        minimized_bkgnd_->SetPixelX(sidebar_ ? 0 : kVDMainBorderWidth);
        minimized_bkgnd_->SetPixelY(sidebar_ ? kVDMainBorderWidth :
                                    kVDMainToolbarHeight + kVDMainBorderWidth);
        minimized_bkgnd_->SetVisible(false);
        minimized_bkgnd_->SetEnabled(true);
        minimized_bkgnd_->ConnectOnClickEvent(
          NewSlot(this, &NormalMainViewDecorator::OnToggleExpandedButtonClicked));
        GetChildren()->InsertElement(minimized_bkgnd_, NULL);
      }

      icon_ = new ImgElement(NULL, this, NULL);
      icon_->SetRelativePinY(0.5);
      icon_->SetPixelX(sidebar_ ? kVDMainIconMarginH :
                       kVDMainIconMarginH + kVDMainBorderWidth);
      icon_->SetPixelY((sidebar_ ? kVDMainBorderWidth :
                        kVDMainToolbarHeight + kVDMainBorderWidth) +
                       kVDMainMinimizedHeight * 0.5);
      icon_->SetVisible(false);
      GetChildren()->InsertElement(icon_, NULL);

      caption_ = new LabelElement(NULL, this, NULL);
      caption_->GetTextFrame()->SetSize(10);
      caption_->GetTextFrame()->SetColor(Color::kWhite, 1);
      caption_->GetTextFrame()->SetWordWrap(false);
      caption_->GetTextFrame()->SetTrimming(
          CanvasInterface::TRIMMING_CHARACTER_ELLIPSIS);
      caption_->SetPixelHeight(kVDMainMinimizedHeight -
                               kVDMainCaptionMarginV * 2);
      caption_->SetPixelY((sidebar_ ? kVDMainBorderWidth :
                          kVDMainToolbarHeight + kVDMainBorderWidth) +
                          kVDMainCaptionMarginV);
      caption_->SetVisible(false);
      GetChildren()->InsertElement(caption_, NULL);

      snapshot_ = new CopyElement(NULL, this, NULL);
      snapshot_->SetVisible(false);
      snapshot_->SetOpacity(0.5);
      GetChildren()->InsertElement(snapshot_, NULL);

      buttons_div_ = new DivElement(NULL, this, NULL);
      buttons_div_->SetRelativePinX(1);
      buttons_div_->SetPixelPinY(0);
      buttons_div_->SetRelativeX(1);
      buttons_div_->SetPixelY(0);
      buttons_div_->SetPixelHeight(kVDMainToolbarHeight);
      buttons_div_->SetBackgroundMode(
          DivElement::BACKGROUND_MODE_STRETCH_MIDDLE);
      buttons_div_->SetBackground(Variant(kVDButtonBackground));
      buttons_div_->SetVisible(false);
      GetChildren()->InsertElement(buttons_div_, NULL);

      Elements *elements = buttons_div_->GetChildren();
      for (size_t i = 0; i < N_BUTTONS; ++i) {
        ButtonElement *button = new ButtonElement(buttons_div_, this, NULL);
        button->SetTooltip(GM_(kButtonsInfo[i].tooltip));
        button->SetImage(Variant(kButtonsInfo[i].normal));
        button->SetOverImage(Variant(kButtonsInfo[i].over));
        button->SetDownImage(Variant(kButtonsInfo[i].down));
        button->ConnectOnClickEvent(NewSlot(this, kButtonsInfo[i].handler));
        elements->InsertElement(button, NULL);
      }

      LayoutButtons();
    }

    ~NormalMainViewDecorator() {
      if (update_visibility_timer_)
        ClearTimeout(update_visibility_timer_);
      if (plugin_flags_connection_)
        plugin_flags_connection_->Disconnect();
    }

   public:
    virtual EventResult OnMouseEvent(const MouseEvent &event) {
      EventResult result = ViewDecoratorBase::OnMouseEvent(event);

      Event::Type t = event.GetType();
      if (t == Event::EVENT_MOUSE_OVER || t == Event::EVENT_MOUSE_OUT) {
        mouseover_ = (t == Event::EVENT_MOUSE_OVER);
        if (!update_visibility_timer_) {
          update_visibility_timer_ = SetTimeout(
              NewSlot(this, &NormalMainViewDecorator::UpdateVisibility),
              mouseover_ ? kVDShowTimeout : kVDHideTimeout);
        }

        if (!mouseover_) {
          hittest_ = HT_CLIENT;
          GetViewHost()->SetCursor(-1);
        }
      } else if (t == Event::EVENT_MOUSE_MOVE) {
        if (!mouseover_) {
          mouseover_ = true;
          UpdateVisibility();
        }

        bool h_resizable = false;
        bool v_resizable = false;
        if (minimized_) {
          h_resizable = true;
        } else if (child_resizable_ == RESIZABLE_TRUE) {
          h_resizable = true;
          v_resizable = true;
        }

        double x = event.GetX();
        double y = event.GetY();
        double w = GetWidth();
        double h = GetHeight();
        double top = transparent_ ? kVDMainToolbarHeight : 0;

        hittest_ = HT_CLIENT;
        if (!sidebar_) {
          // Only show bottom right corner when there is no transparent
          // background or the child view is not resizable.
          if (!transparent_ ||
              (child_resizable_ != RESIZABLE_TRUE && !minimized_)) {
            if (x > w - kVDMainCornerSize && y > h - kVDMainCornerSize)
              bottom_->SetVisible(true);
            else
              bottom_->SetVisible(false);
          } else if (x >= w - kVDMainBorderWidth * 2 &&
                     y >= h - kVDMainBorderWidth * 2) {
            hittest_ = HT_BOTTOMRIGHT;
            GetViewHost()->SetCursor(CURSOR_SIZENWSE);
          } else if (x >= w - kVDMainBorderWidth * 2 &&
                     y >= top && y <= top + kVDMainBorderWidth * 2) {
            hittest_ = HT_TOPRIGHT;
            GetViewHost()->SetCursor(CURSOR_SIZENESW);
          } else if (x <= kVDMainBorderWidth * 2 &&
                     y >= top && y <= top + kVDMainBorderWidth * 2) {
            hittest_ = HT_TOPLEFT;
            GetViewHost()->SetCursor(CURSOR_SIZENWSE);
          } else if (x <= kVDMainBorderWidth  * 2 &&
                     y >= h - kVDMainBorderWidth * 2) {
            hittest_ = HT_BOTTOMLEFT;
            GetViewHost()->SetCursor(CURSOR_SIZENESW);
          } else if (x >= w - kVDMainBorderWidth && h_resizable) {
            hittest_ = HT_RIGHT;
            GetViewHost()->SetCursor(CURSOR_SIZEWE);
          } else if (x <= kVDMainBorderWidth && h_resizable) {
            hittest_ = HT_LEFT;
            GetViewHost()->SetCursor(CURSOR_SIZEWE);
          } else if (y >= h - kVDMainBorderWidth && v_resizable) {
            hittest_ = HT_BOTTOM;
            GetViewHost()->SetCursor(CURSOR_SIZENS);
          } else if (y >= top && y <= top + kVDMainBorderWidth &&
                     v_resizable) {
            hittest_ = HT_TOP;
            GetViewHost()->SetCursor(CURSOR_SIZENS);
          }
        } else if (y >= h - kVDMainBorderWidth && !minimized_) {
          hittest_ = HT_BOTTOM;
          GetViewHost()->SetCursor(CURSOR_SIZENS);
        }
      }
      return result;
    }

    struct ZoomFunctor {
      ZoomFunctor(NormalMainViewDecorator *owner, double zoom)
        : owner_(owner), zoom_(zoom) { }

      void operator()(const char *) const { owner_->OnZoom(zoom_); }
      bool operator==(const ZoomFunctor &another) const {
        return owner_ == another.owner_ && zoom_ == another.zoom_;
      }

      NormalMainViewDecorator *owner_;
      double zoom_;
    };

    virtual bool OnAddContextMenuItems(MenuInterface *menu) {
      menu->AddItem(
          GM_(minimized_ ? "MENU_ITEM_EXPAND" : "MENU_ITEM_COLLAPSE"), 0,
          NewSlot(this, &NormalMainViewDecorator::CollapseExpandMenuCallback));

      menu->AddItem(
          GM_(sidebar_ ? "MENU_ITEM_UNDOCK" : "MENU_ITEM_DOCK"), 0,
          NewSlot(this, sidebar_ ?
                  &NormalMainViewDecorator::UndockMenuCallback :
                  &NormalMainViewDecorator::DockMenuCallback));

      if (!sidebar_ && !minimized_ && !popped_out_) {
        MenuInterface *zoom = menu->AddPopup(GM_("MENU_ITEM_ZOOM"));
        zoom->AddItem(GM_("MENU_ITEM_AUTO_FIT"), 0,
              NewFunctorSlot<void, const char *>(ZoomFunctor(this, 1.0)));
        zoom->AddItem(GM_("MENU_ITEM_50P"), 0,
              NewFunctorSlot<void, const char *>(ZoomFunctor(this, 0.5)));
        zoom->AddItem(GM_("MENU_ITEM_75P"), 0,
              NewFunctorSlot<void, const char *>(ZoomFunctor(this, 0.75)));
        zoom->AddItem(GM_("MENU_ITEM_100P"), 0,
              NewFunctorSlot<void, const char *>(ZoomFunctor(this, 1.0)));
        zoom->AddItem(GM_("MENU_ITEM_125P"), 0,
              NewFunctorSlot<void, const char *>(ZoomFunctor(this, 1.25)));
        zoom->AddItem(GM_("MENU_ITEM_150P"), 0,
              NewFunctorSlot<void, const char *>(ZoomFunctor(this, 1.50)));
        zoom->AddItem(GM_("MENU_ITEM_175P"), 0,
              NewFunctorSlot<void, const char *>(ZoomFunctor(this, 1.75)));
        zoom->AddItem(GM_("MENU_ITEM_200P"), 0,
              NewFunctorSlot<void, const char *>(ZoomFunctor(this, 2.0)));
      }

      View *child = GetChildView();
      if (child || original_child_view_) {
        menu->AddItem("", MenuInterface::MENU_ITEM_FLAG_SEPARATOR, NULL);
        if (child)
          return child->OnAddContextMenuItems(menu);
        else
          return original_child_view_->OnAddContextMenuItems(menu);
      }
      return true;
    }

    virtual EventResult OnOtherEvent(const Event &event) {
      Event::Type t = event.GetType();
      if (t == Event::EVENT_POPOUT && !popped_out_) {
        original_child_view_ = GetChildView();
        popped_out_ = true;
        // Take a snapshot for the child view.
        snapshot_->SetFrozen(false);
        if (minimized_)
          SetChildViewVisible(true);
        snapshot_->SetSrc(Variant(GetViewElement()));
        snapshot_->SetFrozen(true);
        snapshot_->SetSrc(Variant());
        snapshot_->SetVisible(!minimized_);
        SetChildViewVisible(false);
        UpdateToggleExpandedButton();
        UpdateViewSize();
      } else if (t == Event::EVENT_POPIN && popped_out_) {
        original_child_view_ = NULL;
        popped_out_ = false;
        snapshot_->SetVisible(false);
        SetChildViewVisible(!minimized_);
        UpdateToggleExpandedButton();
        UpdateViewSize();
      }

      // Handle popout/popin events.
      return ViewDecoratorBase::OnOtherEvent(event);
    }

    virtual HitTest GetHitTest() const {
      if (hittest_ != HT_CLIENT)
        return hittest_;
      return ViewDecoratorBase::GetHitTest();
    }

    virtual void SetResizable(ResizableMode resizable) {
      if (child_resizable_ != resizable) {
        child_resizable_= resizable;
        UpdateViewSize();
      }
    }

    virtual void SetCaption(const char *caption) {
      caption_->GetTextFrame()->SetText(caption);
      ViewDecoratorBase::SetCaption(caption);
    }

   public:
    virtual void CloseDecoratedView() {
      if (popped_out_)
        owner_->on_popin_signal_();
      ViewDecoratorBase::CloseDecoratedView();
    }

   protected:
    virtual bool OnClientSizing(double *width, double *height) {
      if (minimized_)
        *height = kVDMainMinimizedHeight;

      return true;
    }

    virtual void ChildViewChanged() {
      if (plugin_flags_connection_) {
        plugin_flags_connection_->Disconnect();
        plugin_flags_connection_ = NULL;
      }

      View *child = GetChildView();
      Gadget *gadget = child ? child->GetGadget() : NULL;
      if (gadget) {
        gadget->ConnectOnPluginFlagsChanged(
            NewSlot(this, &NormalMainViewDecorator::OnPluginFlagsChanged));
        OnPluginFlagsChanged(gadget->GetPluginFlags());

        icon_->SetSrc(Variant(gadget->GetManifestInfo(kManifestSmallIcon)));
        icon_->SetPixelWidth(std::min(kVDMainIconWidth,
                                      icon_->GetSrcWidth()));
        icon_->SetPixelHeight(std::min(kVDMainIconHeight,
                                       icon_->GetSrcHeight()));
      } else {
        OnPluginFlagsChanged(0);
        // Keep the icon unchanged.
      }

      if (child) {
        SetResizable(child->GetResizable());
        caption_->GetTextFrame()->SetText(child->GetCaption());
        if (minimized_) {
          SimpleEvent event(Event::EVENT_MINIMIZE);
          child->OnOtherEvent(event);
        }
      }

      DoLayout();
    }

    virtual void DoLayout() {
      if (background_) {
        background_->SetPixelWidth(GetWidth());
        background_->SetPixelHeight(GetHeight() - background_->GetPixelY());
      }

      if (minimized_bkgnd_) {
        minimized_bkgnd_->SetPixelWidth(
            GetWidth() - (sidebar_ ? 0 : kVDMainBorderWidth * 2));
      }

      caption_->SetPixelX(icon_->GetPixelX() + icon_->GetPixelWidth() +
                          kVDMainIconMarginH);
      caption_->SetPixelWidth(GetWidth() - caption_->GetPixelX() -
                              kVDMainBorderWidth - kVDMainCaptionMarginH);

      if (popped_out_ && snapshot_->IsVisible()) {
        double left, right, top, bottom;
        GetMargins(&left, &right, &top, &bottom);
        double cw = GetWidth() - left - right;
        double ch = GetHeight() - top - bottom;
        double sw = snapshot_->GetSrcWidth();
        double sh = snapshot_->GetSrcHeight();
        if (sw > 0 && sh > 0 && cw > 0 && ch > 0) {
          double aspect_ratio = sw / sh;
          if (cw / ch < aspect_ratio) {
            sw = cw;
            sh = sw / aspect_ratio;
          } else {
            sh = ch;
            sw = sh * aspect_ratio;
          }
          cw = left + (cw - sw) / 2.0;
          ch = top + (ch - sh) / 2.0;
          snapshot_->SetPixelX(cw);
          snapshot_->SetPixelY(ch);
          snapshot_->SetPixelWidth(sw);
          snapshot_->SetPixelHeight(sh);
        }
      }
    }

    virtual void GetMargins(double *left, double *right,
                            double *top, double *bottom) {
      *left = 0;
      *right = 0;
      *top = kVDMainToolbarHeight;
      *bottom = 0;

      if (!sidebar_) {
        if (child_resizable_ == RESIZABLE_TRUE || minimized_) {
          *left = kVDMainBorderWidth;
          *right = kVDMainBorderWidth;
          *bottom = kVDMainBorderWidth;
          if (transparent_)
            *top += kVDMainBorderWidth;
        }
      } else {
        if (minimized_)
          *top = kVDMainBorderWidth;
        *bottom = kVDMainBorderWidth;
      }
    }

    virtual void GetMinimumClientExtents(double *width, double *height) {
      if (minimized_) {
        *width = kVDMainIconWidth + kVDMainIconMarginH * 2;
        *height = kVDMainMinimizedHeight;
      } else {
        *width = 0;
        *height = 0;
      }
    }

    virtual void GetClientExtents(double *width, double *height) {
      if (minimized_) {
        *height = kVDMainMinimizedHeight;
      } else if (popped_out_) {
        *width = snapshot_->GetSrcWidth();
        *height = snapshot_->GetSrcHeight();
      } else {
        ViewDecoratorBase::GetClientExtents(width, height);
      }
    }

   private:
    void UpdateVisibility() {
      update_visibility_timer_ = 0;
      if (mouseover_) {
        // If there is no transparent background, the background image will
        // always be shown.
        // Otherwise, the background image will be shown if child view is
        // resizable or minimized.
        if (background_ && transparent_)
            background_->SetVisible(child_resizable_ == RESIZABLE_TRUE ||
                                    minimized_);

        // Toolbar buttons will always be shown when mouse is over.
        if (buttons_div_)
          buttons_div_->SetVisible(true);

        // When in standalone mode, the bottom right corner will only be shown
        // when mouse pointer is near the corner.
        // In sidebar mode, the bottom line will always be shown when mouse is
        // over.
        if (sidebar_ && bottom_)
          bottom_->SetVisible(true);

        // The visibility of caption_, minimized_bkgnd_, icon_ and
        // snapshot_ are set in corresponding event or menu handler.
      } else {
        if (background_ && transparent_)
          background_->SetVisible(false);
        if (buttons_div_)
          buttons_div_->SetVisible(false);
        if (bottom_)
          bottom_->SetVisible(false);
      }
      GetViewHost()->EnableInputShapeMask(!mouseover_);
    }

    void LayoutButtons() {
      Elements *elements = buttons_div_->GetChildren();

      double x = 0;
      int count = elements->GetCount();
      for (int i = 0; i < count; ++i) {
        BasicElement *button = elements->GetItemByIndex(i);
        if (button && button->IsVisible()) {
          button->SetPixelX(x);
          x += kVDMainButtonWidth;
        }
      }
      buttons_div_->SetPixelWidth(x);
    }

    void UpdateToggleExpandedButton() {
      Elements *elements = buttons_div_->GetChildren();
      ButtonElement *btn =
          down_cast<ButtonElement *>(elements->GetItemByIndex(TOGGLE_EXPANDED));
      btn->SetImage(Variant(
          popped_out_ ? kVDButtonUnexpandNormal : kVDButtonExpandNormal));
      btn->SetOverImage(Variant(
          popped_out_ ? kVDButtonUnexpandOver : kVDButtonExpandOver));
      btn->SetDownImage(Variant(
          popped_out_ ? kVDButtonUnexpandDown : kVDButtonExpandDown));
    }

    void OnBackButtonClicked() {
      View *child = GetChildView();
      Gadget *gadget = child ? child->GetGadget() : NULL;
      if (gadget)
        gadget->OnCommand(Gadget::CMD_TOOLBAR_BACK);
    }

    void OnForwardButtonClicked() {
      View *child = GetChildView();
      Gadget *gadget = child ? child->GetGadget() : NULL;
      if (gadget)
        gadget->OnCommand(Gadget::CMD_TOOLBAR_FORWARD);
    }

    void OnToggleExpandedButtonClicked() {
      if (popped_out_)
        owner_->on_popin_signal_();
      else
        owner_->on_popout_signal_();
    }

    void OnMenuButtonClicked() {
      GetViewHost()->ShowContextMenu(MouseEvent::BUTTON_LEFT);
    }

    void OnCloseButtonClicked() {
      if (popped_out_)
        owner_->on_popin_signal_();
      PostSignal(&owner_->on_close_signal_);
    }

    void OnPluginFlagsChanged(int flags) {
      Elements *elements = buttons_div_->GetChildren();
      elements->GetItemByIndex(BACK)->SetVisible(
          flags & Gadget::PLUGIN_FLAG_TOOLBAR_BACK);
      elements->GetItemByIndex(FORWARD)->SetVisible(
          flags & Gadget::PLUGIN_FLAG_TOOLBAR_FORWARD);
      LayoutButtons();
    }

    void CollapseExpandMenuCallback(const char *) {
      minimized_ = !minimized_;

      if (minimized_bkgnd_)
        minimized_bkgnd_->SetVisible(minimized_);
      icon_->SetVisible(minimized_);
      caption_->SetVisible(minimized_);

      if (popped_out_)
        snapshot_->SetVisible(!minimized_);
      else
        SetChildViewVisible(!minimized_);

      UpdateVisibility();
      UpdateViewSize();

      View *child = GetChildView();
      if (child) {
        SimpleEvent event(minimized_ ? Event::EVENT_MINIMIZE :
                          Event::EVENT_RESTORE);
        child->OnOtherEvent(event);
      }
    }

    void DockMenuCallback(const char *) {
      owner_->on_dock_signal_();
    }

    void UndockMenuCallback(const char *) {
      owner_->on_undock_signal_();
    }

    void OnZoom(double zoom) {
      SetChildViewScale(zoom);
    }

   private:
    DecoratedViewHost::Impl *owner_;

    bool sidebar_;
    bool transparent_;

    bool minimized_;
    bool popped_out_;
    bool mouseover_;
    bool always_show_caption_;

    int update_visibility_timer_;

    ViewInterface::HitTest hittest_;
    ViewInterface::ResizableMode child_resizable_;

    // Once added to this view, these are owned by the view.
    // Do not delete.
    ImgElement *background_;

    // Could be bottom right corner if transparent is false,
    // Or bottom bar if in sidebar.
    ImgElement *bottom_;

    // Holds all toolbar buttons.
    DivElement *buttons_div_;

    // Background for minimized mode.
    ImgElement *minimized_bkgnd_;

    // Gadget icon, will only be shown when the view is minimized.
    ImgElement *icon_;

    // When the view is minimized, it'll always be shown in the middle.
    LabelElement *caption_;

    // to save snapshot of child view.
    CopyElement *snapshot_;

    Connection *plugin_flags_connection_;

    View *original_child_view_;
  };
  // End of NormalMainViewDecorator

  // Decorator for expanded main view.
  class ExpandedMainViewDecorator : public ViewDecoratorBase {
   public:
    ExpandedMainViewDecorator(ViewHostInterface *view_host,
                              DecoratedViewHost::Impl *owner)
      : ViewDecoratorBase(view_host, false, false),
        owner_(owner),
        hittest_(HT_CLIENT),
        close_button_(NULL),
        caption_(NULL),
        top_margin_(0) {
      ImgElement *top = new ImgElement(NULL, this, NULL);
      top->SetSrc(Variant(kVDPopOutBackgroundTitle));
      top->SetStretchMiddle(true);
      top->SetPixelX(0);
      top->SetPixelY(0);
      top->SetRelativeWidth(1);
      GetChildren()->InsertElement(top, GetViewElement());
      top_margin_ = top->GetSrcHeight() + kVDExpandedBorderWidth;

      ImgElement *bkgnd = new ImgElement(NULL, this, NULL);
      bkgnd->SetSrc(Variant(kVDPopOutBackground));
      bkgnd->SetStretchMiddle(true);
      bkgnd->SetPixelX(0);
      bkgnd->SetPixelY(0);
      bkgnd->SetRelativeWidth(1);
      bkgnd->SetRelativeHeight(1);
      bkgnd->EnableCanvasCache(true);
      GetChildren()->InsertElement(bkgnd, GetViewElement());

      caption_ = new LabelElement(NULL, this, NULL);
      caption_->GetTextFrame()->SetSize(10);
      caption_->GetTextFrame()->SetColor(Color::kBlack, 1);
      caption_->GetTextFrame()->SetWordWrap(false);
      caption_->GetTextFrame()->SetTrimming(
          CanvasInterface::TRIMMING_CHARACTER);
      caption_->SetPixelX(kVDExpandedBorderWidth);
      caption_->SetPixelY(kVDExpandedBorderWidth);
      GetChildren()->InsertElement(caption_, NULL);

      close_button_ = new ButtonElement(NULL, this, NULL);
      close_button_->SetPixelY(kVDExpandedBorderWidth);
      close_button_->SetImage(Variant(kVDPopOutCloseNormal));
      close_button_->SetOverImage(Variant(kVDPopOutCloseOver));
      close_button_->SetDownImage(Variant(kVDPopOutCloseDown));
      close_button_->ConnectOnClickEvent(
          NewSlot(this, &ExpandedMainViewDecorator::OnCloseButtonClicked));
      GetChildren()->InsertElement(close_button_, NULL);
      view_host->EnableInputShapeMask(false);
    }

   public:
    virtual EventResult OnMouseEvent(const MouseEvent &event) {
      EventResult result = ViewDecoratorBase::OnMouseEvent(event);

      Event::Type t = event.GetType();
      if (t == Event::EVENT_MOUSE_OUT) {
        hittest_ = HT_CLIENT;
        GetViewHost()->SetCursor(-1);
      } else if (t == Event::EVENT_MOUSE_MOVE) {
        double x = event.GetX();
        double y = event.GetY();
        double w = GetWidth();
        double h = GetHeight();

        View *child = GetChildView();
        bool resizable =
            child ? (child->GetResizable() == RESIZABLE_TRUE) : false;

        hittest_ = HT_CLIENT;
        if (resizable) {
          if (x >= w - kVDExpandedBorderWidth &&
              y >= h - kVDExpandedBorderWidth) {
            hittest_ = HT_BOTTOMRIGHT;
            GetViewHost()->SetCursor(CURSOR_SIZENWSE);
          } else if (x >= w - kVDExpandedBorderWidth &&
                     y <= kVDExpandedBorderWidth) {
            hittest_ = HT_TOPRIGHT;
            GetViewHost()->SetCursor(CURSOR_SIZENESW);
          } else if (x <= kVDExpandedBorderWidth &&
                     y <= kVDExpandedBorderWidth) {
            hittest_ = HT_TOPLEFT;
            GetViewHost()->SetCursor(CURSOR_SIZENWSE);
          } else if (x <= kVDExpandedBorderWidth &&
                     y >= h - kVDExpandedBorderWidth) {
            hittest_ = HT_BOTTOMLEFT;
            GetViewHost()->SetCursor(CURSOR_SIZENESW);
          } else if (x >= w - kVDExpandedBorderWidth) {
            hittest_ = HT_RIGHT;
            GetViewHost()->SetCursor(CURSOR_SIZEWE);
          } else if (x <= kVDExpandedBorderWidth) {
            hittest_ = HT_LEFT;
            GetViewHost()->SetCursor(CURSOR_SIZEWE);
          } else if (y >= h - kVDExpandedBorderWidth) {
            hittest_ = HT_BOTTOM;
            GetViewHost()->SetCursor(CURSOR_SIZENS);
          } else if (y <= kVDExpandedBorderWidth) {
            hittest_ = HT_TOP;
            GetViewHost()->SetCursor(CURSOR_SIZENS);
          }
        }
      }
      return result;
    }

    virtual HitTest GetHitTest() const {
      if (hittest_ != HT_CLIENT)
        return hittest_;
      return ViewDecoratorBase::GetHitTest();
    }

    virtual void SetCaption(const char *caption) {
      caption_->GetTextFrame()->SetText(caption);
      ViewDecoratorBase::SetCaption(caption);
    }

   protected:
    virtual void ChildViewChanged() {
      View *child = GetChildView();
      if (child)
        caption_->GetTextFrame()->SetText(child->GetCaption());
    }

    virtual void DoLayout() {
      close_button_->SetPixelX(GetWidth() - close_button_->GetPixelWidth() -
                               kVDExpandedBorderWidth);
      caption_->SetPixelWidth(close_button_->GetPixelX() -
                              caption_->GetPixelX() - 1);
    }

    virtual void GetMargins(double *left, double *right,
                            double *top, double *bottom) {
      *left = kVDExpandedBorderWidth;
      *right = kVDExpandedBorderWidth;
      *top = top_margin_;
      *bottom = kVDExpandedBorderWidth;
    }

   private:
    void OnCloseButtonClicked() {
      PostSignal(&owner_->on_close_signal_);
    }

   private:
    DecoratedViewHost::Impl *owner_;
    ViewInterface::HitTest hittest_;

    // Once added to this view, these are owned by the view.
    // Do not delete.
    ImgElement *background_;
    // Close button.
    ButtonElement *close_button_;
    LabelElement *caption_;
    double top_margin_;
  };
  // End of ExpandedMainViewDecorator

  // Decorator for details view.
  class DetailsViewDecorator : public ViewDecoratorBase {
   public:
    DetailsViewDecorator(ViewHostInterface *view_host,
                         DecoratedViewHost::Impl *owner)
      : ViewDecoratorBase(view_host, false, false),
        owner_(owner),
        hittest_(HT_CLIENT),
        background_(NULL),
        bottom_(NULL),
        close_button_(NULL),
        remove_button_(NULL),
        negative_button_(NULL),
        caption_(NULL),
        flags_(0),
        feedback_handler_(NULL) {
      ImgElement *top = new ImgElement(NULL, this, NULL);
      top->SetSrc(Variant(kVDDetailsTop));
      top->SetStretchMiddle(true);
      top->SetPixelX(0);
      top->SetPixelY(0);
      top->SetRelativeWidth(1);
      GetChildren()->InsertElement(top, GetViewElement());

      background_ = new ImgElement(NULL, this, NULL);
      background_->SetSrc(Variant(kVDDetailsMiddle));
      background_->SetStretchMiddle(true);
      background_->SetPixelX(0);
      background_->SetPixelY(top->GetSrcHeight());
      background_->SetRelativeWidth(1);
      background_->EnableCanvasCache(true);
      GetChildren()->InsertElement(background_, GetViewElement());

      bottom_ = new ImgElement(NULL, this, NULL);
      bottom_->SetSrc(Variant(kVDDetailsBottom));
      bottom_->SetStretchMiddle(true);
      bottom_->SetPixelX(0);
      bottom_->SetRelativeY(1);
      bottom_->SetRelativePinY(1);
      bottom_->SetRelativeWidth(1);
      GetChildren()->InsertElement(bottom_, GetViewElement());

      caption_ = new LabelElement(NULL, this, NULL);
      caption_->GetTextFrame()->SetSize(10);
      caption_->GetTextFrame()->SetColor(Color::kBlack, 1);
      caption_->GetTextFrame()->SetWordWrap(false);
      caption_->GetTextFrame()->SetTrimming(
          CanvasInterface::TRIMMING_CHARACTER);
      caption_->SetPixelX(kVDDetailsBorderWidth);
      caption_->SetPixelY(kVDDetailsBorderWidth);
      GetChildren()->InsertElement(caption_, NULL);

      close_button_ = new ButtonElement(NULL, this, NULL);
      close_button_->SetPixelY(kVDDetailsBorderWidth);
      close_button_->SetImage(Variant(kVDPopOutCloseNormal));
      close_button_->SetOverImage(Variant(kVDPopOutCloseOver));
      close_button_->SetDownImage(Variant(kVDPopOutCloseDown));
      close_button_->ConnectOnClickEvent(
          NewSlot(this, &DetailsViewDecorator::OnCloseButtonClicked));
      GetChildren()->InsertElement(close_button_, NULL);

      view_host->EnableInputShapeMask(false);
    }

    ~DetailsViewDecorator() {
      delete feedback_handler_;
    }

   public:
    virtual EventResult OnMouseEvent(const MouseEvent &event) {
      EventResult result = ViewDecoratorBase::OnMouseEvent(event);

      Event::Type t = event.GetType();
      if (t == Event::EVENT_MOUSE_OUT) {
        hittest_ = HT_CLIENT;
        GetViewHost()->SetCursor(-1);
      } else if (t == Event::EVENT_MOUSE_MOVE) {
        double x = event.GetX();
        double y = event.GetY();
        double w = GetWidth();
        double h = GetHeight();

        View *child = GetChildView();
        bool resizable =
            child ? (child->GetResizable() == RESIZABLE_TRUE) : false;

        hittest_ = HT_CLIENT;
        if (resizable) {
          if (x >= w - kVDDetailsBorderWidth &&
              y >= h - kVDDetailsBorderWidth) {
            hittest_ = HT_BOTTOMRIGHT;
            GetViewHost()->SetCursor(CURSOR_SIZENWSE);
          } else if (x >= w - kVDDetailsBorderWidth &&
                     y <= kVDDetailsBorderWidth) {
            hittest_ = HT_TOPRIGHT;
            GetViewHost()->SetCursor(CURSOR_SIZENESW);
          } else if (x <= kVDDetailsBorderWidth &&
                     y <= kVDDetailsBorderWidth) {
            hittest_ = HT_TOPLEFT;
            GetViewHost()->SetCursor(CURSOR_SIZENWSE);
          } else if (x <= kVDDetailsBorderWidth &&
                     y >= h - kVDDetailsBorderWidth) {
            hittest_ = HT_BOTTOMLEFT;
            GetViewHost()->SetCursor(CURSOR_SIZENESW);
          } else if (x >= w - kVDDetailsBorderWidth) {
            hittest_ = HT_RIGHT;
            GetViewHost()->SetCursor(CURSOR_SIZEWE);
          } else if (x <= kVDDetailsBorderWidth) {
            hittest_ = HT_LEFT;
            GetViewHost()->SetCursor(CURSOR_SIZEWE);
          } else if (y >= h - kVDDetailsBorderWidth) {
            hittest_ = HT_BOTTOM;
            GetViewHost()->SetCursor(CURSOR_SIZENS);
          } else if (y <= kVDDetailsBorderWidth) {
            hittest_ = HT_TOP;
            GetViewHost()->SetCursor(CURSOR_SIZENS);
          }
        }
      }
      return result;
    }

    virtual HitTest GetHitTest() const {
      if (hittest_ != HT_CLIENT)
        return hittest_;
      return ViewDecoratorBase::GetHitTest();
    }

    virtual void SetCaption(const char *caption) {
      caption_->GetTextFrame()->SetText(caption);
      ViewDecoratorBase::SetCaption(caption);
    }

   public:
    virtual bool ShowDecoratedView(bool modal, int flags,
                                   Slot1<void, int> *feedback_handler) {
      delete feedback_handler_;
      feedback_handler_ = feedback_handler;
      if (flags & ViewInterface::DETAILS_VIEW_FLAG_TOOLBAR_OPEN) {
        caption_->ConnectOnClickEvent(
            NewSlot(this, &DetailsViewDecorator::OnCaptionClicked));
        // Blue text.
        caption_->GetTextFrame()->SetColor(Color(0, 0, 1), 1);
        caption_->GetTextFrame()->SetUnderline(true);
        caption_->SetEnabled(true);
        caption_->SetCursor(CURSOR_HAND);
      }
      if (flags & ViewInterface::DETAILS_VIEW_FLAG_REMOVE_BUTTON) {
        remove_button_ = new ButtonElement(NULL, this, NULL);
        remove_button_->SetImage(Variant(kVDDetailsButtonBkgndNormal));
        remove_button_->SetOverImage(Variant(kVDDetailsButtonBkgndOver));
        remove_button_->SetDownImage(Variant(kVDDetailsButtonBkgndClick));
        remove_button_->SetStretchMiddle(true);
        remove_button_->GetTextFrame()->SetText(GMS_("REMOVE_CONTENT_ITEM"));
        remove_button_->SetPixelHeight(kVDDetailsButtonHeight);
        double tw, th;
        remove_button_->GetTextFrame()->GetSimpleExtents(&tw, &th);
        remove_button_->SetPixelWidth(tw + kVDDetailsButtonMargin * 2);
        remove_button_->ConnectOnClickEvent(
            NewSlot(this, &DetailsViewDecorator::OnRemoveButtonClicked));
        GetChildren()->InsertElement(remove_button_, NULL);
      }
      if (flags & ViewInterface::DETAILS_VIEW_FLAG_NEGATIVE_FEEDBACK) {
        negative_button_ = new ButtonElement(NULL, this, NULL);
        negative_button_->SetImage(Variant(kVDDetailsButtonBkgndNormal));
        negative_button_->SetOverImage(Variant(kVDDetailsButtonBkgndOver));
        negative_button_->SetDownImage(Variant(kVDDetailsButtonBkgndClick));
        negative_button_->SetStretchMiddle(true);
        negative_button_->GetTextFrame()->SetText(
            GMS_("DONT_SHOW_CONTENT_ITEM"));
        negative_button_->SetPixelHeight(kVDDetailsButtonHeight);
        double tw, th;
        negative_button_->GetTextFrame()->GetSimpleExtents(&tw, &th);
        negative_button_->SetPixelWidth(tw + kVDDetailsButtonMargin * 2);
        negative_button_->ConnectOnClickEvent(
            NewSlot(this, &DetailsViewDecorator::OnNegativeButtonClicked));
        GetChildren()->InsertElement(negative_button_, NULL);
      }
      DoLayout();
      return ShowView(modal, 0, NULL);
    }

    virtual void CloseDecoratedView() {
      if (feedback_handler_) {
        (*feedback_handler_)(flags_);
        delete feedback_handler_;
        feedback_handler_ = NULL;
      }
      ViewDecoratorBase::CloseDecoratedView();
    }

   protected:
    virtual void ChildViewChanged() {
      View *child = GetChildView();
      if (child)
        caption_->GetTextFrame()->SetText(child->GetCaption());
    }

    virtual void DoLayout() {
      double width = GetWidth();
      double height = GetHeight();
      background_->SetPixelHeight(height - background_->GetPixelY() -
                                  bottom_->GetPixelHeight());
      close_button_->SetPixelX(width - close_button_->GetPixelWidth() -
                               kVDDetailsBorderWidth);
      caption_->SetPixelWidth(close_button_->GetPixelX() -
                              caption_->GetPixelX() - 1);

      if (remove_button_) {
        width -= (kVDDetailsBorderWidth + remove_button_->GetPixelWidth());
        remove_button_->SetPixelX(width);
        remove_button_->SetPixelY(height - kVDDetailsBorderWidth -
                                  remove_button_->GetPixelHeight());
      }
      if (negative_button_) {
        width -= (kVDDetailsBorderWidth + negative_button_->GetPixelWidth());
        negative_button_->SetPixelX(width);
        negative_button_->SetPixelY(height - kVDDetailsBorderWidth -
                                    negative_button_->GetPixelHeight());
      }
    }

    virtual void GetMargins(double *left, double *right,
                            double *top, double *bottom) {
      *left = kVDDetailsBorderWidth;
      *right = kVDDetailsBorderWidth;
      *top = background_->GetPixelY();
      *bottom = bottom_->GetPixelHeight();
    }

    virtual void GetMinimumClientExtents(double *width, double *height) {
      *width = 0;
      *height = 0;
      if (remove_button_)
        *width += remove_button_->GetPixelWidth();
      if (negative_button_)
        *width += negative_button_->GetPixelWidth();
      if (remove_button_ && negative_button_);
        *width += kVDDetailsBorderWidth;
    }

   private:
    void OnCloseButtonClicked() {
      PostSignal(&owner_->on_close_signal_);
    }

    void OnCaptionClicked() {
      flags_ = ViewInterface::DETAILS_VIEW_FLAG_TOOLBAR_OPEN;
      PostSignal(&owner_->on_close_signal_);
    }

    void OnRemoveButtonClicked() {
      flags_ = ViewInterface::DETAILS_VIEW_FLAG_REMOVE_BUTTON;
      PostSignal(&owner_->on_close_signal_);
    }

    void OnNegativeButtonClicked() {
      flags_ = ViewInterface::DETAILS_VIEW_FLAG_NEGATIVE_FEEDBACK;
      PostSignal(&owner_->on_close_signal_);
    }

   private:
    DecoratedViewHost::Impl *owner_;
    ViewInterface::HitTest hittest_;

    // Once added to this view, these are owned by the view.
    // Do not delete.
    ImgElement *background_;
    ImgElement *bottom_;

    // Close button.
    ButtonElement *close_button_;
    ButtonElement *remove_button_;
    ButtonElement *negative_button_;

    LabelElement *caption_;

    int flags_;
    Slot1<void, int> *feedback_handler_;
  };
  // End of DetailsViewDecorator

 public:
  Impl(DecoratedViewHost *owner,
       ViewHostInterface *view_host,
       DecoratedViewHost::DecoratorType decorator_type,
       bool transparent)
    : owner_(owner),
      view_decorator_(NULL),
      decorator_type_(decorator_type) {
    ASSERT(view_host);
    if (view_host->GetType() == ViewHostInterface::VIEW_HOST_MAIN) {
      if (decorator_type == DecoratedViewHost::MAIN_DOCKED ||
          decorator_type == DecoratedViewHost::MAIN_STANDALONE) {
        bool sidebar = (decorator_type == DecoratedViewHost::MAIN_DOCKED);
        view_decorator_ =
            new NormalMainViewDecorator(view_host, this, sidebar, transparent);
        view_decorator_->SetAllowXMargin(sidebar);
      } else if (decorator_type == DecoratedViewHost::MAIN_EXPANDED) {
        view_decorator_ = new ExpandedMainViewDecorator(view_host, this);
      }
    } else if (view_host->GetType() == ViewHostInterface::VIEW_HOST_DETAILS &&
               decorator_type == DecoratedViewHost::DETAILS) {
      view_decorator_ = new DetailsViewDecorator(view_host, this);
    }

    ASSERT(view_decorator_);
    if (!view_decorator_) {
      LOG("Type of ViewHost doesn't match with ViewDecorator type.");
      view_decorator_ = new ViewDecoratorBase(view_host, false, false);
    }
  }

  ~Impl() {
    delete view_decorator_;
  }

  DecoratedViewHost *owner_;
  ViewDecoratorBase *view_decorator_;
  DecoratorType decorator_type_;

  Signal0<void> on_dock_signal_;
  Signal0<void> on_undock_signal_;
  Signal0<void> on_popout_signal_;
  Signal0<void> on_popin_signal_;
  Signal0<void> on_close_signal_;;
};

const DecoratedViewHost::Impl::NormalMainViewDecorator::ButtonInfo
  DecoratedViewHost::Impl::NormalMainViewDecorator::kButtonsInfo[] = {
  {
    "VD_BACK_BUTTON_TOOLTIP",
    kVDButtonBackNormal,
    kVDButtonBackOver,
    kVDButtonBackDown,
    &DecoratedViewHost::Impl::NormalMainViewDecorator::OnBackButtonClicked
  },
  {
    "VD_FORWARD_BUTTON_TOOLTIP",
    kVDButtonForwardNormal,
    kVDButtonForwardOver,
    kVDButtonForwardDown,
    &DecoratedViewHost::Impl::NormalMainViewDecorator::OnForwardButtonClicked
  },
  {
    "VD_TOGGLE_EXPANDED_BUTTON_TOOLTIP",
    kVDButtonExpandNormal,
    kVDButtonExpandOver,
    kVDButtonExpandDown,
    &DecoratedViewHost::Impl::NormalMainViewDecorator::OnToggleExpandedButtonClicked,
  },
  {
    "VD_MENU_BUTTON_TOOLTIP",
    kVDButtonMenuNormal,
    kVDButtonMenuOver,
    kVDButtonMenuDown,
    &DecoratedViewHost::Impl::NormalMainViewDecorator::OnMenuButtonClicked,
  },
  {
    "VD_CLOSE_BUTTON_TOOLTIP",
    kVDButtonCloseNormal,
    kVDButtonCloseOver,
    kVDButtonCloseDown,
    &DecoratedViewHost::Impl::NormalMainViewDecorator::OnCloseButtonClicked,
  }
};


DecoratedViewHost::DecoratedViewHost(ViewHostInterface *view_host,
                                     DecoratorType decorator_type,
                                     bool transparent)
  : impl_(new Impl(this, view_host, decorator_type,  transparent)) {
}

DecoratedViewHost::~DecoratedViewHost() {
  delete impl_;
  impl_ = NULL;
}

DecoratedViewHost::DecoratorType DecoratedViewHost::GetDecoratorType() const {
  return impl_->decorator_type_;
}

ViewInterface *DecoratedViewHost::GetDecoratedView() const {
  return impl_->view_decorator_;
}

Connection *DecoratedViewHost::ConnectOnDock(Slot0<void> *slot) {
  return impl_->on_dock_signal_.Connect(slot);
}

Connection *DecoratedViewHost::ConnectOnUndock(Slot0<void> *slot) {
  return impl_->on_undock_signal_.Connect(slot);
}

Connection *DecoratedViewHost::ConnectOnPopOut(Slot0<void> *slot) {
  return impl_->on_popout_signal_.Connect(slot);
}

Connection *DecoratedViewHost::ConnectOnPopIn(Slot0<void> *slot) {
  return impl_->on_popin_signal_.Connect(slot);
}

Connection *DecoratedViewHost::ConnectOnClose(Slot0<void> *slot) {
  return impl_->on_close_signal_.Connect(slot);
}

ViewHostInterface::Type DecoratedViewHost::GetType() const {
  ViewHostInterface *view_host = impl_->view_decorator_->GetViewHost();
  return view_host ? view_host->GetType() : VIEW_HOST_MAIN;
}

void DecoratedViewHost::Destroy() {
  delete this;
}

void DecoratedViewHost::SetView(ViewInterface *view) {
  impl_->view_decorator_->SetChildView(down_cast<View *>(view));
}

ViewInterface *DecoratedViewHost::GetView() const {
  return impl_->view_decorator_->GetChildView();
}

GraphicsInterface *DecoratedViewHost::NewGraphics() const {
  ViewHostInterface *view_host = impl_->view_decorator_->GetViewHost();
  return view_host ? view_host->NewGraphics() : NULL;
}

void *DecoratedViewHost::GetNativeWidget() const {
  return impl_->view_decorator_->GetNativeWidget();
}

void DecoratedViewHost::ViewCoordToNativeWidgetCoord(
    double x, double y, double *widget_x, double *widget_y) const {
  double px, py;
  impl_->view_decorator_->GetViewElement()->ChildViewCoordToViewCoord(
      x, y, &px, &py);
  impl_->view_decorator_->ViewCoordToNativeWidgetCoord(px, py,
                                                       widget_x, widget_y);
}

void DecoratedViewHost::QueueDraw() {
  impl_->view_decorator_->GetViewElement()->QueueDraw();
}

void DecoratedViewHost::QueueResize() {
  impl_->view_decorator_->UpdateViewSize();
}

void DecoratedViewHost::EnableInputShapeMask(bool /* enable */) {
  // Do nothing.
}

void DecoratedViewHost::SetResizable(ViewInterface::ResizableMode mode) {
  impl_->view_decorator_->SetResizable(mode);
}

void DecoratedViewHost::SetCaption(const char *caption) {
  impl_->view_decorator_->SetCaption(caption);
}

void DecoratedViewHost::SetShowCaptionAlways(bool always) {
  impl_->view_decorator_->SetShowCaptionAlways(always);
}

void DecoratedViewHost::SetCursor(int type) {
  ViewHostInterface *view_host = impl_->view_decorator_->GetViewHost();
  if (view_host)
    view_host->SetCursor(type);
}

void DecoratedViewHost::SetTooltip(const char *tooltip) {
  impl_->view_decorator_->SetTooltip(tooltip);
}

bool DecoratedViewHost::ShowView(bool modal, int flags,
                              Slot1<void, int> *feedback_handler) {
  return impl_->view_decorator_->ShowDecoratedView(modal, flags,
                                                   feedback_handler);
}

void DecoratedViewHost::CloseView() {
  impl_->view_decorator_->CloseDecoratedView();
}

bool DecoratedViewHost::ShowContextMenu(int button) {
  ViewHostInterface *view_host = impl_->view_decorator_->GetViewHost();
  return view_host ? view_host->ShowContextMenu(button) : false;
}

void DecoratedViewHost::Alert(const char *message) {
  impl_->view_decorator_->Alert(message);
}

bool DecoratedViewHost::Confirm(const char *message) {
  return impl_->view_decorator_->Confirm(message);
}

std::string DecoratedViewHost::Prompt(const char *message,
                                   const char *default_value) {
  return impl_->view_decorator_->Prompt(message, default_value);
}

ViewInterface::DebugMode DecoratedViewHost::GetDebugMode() const {
  return impl_->view_decorator_->GetDebugMode();
}

void DecoratedViewHost::BeginResizeDrag(int button,
                                        ViewInterface::HitTest hittest) {
  ViewHostInterface *view_host = impl_->view_decorator_->GetViewHost();
  if (view_host)
    view_host->BeginResizeDrag(button, hittest);
}

void DecoratedViewHost::BeginMoveDrag(int button) {
  ViewHostInterface *view_host = impl_->view_decorator_->GetViewHost();
  if (view_host)
    view_host->BeginMoveDrag(button);
}

} // namespace ggadget
