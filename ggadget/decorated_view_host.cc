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
#include "options_interface.h"

namespace ggadget {

static const double kVDMainBorderWidth = 6;
static const double kVDMainSidebarBorderHeight = 3;
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
static const double kVDDetailsCaptionMargin = 1;

static const unsigned int kVDShowTimeout = 200;
static const unsigned int kVDHideTimeout = 500;

class DecoratedViewHost::Impl {
 public:
  // Base class for all kinds of view decorators.
  class ViewDecoratorBase : public View {
   public:
    ViewDecoratorBase(ViewHostInterface *host,
                      const char *option_prefix,
                      bool allow_x_margin,
                      bool allow_y_margin)
      : View(host, NULL, NULL, NULL),
        option_prefix_(option_prefix),
        allow_x_margin_(allow_x_margin),
        allow_y_margin_(allow_y_margin),
        on_mouse_event_(false),
        cursor_(CURSOR_DEFAULT),
        hittest_(HT_CLIENT),
        child_resizable_(ViewInterface::RESIZABLE_ZOOM),
        auto_restore_view_size_(true),
        child_view_(NULL),
        view_element_(new ViewElement(NULL, this, NULL, false)) {
      view_element_->SetVisible(true);
      GetChildren()->InsertElement(view_element_, NULL);
      view_element_->ConnectOnSizeEvent(
          NewSlot(this, &ViewDecoratorBase::UpdateViewSize));
      // Always resizable.
      View::SetResizable(RESIZABLE_TRUE);
      View::EnableCanvasCache(false);
    }
    virtual ~ViewDecoratorBase() {}

    ViewElement *GetViewElement() {
      return view_element_;
    }

    void SetChildView(View *child_view) {
      if (child_view_ != child_view) {
        SaveViewStates();
        child_view_ = child_view;
        view_element_->SetChildView(child_view);

        if (child_view_) {
          child_resizable_ = child_view_->GetResizable();

          // Only restore view states if the view has been initialized.
          if (child_view_->GetWidth() > 0 && child_view_->GetHeight() > 0)
            RestoreViewStates();
        }
        ChildViewChanged();
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

    void SetDecoratorHitTest(HitTest hittest) {
      hittest_ = hittest;
    }

    ViewInterface::ResizableMode GetChildResizable() const {
      return child_resizable_;
    }

    void EnableAutoRestoreViewSize(bool enable) {
      auto_restore_view_size_ = enable;
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

      cw = std::max(*width - left - right, cw);
      ch = std::max(*height - top - bottom, ch);
      if (IsChildViewVisible()) {
        view_element_->OnSizing(&cw, &ch);
      } else {
        OnClientSizing(&cw, &ch);
      }
      cw += (left + right);
      ch += (top + bottom);

      if (!allow_x_margin_)
        *width = cw;
      if (!allow_y_margin_)
        *height = ch;

      // Always returns true here, SetSize() can handle the case when
      // view element is OnSizing() returns false.
      return true;
    }

    virtual void SetResizable(ResizableMode resizable) {
      if (child_resizable_ != resizable) {
        // Reset the zoom factor to 1 if the child view is changed to
        // resizable.
        if (child_resizable_ != ViewInterface::RESIZABLE_TRUE &&
            resizable == ViewInterface::RESIZABLE_TRUE) {
          view_element_->SetScale(1);
        }
        child_resizable_= resizable;
        UpdateViewSize();
      }
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
        if (view_element_->OnSizing(&vw, &vh))
          view_element_->SetSize(vw, vh);

        cw = std::max(view_element_->GetPixelWidth(), cw);
        ch = std::max(view_element_->GetPixelHeight(), ch);
      } else {
        cw = std::max(width - left - right, cw);
        ch = std::max(height - top - bottom, ch);
      }

      cw += (left + right);
      ch += (top + bottom);
      // Call SetViewSize directly here to make sure that
      // allow_x_margin_ and allow_y_margin_ can take effect.
      if (SetViewSize(width, height, cw, ch))
        Layout();
    }

    virtual HitTest GetHitTest() const {
      return hittest_ == HT_CLIENT ? View::GetHitTest() : hittest_;
    }

    virtual void SetCursor(int type) {
      // If it's currently handling a mouse event, then just caches the cursor
      // type, and it'll be set after handling the mouse event.
      // It can help avoid cursor flicker.
      if (on_mouse_event_)
        cursor_ = type;
      else
        View::SetCursor(type);
    }

   public:
    virtual EventResult OnMouseEvent(const MouseEvent &event) {
      on_mouse_event_ = true;
      cursor_ = CURSOR_DEFAULT;
      hittest_ = HT_CLIENT;

      EventResult result1 = View::OnMouseEvent(event);
      EventResult result2 = EVENT_RESULT_UNHANDLED;

      if (result1 == EVENT_RESULT_UNHANDLED ||
          event.GetType() == Event::EVENT_MOUSE_OVER ||
          event.GetType() == Event::EVENT_MOUSE_OUT) {
        HandleMouseEvent(event);
      }

      View::SetCursor(cursor_);
      on_mouse_event_ = false;
      return std::max(result1, result2);
    }

   public:
    virtual bool ShowDecoratedView(bool modal, int flags,
                                   Slot1<void, int> *feedback_handler) {
      RestoreViewStates();
      // Nothing else. Derived class shall override this method to do more
      // things.
      return ShowView(modal, flags, feedback_handler);
    }

    virtual void CloseDecoratedView() {
      // Derived class shall override this method to do more things.
      CloseView();
    }

    virtual void SetDockEdge(bool) {
      // Only valid for NormalMainViewDecorator with MAIN_DOCKED type.
    }

    virtual bool IsMinimized() {
      // Only valid for NormalMainViewDecorator.
      return false;
    }

    virtual void SetMinimized(bool minimized) {
      // Only valid for NormalMainViewDecorator.
    }

    virtual void SaveViewStates() {
      if (!auto_restore_view_size_)
        return;
      View *child = GetChildView();
      Gadget *gadget = child ? child->GetGadget() : NULL;
      if (gadget) {
        OptionsInterface *opt = gadget->GetOptions();
        ViewElement *elm = GetViewElement();
        std::string prefix(option_prefix_);
        opt->PutInternalValue((prefix + "_width").c_str(),
                              Variant(elm->GetPixelWidth()));
        opt->PutInternalValue((prefix + "_height").c_str(),
                              Variant(elm->GetPixelHeight()));
        opt->PutInternalValue((prefix + "_scale").c_str(),
                              Variant(elm->GetScale()));
        DLOG("SaveViewStates(%d): w:%.0lf h:%.0lf s: %.2lf",
             gadget->GetInstanceID(), elm->GetPixelWidth(),
             elm->GetPixelHeight(), elm->GetScale());
      }
    }

    virtual void RestoreViewStates() {
      if (!auto_restore_view_size_) {
        UpdateViewSize();
        return;
      }

      View *child = GetChildView();
      Gadget *gadget = child ? child->GetGadget() : NULL;
      // Only load view states when the original size has been saved.
      if (gadget) {
        OptionsInterface *opt = gadget->GetOptions();
        ViewElement *elm = GetViewElement();
        std::string prefix(option_prefix_);
        Variant vw = opt->GetInternalValue((prefix + "_width").c_str());
        Variant vh = opt->GetInternalValue((prefix + "_height").c_str());
        Variant vs = opt->GetInternalValue((prefix + "_scale").c_str());
        if (vs.type() == Variant::TYPE_DOUBLE) {
          elm->SetScale(VariantValue<double>()(vs));
        } else {
          elm->SetScale(1);
        }
        if (GetChildResizable() == ViewInterface::RESIZABLE_TRUE) {
          double width, height;
          if (vw.type() == Variant::TYPE_DOUBLE &&
              vh.type() == Variant::TYPE_DOUBLE) {
            width = VariantValue<double>()(vw);
            height = VariantValue<double>()(vh);
          } else {
            // Restore to default size if there is no size information saved.
            GetChildView()->GetDefaultSize(&width, &height);
          }
          if (elm->OnSizing(&width, &height))
            elm->SetSize(width, height);
        }
        DLOG("RestoreViewStates(%d): w:%.0lf h:%.0lf s: %.2lf",
             gadget->GetInstanceID(), elm->GetPixelWidth(),
             elm->GetPixelHeight(), elm->GetScale());
        UpdateViewSize();
      }
    }
   protected:
    // To be implemented by derived classes to do additional mouse event
    // handling.
    virtual EventResult HandleMouseEvent(const MouseEvent &event) {
      return EVENT_RESULT_UNHANDLED;
    }

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
      if (!allow_x_margin_)
        req_w = min_w;
      if (!allow_y_margin_)
        req_h = min_h;

      if (req_w != GetWidth() || req_h != GetHeight()) {
        DLOG("DecoratedView::SetViewSize(%lf, %lf)", req_w, req_h);
        View::SetSize(req_w, req_h);
        return true;
      }
      return false;
    }

   private:
    const char *option_prefix_;

    bool allow_x_margin_;
    bool allow_y_margin_;

    bool on_mouse_event_;
    int cursor_;
    HitTest hittest_;
    ViewInterface::ResizableMode child_resizable_;
    bool auto_restore_view_size_;

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
      : ViewDecoratorBase(view_host,
                          sidebar ? "main_view_docked" : "main_view_standalone",
                          sidebar, false),
        owner_(owner),
        sidebar_(sidebar),
        dock_right_(true),
        transparent_(transparent),
        minimized_(false),
        popped_out_(false),
        mouseover_(false),
        minimized_state_loaded_(false),
        update_visibility_timer_(0),
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
        background_->SetVisible(!transparent_);
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
        bottom_->SetStretchMiddle(true);
      }
      bottom_->SetVisible(false);
      GetChildren()->InsertElement(bottom_, NULL);

      double minimized_top = sidebar_ ? kVDMainSidebarBorderHeight :
                              (!transparent_ ? kVDMainBorderWidth :
                                kVDMainToolbarHeight + kVDMainBorderWidth);
      if (transparent_) {
        minimized_bkgnd_= new ImgElement(NULL, this, NULL);
        minimized_bkgnd_->SetSrc(Variant(kVDMainBackgroundMinimized));
        minimized_bkgnd_->SetStretchMiddle(true);
        minimized_bkgnd_->SetPixelHeight(kVDMainMinimizedHeight);
        minimized_bkgnd_->SetPixelX(sidebar_ ? 0 : kVDMainBorderWidth);
        minimized_bkgnd_->SetPixelY(minimized_top);
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
      icon_->SetPixelY(minimized_top + kVDMainMinimizedHeight * 0.5);
      icon_->SetVisible(false);
      icon_->SetEnabled(true);
      icon_->ConnectOnClickEvent(
          NewSlot(this, &NormalMainViewDecorator::OnToggleExpandedButtonClicked));
      GetChildren()->InsertElement(icon_, NULL);

      caption_ = new LabelElement(NULL, this, NULL);
      caption_->GetTextFrame()->SetSize(10);
      caption_->GetTextFrame()->SetColor(Color::kWhite, 1);
      caption_->GetTextFrame()->SetWordWrap(false);
      caption_->GetTextFrame()->SetTrimming(
          CanvasInterface::TRIMMING_CHARACTER_ELLIPSIS);
      caption_->SetPixelHeight(kVDMainMinimizedHeight -
                               kVDMainCaptionMarginV * 2);
      caption_->SetPixelY(minimized_top + kVDMainCaptionMarginV);
      caption_->SetVisible(false);
      caption_->SetEnabled(true);
      caption_->ConnectOnClickEvent(
          NewSlot(this, &NormalMainViewDecorator::OnToggleExpandedButtonClicked));
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
      UpdateToggleExpandedButton();
      LayoutButtons();
    }

    ~NormalMainViewDecorator() {
      if (update_visibility_timer_)
        ClearTimeout(update_visibility_timer_);
      if (plugin_flags_connection_)
        plugin_flags_connection_->Disconnect();
    }

   public:
    virtual Gadget *GetGadget() const {
      if (popped_out_ && original_child_view_)
        return original_child_view_->GetGadget();

      return ViewDecoratorBase::GetGadget();
    }

    virtual bool OnAddContextMenuItems(MenuInterface *menu) {
      static const struct {
        const char *label;
        double zoom;
      } kZoomMenuItems[] = {
        { "MENU_ITEM_AUTO_FIT", 0 },
        { "MENU_ITEM_50P", 0.5 },
        { "MENU_ITEM_75P", 0.75 },
        { "MENU_ITEM_100P", 1.0 },
        { "MENU_ITEM_125P", 1.25 },
        { "MENU_ITEM_150P", 1.50 },
        { "MENU_ITEM_175P", 1.75 },
        { "MENU_ITEM_200P", 2.0 },
      };
      static const int kNumZoomMenuItems = 8;

      bool result = false;
      View *child = GetChildView();
      if (child)
        result = child->OnAddContextMenuItems(menu);
      else if (original_child_view_)
        result = original_child_view_->OnAddContextMenuItems(menu);

      if (result) {
        int priority = MenuInterface::MENU_ITEM_PRI_DECORATOR;

        menu->AddItem(
          GM_(minimized_ ? "MENU_ITEM_EXPAND" : "MENU_ITEM_COLLAPSE"), 0,
          NewSlot(this, &NormalMainViewDecorator::CollapseExpandMenuCallback),
          priority);

        if (owner_->on_undock_signal_.HasActiveConnections() && sidebar_) {
          menu->AddItem(GM_("MENU_ITEM_UNDOCK_FROM_SIDEBAR"), 0,
            NewSlot(this, &NormalMainViewDecorator::UndockMenuCallback),
            priority);
        } else if (owner_->on_dock_signal_.HasActiveConnections() &&
                   !sidebar_) {
          menu->AddItem(GM_("MENU_ITEM_DOCK_TO_SIDEBAR"), 0,
            NewSlot(this, &NormalMainViewDecorator::DockMenuCallback),
            priority);
        }

        if (!sidebar_ && !minimized_ && !popped_out_) {
          double scale = GetViewElement()->GetScale();
          int flags[kNumZoomMenuItems];
          bool has_checked = false;

          for (int i = 0; i < kNumZoomMenuItems; ++i) {
            flags[i] = 0;
            if (kZoomMenuItems[i].zoom == scale) {
              flags[i] = MenuInterface::MENU_ITEM_FLAG_CHECKED;
              has_checked = true;
            }
          }

          // Check "Auto Fit" item if the current scale doesn't match with any
          // other menu items.
          if (!has_checked)
            flags[0] = MenuInterface::MENU_ITEM_FLAG_CHECKED;

          MenuInterface *zoom = menu->AddPopup(GM_("MENU_ITEM_ZOOM"), priority);

          for (int i = 0; i < kNumZoomMenuItems; ++i) {
            zoom->AddItem(
                GM_(kZoomMenuItems[i].label), flags[i],
                NewSlot(this, &NormalMainViewDecorator::OnZoomMenuCallback,
                        kZoomMenuItems[i].zoom),
                priority);
          }
        }
      }

      return result;
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

    virtual void SetDockEdge(bool right) {
      if (dock_right_ != right) {
        dock_right_ = right;
        UpdateToggleExpandedButton();
      }
    }

    virtual bool IsMinimized() {
      return minimized_;
    }

    virtual void SetMinimized(bool minimized) {
      if (minimized_ != minimized)
        CollapseExpandMenuCallback(NULL);
    }

    virtual void SaveViewStates() {
      Gadget *gadget = GetGadget();
      if (gadget) {
        OptionsInterface *opt = gadget->GetOptions();
        opt->PutInternalValue("main_view_minimized", Variant(minimized_));
        DLOG("SaveViewStates(%d): main view minimized: %s",
             gadget->GetInstanceID(), minimized_ ? "true" : "false");
      }
      ViewDecoratorBase::SaveViewStates();
    }

    virtual void RestoreViewStates() {
      ViewDecoratorBase::RestoreViewStates();
      Gadget *gadget = GetGadget();
      // Only load view states when the original size has been saved.
      if (gadget && !minimized_state_loaded_) {
        OptionsInterface *opt = gadget->GetOptions();
        Variant vm = opt->GetInternalValue("main_view_minimized");
        if (vm.type() == Variant::TYPE_BOOL &&
           minimized_ != VariantValue<bool>()(vm)) {
          CollapseExpandMenuCallback(NULL);
        }
        minimized_state_loaded_ = true;
      }
    }

   protected:
    virtual EventResult HandleMouseEvent(const MouseEvent &event) {
      Event::Type t = event.GetType();
      if (t == Event::EVENT_MOUSE_OVER || t == Event::EVENT_MOUSE_OUT) {
        mouseover_ = (t == Event::EVENT_MOUSE_OVER);
        if (sidebar_) {
          // Show/hide decorator immediately in sidebar mode.
          UpdateVisibility();
        } else if (!update_visibility_timer_) {
          update_visibility_timer_ = SetTimeout(
              NewSlot(this, &NormalMainViewDecorator::UpdateVisibility),
              mouseover_ ? kVDShowTimeout : kVDHideTimeout);
        }
      } else if (mouseover_) {
        bool h_resizable = false;
        bool v_resizable = false;
        if (minimized_) {
          h_resizable = true;
        } else if (GetChildResizable() == RESIZABLE_TRUE) {
          h_resizable = true;
          v_resizable = true;
        }

        double x = event.GetX();
        double y = event.GetY();
        double w = GetWidth();
        double h = GetHeight();
        double top = transparent_ ? kVDMainToolbarHeight : 0;

        if (!sidebar_) {
          // Only show bottom right corner when there is no transparent
          // background or the child view is not resizable.
          if ((GetChildResizable() != RESIZABLE_TRUE && !minimized_) ||
              (!transparent_ && h_resizable && v_resizable)) {
            if (x > w - kVDMainCornerSize && y > h - kVDMainCornerSize)
              bottom_->SetVisible(true);
            else
              bottom_->SetVisible(false);
          } else if (x >= w - kVDMainBorderWidth * 2 &&
                     y >= h - kVDMainBorderWidth * 2 &&
                     h_resizable && v_resizable) {
            SetDecoratorHitTest(HT_BOTTOMRIGHT);
            SetCursor(CURSOR_SIZENWSE);
          } else if (x >= w - kVDMainBorderWidth * 2 &&
                     y >= top && y <= top + kVDMainBorderWidth * 2 &&
                     h_resizable && v_resizable) {
            SetDecoratorHitTest(HT_TOPRIGHT);
            SetCursor(CURSOR_SIZENESW);
          } else if (x <= kVDMainBorderWidth * 2 &&
                     y >= top && y <= top + kVDMainBorderWidth * 2 &&
                     h_resizable && v_resizable) {
            SetDecoratorHitTest(HT_TOPLEFT);
            SetCursor(CURSOR_SIZENWSE);
          } else if (x <= kVDMainBorderWidth  * 2 &&
                     y >= h - kVDMainBorderWidth * 2 &&
                     h_resizable && v_resizable) {
            SetDecoratorHitTest(HT_BOTTOMLEFT);
            SetCursor(CURSOR_SIZENESW);
          } else if (x >= w - kVDMainBorderWidth && y >= top && h_resizable) {
            SetDecoratorHitTest(HT_RIGHT);
            SetCursor(CURSOR_SIZEWE);
          } else if (x <= kVDMainBorderWidth && y >= top && h_resizable) {
            SetDecoratorHitTest(HT_LEFT);
            SetCursor(CURSOR_SIZEWE);
          } else if (y >= h - kVDMainBorderWidth && v_resizable) {
            SetDecoratorHitTest(HT_BOTTOM);
            SetCursor(CURSOR_SIZENS);
          } else if (y >= top && y <= top + kVDMainBorderWidth &&
                     v_resizable) {
            SetDecoratorHitTest(HT_TOP);
            SetCursor(CURSOR_SIZENS);
          }
        } else if (y >= h - kVDMainBorderWidth && !minimized_) {
          SetDecoratorHitTest(HT_BOTTOM);
          SetCursor(CURSOR_SIZENS);
        }
      }

      return EVENT_RESULT_UNHANDLED;
    }

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
        caption_->GetTextFrame()->SetText(child->GetCaption());
        if (minimized_) {
          SimpleEvent event(Event::EVENT_MINIMIZE);
          child->OnOtherEvent(event);
        }
      }

      DoLayout();
      LayoutButtons();
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

      if (sidebar_) {
        if (minimized_)
          *top = kVDMainSidebarBorderHeight;
        *bottom = kVDMainSidebarBorderHeight;
      } else if (GetChildResizable() == RESIZABLE_TRUE || minimized_) {
        *left = kVDMainBorderWidth;
        *right = kVDMainBorderWidth;
        *bottom = kVDMainBorderWidth;
        if (transparent_)
          *top += kVDMainBorderWidth;
        else
          *top = kVDMainBorderWidth;
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
            background_->SetVisible(GetChildResizable() == RESIZABLE_TRUE ||
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
      ButtonElement *toggle_btn =
          down_cast<ButtonElement *>(elements->GetItemByIndex(TOGGLE_EXPANDED));

      toggle_btn->SetVisible(owner_->on_popin_signal_.HasActiveConnections() &&
                             owner_->on_popout_signal_.HasActiveConnections());

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
      bool unexpand = (dock_right_ ? popped_out_ : !popped_out_);
      Elements *elements = buttons_div_->GetChildren();
      ButtonElement *btn =
          down_cast<ButtonElement *>(elements->GetItemByIndex(TOGGLE_EXPANDED));
      btn->SetImage(Variant(
          unexpand ? kVDButtonUnexpandNormal : kVDButtonExpandNormal));
      btn->SetOverImage(Variant(
          unexpand ? kVDButtonUnexpandOver : kVDButtonExpandOver));
      btn->SetDownImage(Variant(
          unexpand ? kVDButtonUnexpandDown : kVDButtonExpandDown));
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

    void OnZoomMenuCallback(const char *, double zoom) {
      SetChildViewScale(zoom == 0 ? 1.0 : zoom);
    }

   private:
    DecoratedViewHost::Impl *owner_;

    bool sidebar_;
    bool dock_right_;
    bool transparent_;

    bool minimized_;
    bool popped_out_;
    bool mouseover_;
    bool minimized_state_loaded_;

    int update_visibility_timer_;

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
      : ViewDecoratorBase(view_host, "main_view_expanded", false, false),
        owner_(owner),
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
    virtual void SetCaption(const char *caption) {
      caption_->GetTextFrame()->SetText(caption);
      ViewDecoratorBase::SetCaption(caption);
    }

   public:
    virtual bool ShowDecoratedView(bool modal, int flags,
                                   Slot1<void, int> *handler) {
      return ViewDecoratorBase::ShowDecoratedView(modal, flags, handler);
    }

   protected:
    virtual EventResult HandleMouseEvent(const MouseEvent &event) {
      Event::Type t = event.GetType();
      if (t != Event::EVENT_MOUSE_OUT) {
        double x = event.GetX();
        double y = event.GetY();
        double w = GetWidth();
        double h = GetHeight();

        View *child = GetChildView();
        bool resizable =
            child ? (child->GetResizable() == RESIZABLE_TRUE) : false;

        if (resizable) {
          if (x >= w - kVDExpandedBorderWidth &&
              y >= h - kVDExpandedBorderWidth) {
            SetDecoratorHitTest(HT_BOTTOMRIGHT);
            SetCursor(CURSOR_SIZENWSE);
          } else if (x >= w - kVDExpandedBorderWidth &&
                     y <= kVDExpandedBorderWidth) {
            SetDecoratorHitTest(HT_TOPRIGHT);
            SetCursor(CURSOR_SIZENESW);
          } else if (x <= kVDExpandedBorderWidth &&
                     y <= kVDExpandedBorderWidth) {
            SetDecoratorHitTest(HT_TOPLEFT);
            SetCursor(CURSOR_SIZENWSE);
          } else if (x <= kVDExpandedBorderWidth &&
                     y >= h - kVDExpandedBorderWidth) {
            SetDecoratorHitTest(HT_BOTTOMLEFT);
            SetCursor(CURSOR_SIZENESW);
          } else if (x >= w - kVDExpandedBorderWidth) {
            SetDecoratorHitTest(HT_RIGHT);
            SetCursor(CURSOR_SIZEWE);
          } else if (x <= kVDExpandedBorderWidth) {
            SetDecoratorHitTest(HT_LEFT);
            SetCursor(CURSOR_SIZEWE);
          } else if (y >= h - kVDExpandedBorderWidth) {
            SetDecoratorHitTest(HT_BOTTOM);
            SetCursor(CURSOR_SIZENS);
          } else if (y <= kVDExpandedBorderWidth) {
            SetDecoratorHitTest(HT_TOP);
            SetCursor(CURSOR_SIZENS);
          }
        }
      }

      return EVENT_RESULT_UNHANDLED;
    }

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
      : ViewDecoratorBase(view_host, "details_view", false, false),
        owner_(owner),
        background_(NULL),
        top_(NULL),
        bottom_(NULL),
        close_button_(NULL),
        remove_button_(NULL),
        negative_button_(NULL),
        caption_(NULL),
        flags_(0),
        feedback_handler_(NULL) {
      top_ = new ImgElement(NULL, this, NULL);
      top_->SetSrc(Variant(kVDDetailsTop));
      top_->SetStretchMiddle(true);
      top_->SetPixelX(0);
      top_->SetPixelY(0);
      top_->SetRelativeWidth(1);
      GetChildren()->InsertElement(top_, GetViewElement());

      background_ = new ImgElement(NULL, this, NULL);
      background_->SetStretchMiddle(true);
      background_->SetPixelX(0);
      background_->SetPixelY(top_->GetSrcHeight());
      background_->SetRelativeWidth(1);
      background_->EnableCanvasCache(true);
      GetChildren()->InsertElement(background_, GetViewElement());

      caption_ = new LabelElement(NULL, this, NULL);
      caption_->GetTextFrame()->SetSize(10);
      caption_->GetTextFrame()->SetColor(Color::kBlack, 1);
      caption_->GetTextFrame()->SetWordWrap(true);
      caption_->GetTextFrame()->SetTrimming(
          CanvasInterface::TRIMMING_CHARACTER_ELLIPSIS);
      caption_->SetPixelX(kVDDetailsBorderWidth + kVDDetailsCaptionMargin);
      caption_->SetPixelY(kVDDetailsBorderWidth + kVDDetailsCaptionMargin);
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
        remove_button_->SetIconImage(Variant(kVDDetailsButtonNegfbNormal));
        remove_button_->SetIconPosition(ButtonElement::ICON_RIGHT);
        double tw, th;
        remove_button_->GetTextFrame()->GetSimpleExtents(&tw, &th);
        remove_button_->ConnectOnClickEvent(
            NewSlot(this, &DetailsViewDecorator::OnRemoveButtonClicked));
        remove_button_->ConnectOnMouseOverEvent(
            NewSlot(this, &DetailsViewDecorator::OnRemoveButtonMouseOver));
        remove_button_->ConnectOnMouseOutEvent(
            NewSlot(this, &DetailsViewDecorator::OnRemoveButtonMouseOut));
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
        negative_button_->ConnectOnClickEvent(
            NewSlot(this, &DetailsViewDecorator::OnNegativeButtonClicked));
        GetChildren()->InsertElement(negative_button_, NULL);
      }

      if (remove_button_ || negative_button_) {
        bottom_ = new ImgElement(NULL, this, NULL);
        bottom_->SetSrc(Variant(kVDDetailsBottom));
        bottom_->SetStretchMiddle(true);
        bottom_->SetPixelX(0);
        bottom_->SetRelativeY(1);
        bottom_->SetRelativePinY(1);
        bottom_->SetRelativeWidth(1);
        GetChildren()->InsertElement(bottom_, GetViewElement());
        background_->SetSrc(Variant(kVDDetailsMiddle));
      } else {
        // If there is no bottom buttons, then just use the normal background
        // image.
        background_->SetSrc(Variant(kVDDetailsBackground));
      }

      DoLayout();
      return ShowView(modal, 0, NULL);
    }

    virtual void CloseDecoratedView() {
      if (feedback_handler_) {
        // To make sure that openUrl can work.
        bool old_interaction = false;
        Gadget *gadget = GetGadget();
        if (gadget)
          old_interaction = gadget->SetInUserInteraction(true);

        (*feedback_handler_)(flags_);
        delete feedback_handler_;
        feedback_handler_ = NULL;

        if (gadget)
          gadget->SetInUserInteraction(old_interaction);
      }
      ViewDecoratorBase::CloseDecoratedView();
    }

   protected:
    virtual EventResult HandleMouseEvent(const MouseEvent &event) {
      Event::Type t = event.GetType();
      if (t != Event::EVENT_MOUSE_OUT) {
        double x = event.GetX();
        double y = event.GetY();
        double w = GetWidth();
        double h = GetHeight();

        View *child = GetChildView();
        bool resizable =
            child ? (child->GetResizable() == RESIZABLE_TRUE) : false;

        if (resizable) {
          if (x >= w - kVDDetailsBorderWidth &&
              y >= h - kVDDetailsBorderWidth) {
            SetDecoratorHitTest(HT_BOTTOMRIGHT);
            SetCursor(CURSOR_SIZENWSE);
          } else if (x >= w - kVDDetailsBorderWidth &&
                     y <= kVDDetailsBorderWidth) {
            SetDecoratorHitTest(HT_TOPRIGHT);
            SetCursor(CURSOR_SIZENESW);
          } else if (x <= kVDDetailsBorderWidth &&
                     y <= kVDDetailsBorderWidth) {
            SetDecoratorHitTest(HT_TOPLEFT);
            SetCursor(CURSOR_SIZENWSE);
          } else if (x <= kVDDetailsBorderWidth &&
                     y >= h - kVDDetailsBorderWidth) {
            SetDecoratorHitTest(HT_BOTTOMLEFT);
            SetCursor(CURSOR_SIZENESW);
          } else if (x >= w - kVDDetailsBorderWidth) {
            SetDecoratorHitTest(HT_RIGHT);
            SetCursor(CURSOR_SIZEWE);
          } else if (x <= kVDDetailsBorderWidth) {
            SetDecoratorHitTest(HT_LEFT);
            SetCursor(CURSOR_SIZEWE);
          } else if (y >= h - kVDDetailsBorderWidth) {
            SetDecoratorHitTest(HT_BOTTOM);
            SetCursor(CURSOR_SIZENS);
          } else if (y <= kVDDetailsBorderWidth) {
            SetDecoratorHitTest(HT_TOP);
            SetCursor(CURSOR_SIZENS);
          }
        }
      }

      return EVENT_RESULT_UNHANDLED;
    }

    virtual void ChildViewChanged() {
      View *child = GetChildView();
      if (child)
        caption_->GetTextFrame()->SetText(child->GetCaption());
    }

    virtual void DoLayout() {
      double width = GetWidth();
      double height = GetHeight();
      double caption_width, caption_height;
      double top_height;

      close_button_->Layout();
      close_button_->SetPixelX(width - close_button_->GetPixelWidth() -
                               kVDDetailsBorderWidth);

      caption_width = close_button_->GetPixelX() - caption_->GetPixelX() -
                      kVDDetailsCaptionMargin;

      caption_->SetPixelWidth(caption_width);
      caption_->GetTextFrame()->GetExtents(caption_width,
                                           &caption_width, &caption_height);
      top_height = top_->GetSrcHeight();

      // Only allow displaying two lines of caption.
      if (caption_height > top_height - kVDDetailsBorderWidth -
          kVDDetailsCaptionMargin * 2) {
        caption_->GetTextFrame()->GetSimpleExtents(&caption_width,
                                                   &caption_height);
        caption_height = std::min(caption_height * 2, top_height * 2);
        top_height = caption_height + kVDDetailsBorderWidth +
            kVDDetailsCaptionMargin * 2 + 1;
      }

      caption_->SetPixelHeight(caption_height);
      top_->SetPixelHeight(top_height);

      background_->SetPixelY(top_height);
      background_->SetPixelHeight(height - top_height -
                                  (bottom_ ? bottom_->GetPixelHeight() : 0));
      if (remove_button_) {
        remove_button_->Layout();
        width -= (kVDDetailsBorderWidth + remove_button_->GetPixelWidth());
        remove_button_->SetPixelX(width);
        remove_button_->SetPixelY(height - kVDDetailsBorderWidth -
                                  remove_button_->GetPixelHeight());
      }
      if (negative_button_) {
        negative_button_->Layout();
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
      *bottom = (bottom_ ? bottom_->GetPixelHeight() : kVDDetailsBorderWidth);
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

    void OnRemoveButtonMouseOver() {
      remove_button_->SetIconImage(Variant(kVDDetailsButtonNegfbOver));
    }

    void OnRemoveButtonMouseOut() {
      remove_button_->SetIconImage(Variant(kVDDetailsButtonNegfbNormal));
    }

    void OnNegativeButtonClicked() {
      flags_ = ViewInterface::DETAILS_VIEW_FLAG_NEGATIVE_FEEDBACK;
      PostSignal(&owner_->on_close_signal_);
    }

   private:
    DecoratedViewHost::Impl *owner_;

    // Once added to this view, these are owned by the view.
    // Do not delete.
    ImgElement *background_;
    ImgElement *top_;
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
        bool sidebar = (decorator_type != DecoratedViewHost::MAIN_STANDALONE);
        view_decorator_ = new NormalMainViewDecorator(view_host, this,
                                                      sidebar, transparent);
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
      view_decorator_ = new ViewDecoratorBase(view_host, "unknown_view",
                                              false, false);
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

void DecoratedViewHost::SetDockEdge(bool right) {
  impl_->view_decorator_->SetDockEdge(right);
}

bool DecoratedViewHost::IsMinimized() const {
  return impl_->view_decorator_->IsMinimized();
}

void DecoratedViewHost::SetMinimized(bool minimized) {
  impl_->view_decorator_->SetMinimized(minimized);
}

void DecoratedViewHost::RestoreViewSize() {
  // ViewDecoratorBase::RestoreViewStates() only restores view's size state.
  impl_->view_decorator_->ViewDecoratorBase::RestoreViewStates();
}

void DecoratedViewHost::EnableAutoRestoreViewSize(bool enable) {
  impl_->view_decorator_->EnableAutoRestoreViewSize(enable);
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

void DecoratedViewHost::NativeWidgetCoordToViewCoord(
    double x, double y, double *view_x, double *view_y) const {
  double px, py;
  impl_->view_decorator_->NativeWidgetCoordToViewCoord(x, y, &px, &py);
  impl_->view_decorator_->GetViewElement()->ViewCoordToChildViewCoord(
      px, py, view_x, view_y);
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
  impl_->view_decorator_->SetCursor(type);
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

int DecoratedViewHost::GetDebugMode() const {
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
