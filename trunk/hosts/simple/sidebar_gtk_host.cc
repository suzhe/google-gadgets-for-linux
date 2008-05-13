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

#include "sidebar_gtk_host.h"

#include <gtk/gtk.h>
#include <string>
#include <map>
#include <ggadget/common.h>
#include <ggadget/decorated_view_host.h>
#include <ggadget/gadget.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/gadget_manager_interface.h>
#include <ggadget/gtk/single_view_host.h>
#include <ggadget/gtk/utilities.h>
#include <ggadget/locales.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/messages.h>
#include <ggadget/menu_interface.h>
#include <ggadget/logger.h>
#include <ggadget/options_interface.h>
#include <ggadget/script_runtime_manager.h>
#include <ggadget/sidebar.h>
#include <ggadget/view.h>
#include <ggadget/view_element.h>
#include <ggadget/file_manager_factory.h>
#include <ggadget/file_manager_interface.h>

#include "gadget_browser_host.h"

using namespace ggadget;
using namespace ggadget::gtk;

namespace hosts {
namespace gtk {

const char *kOptionName         = "sidebar-gtk-host";
const char *kOptionAutoHide     = "auto-hide";
const char *kOptionAlwaysOnTop  = "always-on-top";
const char *kOptionPosition     = "position";
const char *kOptionFontSize     = "font-size";
const char *kOptionWidth        = "width";
const char *kOptionMonitor      = "monitor";

const int kDefaultFontSize     = 14;
const int kDefaultSidebarWidth = 200;
const int kDefaultMonitor      = 0;

enum SideBarPosition {
  SIDEBAR_POSITION_NONE,
  SIDEBAR_POSITION_LEFT,
  SIDEBAR_POSITION_RIGHT,
};

class SidebarGtkHost::Impl {
 public:
  class GadgetMoveClosure {
   public:
    GadgetMoveClosure(SidebarGtkHost::Impl *owner,
                      SingleViewHost *outer_view_host,
                      DecoratedViewHost *decorator_view_host,
                      View *view,
                      double height)
        : owner_(owner),
          outer_view_host_(outer_view_host),
          decorator_view_host_(decorator_view_host),
          view_(view),
          sidebar_(NULL),
          height_(height) {
      sidebar_ = owner->view_host_->GetWindow();
      AddConnection(outer_view_host_->ConnectOnMoved(
            NewSlot(this, &GadgetMoveClosure::HandleMoved)));
      AddConnection(outer_view_host_->ConnectOnEndMoveDrag(
            NewSlot(this, &GadgetMoveClosure::HandleEndMoveDrag)));
      AddConnection(decorator_view_host_->ConnectOnDock(
            NewSlot(this, &GadgetMoveClosure::HandleDock)));
    }
    ~GadgetMoveClosure() {
      for (size_t i = 0; i < connections_.size(); ++i)
        connections_[i]->Disconnect();
    }
    void AddConnection(Connection *connection) {
      connections_.push_back(connection);
    }
    void HandleMoved(int x, int y) {
      int h;
      if (IsOverlapWithSideBar(&h, x, y)) {
        owner_->side_bar_->InsertNullElement(h, view_);
        height_ = h;
      } else {
        owner_->side_bar_->ClearNullElement();
      }
    }
    void HandleEndMoveDrag() {
      int h, x, y;
      outer_view_host_->GetWindowPosition(&x, &y);
      if (IsOverlapWithSideBar(&h, x, y)) {
        view_->GetGadget()->SetDisplayTarget(Gadget::TARGET_SIDEBAR);
        height_ = h;
        HandleDock();
      } else {
        decorator_view_host_->RestoreViewStates();
      }
      owner_->side_bar_->ClearNullElement();
    }
    void HandleDock() {
      owner_->Dock(view_, height_, true);
    }
   private:
    bool IsOverlapWithSideBar(int *height, int x, int y) {
      int w, h;
      outer_view_host_->GetWindowSize(&w, &h);
      int sx, sy, sw, sh;
      owner_->view_host_->GetWindowPosition(&sx, &sy);
      owner_->view_host_->GetWindowSize(&sw, &sh);
      if ((x + w >= sx) && (sx + sw >= x)) {
        if (height) {
          int dummy;
          gtk_widget_get_pointer(sidebar_, &dummy, height);
        }
        return true;
      }
      return false;
    }
    SidebarGtkHost::Impl *owner_;
    SingleViewHost *outer_view_host_;
    DecoratedViewHost *decorator_view_host_;
    View *view_;
    GtkWidget *sidebar_;
    double height_;
    std::vector<Connection *> connections_;
  };

  Impl(SidebarGtkHost *owner, bool decorated, int view_debug_mode)
    : gadget_browser_host_(owner, view_debug_mode),
      owner_(owner),
      decorated_(decorated),
      gadgets_shown_(true),
      view_debug_mode_(view_debug_mode),
      view_host_(NULL),
      expanded_original_(NULL),
      expanded_popout_(NULL),
      side_bar_(NULL),
      options_(GetGlobalOptions()),
      option_auto_hide_(false),
      option_always_on_top_(false),
      option_sidebar_position_(SIDEBAR_POSITION_RIGHT),
      option_sidebar_width_(kDefaultSidebarWidth),
      option_sidebar_monitor_(kDefaultMonitor),
      option_font_size_(kDefaultFontSize),
      net_wm_strut_(GDK_NONE),
      net_wm_strut_partial_(GDK_NONE),
      gadget_manager_(GetGadgetManager()) {
    ASSERT(gadget_manager_);
    ScriptRuntimeManager::get()->ConnectErrorReporter(
        NewSlot(this, &Impl::ReportScriptError));
    view_host_ = new SingleViewHost(ViewHostInterface::VIEW_HOST_MAIN, 1.0,
                                    decorated, false, false, view_debug_mode_);
    view_host_->ConnectOnBeginResizeDrag(
        NewSlot(this, &Impl::HandleSideBarBeginResizeDrag));
    view_host_->ConnectOnEndResizeDrag(
        NewSlot(this, &Impl::HandleSideBarEndResizeDrag));
    view_host_->ConnectOnBeginMoveDrag(
        NewSlot(this, &Impl::HandleSideBarBeginMoveDrag));
    view_host_->ConnectOnEndMoveDrag(
        NewSlot(this, &Impl::HandleSideBarEndMoveDrag));
    side_bar_ = new SideBar(owner_, view_host_);
    side_bar_->ConnectOnAddGadget(NewSlot(this, &Impl::HandleAddGadget));
    side_bar_->ConnectOnMenuOpen(NewSlot(this, &Impl::HandleMenuOpen));
    side_bar_->ConnectOnClose(NewSlot(this, &Impl::HandleClose));
    side_bar_->ConnectOnSizeEvent(NewSlot(this, &Impl::HanldeSizeEvent));
    side_bar_->ConnectOnUndock(NewSlot(this, &Impl::HandleUndock));
    side_bar_->ConnectOnPopIn(NewSlot(this, &Impl::HandleGeneralPopIn));

    LoadGlobalOptions();
  }

  ~Impl() {
    FlushGlobalOptions();

    for (GadgetsMap::iterator it = gadgets_.begin();
         it != gadgets_.end(); ++it)
      delete it->second;

    delete side_bar_;

#if GTK_CHECK_VERSION(2,10,0)
    g_object_unref(G_OBJECT(status_icon_));
#endif
  }

  void LoadGlobalOptions() {
    // in first time only save default valus
    if (!options_->GetCount()) {
      FlushGlobalOptions();
      return;
    }

    bool corrupt_data = false;
    Variant value;
    if (!options_->Exists(kOptionAutoHide)) {
      corrupt_data = true;
    } else {
      value = options_->GetValue(kOptionAutoHide);
      if (!value.ConvertToBool(&option_auto_hide_)) corrupt_data = true;
    }

    if (!options_->Exists(kOptionAlwaysOnTop)) {
      corrupt_data = true;
    } else {
      value = options_->GetValue(kOptionAlwaysOnTop);
      if (!value.ConvertToBool(&option_always_on_top_)) corrupt_data = true;
    }

    if (!options_->Exists(kOptionPosition)) {
      corrupt_data = true;
    } else {
      value = options_->GetValue(kOptionPosition);
      if (!value.ConvertToInt(&option_sidebar_position_)) corrupt_data = true;
    }

    if (!options_->Exists(kOptionWidth)) {
      corrupt_data = true;
    } else {
      value = options_->GetValue(kOptionWidth);
      if (!value.ConvertToInt(&option_sidebar_width_)) corrupt_data = true;
    }

    if (!options_->Exists(kOptionMonitor)) {
      corrupt_data = true;
    } else {
      value = options_->GetValue(kOptionMonitor);
      if (!value.ConvertToInt(&option_sidebar_monitor_)) corrupt_data = true;
    }

    if (!options_->Exists(kOptionFontSize)) {
      corrupt_data = true;
    } else {
      value = options_->GetValue(kOptionFontSize);
      if (!value.ConvertToInt(&option_font_size_)) corrupt_data = true;
    }

    if (corrupt_data) FlushGlobalOptions();
  }

  void FlushGlobalOptions() {
    options_->PutValue(kOptionAutoHide, Variant(option_auto_hide_));
    options_->PutValue(kOptionAlwaysOnTop, Variant(option_always_on_top_));
    options_->PutValue(kOptionPosition, Variant(option_sidebar_position_));
    options_->PutValue(kOptionWidth, Variant(option_sidebar_width_));
    options_->PutValue(kOptionMonitor, Variant(option_sidebar_monitor_));
    options_->PutValue(kOptionFontSize, Variant(option_font_size_));
    options_->Flush();
  }

  void SetupUI() {
    main_widget_ = view_host_->GetWindow();

#if GTK_CHECK_VERSION(2,10,0)
    g_signal_connect_after(G_OBJECT(main_widget_), "focus-out-event",
                           G_CALLBACK(HandleFocusOutEvent), this);
    std::string icon_data;
    if (GetGlobalFileManager()->ReadFile(kGadgetsIcon, &icon_data)) {
      GdkPixbuf *icon_pixbuf = LoadPixbufFromData(icon_data);
      status_icon_ = gtk_status_icon_new_from_pixbuf(icon_pixbuf);
      g_object_unref(icon_pixbuf);
    } else {
      DLOG("Failed to load Gadgets icon.");
      status_icon_ = gtk_status_icon_new_from_stock(GTK_STOCK_ABOUT);
    }
    g_signal_connect(G_OBJECT(status_icon_), "activate",
                     G_CALLBACK(ToggleAllGadgetsHandler), this);
    g_signal_connect(G_OBJECT(status_icon_), "popup-menu",
                     G_CALLBACK(StatusIconPopupMenuHandler), this);
#endif

#ifdef _DEBUG
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(main_widget_), FALSE);
#endif
    gtk_window_set_title(GTK_WINDOW(main_widget_), "Google Gadgets");
    gtk_widget_show(main_widget_);
    g_assert(GTK_WIDGET_REALIZED(main_widget_));
    AdjustSidebar();
  }

  bool ConfirmGadget(int id) {
    std::string path = gadget_manager_->GetGadgetInstancePath(id);
    std::string download_url, title, description;
    if (!gadget_manager_->GetGadgetInstanceInfo(id,
                                                GetSystemLocaleName().c_str(),
                                                NULL, &download_url,
                                                &title, &description))
      return false;

    GtkWidget *dialog = gtk_message_dialog_new(
        NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
        "%s\n\n%s\n%s\n\n%s%s",
        GM_("GADGET_CONFIRM_MESSAGE"), title.c_str(), download_url.c_str(),
        GM_("GADGET_DESCRIPTION"), description.c_str());

    GdkScreen *screen;
    gdk_display_get_pointer(gdk_display_get_default(), &screen,
                            NULL, NULL, NULL);
    gtk_window_set_screen(GTK_WINDOW(dialog), screen);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_title(GTK_WINDOW(dialog), GM_("GADGET_CONFIRM_TITLE"));
    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return result == GTK_RESPONSE_YES;
  }

  bool NewGadgetInstanceCallback(int id) {
    if (gadget_manager_->IsGadgetInstanceTrusted(id) ||
        ConfirmGadget(id)) {
      return AddGadgetInstanceCallback(id);
    }
    return false;
  }

  bool AddGadgetInstanceCallback(int id) {
    std::string options = gadget_manager_->GetGadgetInstanceOptionsName(id);
    std::string path = gadget_manager_->GetGadgetInstancePath(id);
    if (options.length() && path.length()) {
      bool result = LoadGadget(path.c_str(), options.c_str(), id);
      LOG("SidebarGtkHost: Load gadget %s, with option %s, %s",
          path.c_str(), options.c_str(), result ? "succeeded" : "failed");
    }
    return true;
  }

  void AdjustSidebar() {
    // got monitor information
    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(main_widget_));
    int monitor_number = gdk_screen_get_n_monitors(screen);
    if (option_sidebar_monitor_ >= monitor_number) {
      DLOG("want to put sidebar in %d monitor, but this screen(%p) has "
           "only %d monitor(s), put to last monitor.",
           option_sidebar_monitor_, screen, monitor_number);
      option_sidebar_monitor_ = monitor_number - 1;
    }
    GdkRectangle rect;
    gdk_screen_get_monitor_geometry(screen, option_sidebar_monitor_, &rect);
    DLOG("monitor %d's rect: %d %d %d %d", option_sidebar_monitor_,
         rect.x, rect.y, rect.width, rect.height);

    // adjust properties
    // FIXME: should set height to the most length except panel's height
    side_bar_->SetSize(option_sidebar_width_, rect.height);
    AdjustOnTopProperties(rect, monitor_number);
    AdjustPositionProperties(rect);
  }

  void AdjustPositionProperties(const GdkRectangle &rect) {
    if (option_sidebar_position_ == SIDEBAR_POSITION_LEFT) {
      DLOG("move sidebar to %d %d", rect.x, rect.y);
      view_host_->SetWindowPosition(rect.x, rect.y);
    } else if (option_sidebar_position_ == SIDEBAR_POSITION_RIGHT) {
      DLOG("move sidebar to %d %d",
           rect.x + rect.width - option_sidebar_width_, rect.y);
      view_host_->SetWindowPosition(
                      rect.x + rect.width - option_sidebar_width_, rect.y);
    } else {
      ASSERT(option_sidebar_position_ != SIDEBAR_POSITION_NONE);
    }
  }

  void AdjustOnTopProperties(const GdkRectangle &rect, int monitor_number) {
    gtk_window_set_keep_above(GTK_WINDOW(main_widget_), option_always_on_top_);

    // if sidebar is on the edge, do strut
    if (option_always_on_top_ &&
        ((option_sidebar_monitor_ == 0 &&
          option_sidebar_position_ == SIDEBAR_POSITION_LEFT) ||
         (option_sidebar_monitor_ == monitor_number - 1 &&
          option_sidebar_position_ == SIDEBAR_POSITION_RIGHT))) {
      // lazy initial gdk atoms
      if (net_wm_strut_ == GDK_NONE)
        net_wm_strut_ = gdk_atom_intern("_NET_WM_STRUT", FALSE);
      if (net_wm_strut_partial_ == GDK_NONE)
        net_wm_strut_partial_ = gdk_atom_intern("_NET_WM_STRUT_PARTIAL", FALSE);

      // change strut property now
      gulong struts[12];
      memset(struts, 0, sizeof(struts));
      if (option_sidebar_position_ == SIDEBAR_POSITION_LEFT) {
        struts[0] = option_sidebar_width_;
        struts[5] = static_cast<gulong>(side_bar_->GetHeight());
      } else {
        struts[1] = option_sidebar_width_;
        struts[7] = static_cast<gulong>(side_bar_->GetHeight());
      }
      gdk_property_change(main_widget_->window, net_wm_strut_,
                          gdk_atom_intern("CARDINAL", FALSE),
                          32, GDK_PROP_MODE_REPLACE,
                          reinterpret_cast<guchar *>(&struts), 4);
      gdk_property_change(main_widget_->window, net_wm_strut_partial_,
                          gdk_atom_intern("CARDINAL", FALSE),
                          32, GDK_PROP_MODE_REPLACE,
                          reinterpret_cast<guchar *>(&struts), 12);

      // change Xwindow type
      gdk_window_set_type_hint(main_widget_->window,
                               GDK_WINDOW_TYPE_HINT_DOCK);
    } else {
      // delete the properties
      gdk_property_delete(main_widget_->window, net_wm_strut_);
      gdk_property_delete(main_widget_->window, net_wm_strut_partial_);
      gdk_window_set_type_hint(main_widget_->window,
                               GDK_WINDOW_TYPE_HINT_NORMAL);
    }
  }

  bool Dock(View *view, double height, bool force_insert) {
    view->GetGadget()->SetDisplayTarget(Gadget::TARGET_SIDEBAR);
    DLOG("Dock in SidebarGtkHost, view: %p", view);
    ViewHostInterface *view_host = side_bar_->NewViewHost(height);
    DecoratedViewHost *decorator =
        new DecoratedViewHost(view_host, DecoratedViewHost::MAIN_DOCKED, true);
    decorator->ConnectOnUndock(NewSlot(this, &Impl::HandleFloatingUndock));
    decorator->ConnectOnClose(NewSlot(this, &Impl::OnCloseHandler, decorator));
    decorator->ConnectOnPopOut(NewSlot(this, &Impl::OnPopOutHandler, decorator));
    decorator->ConnectOnPopIn(NewSlot(this, &Impl::OnPopInHandler, decorator));
    ViewHostInterface *old = view->SwitchViewHost(decorator);
    if (old) old->Destroy();
    view->ShowView(false, 0, NULL);
    side_bar_->Layout();
    return true;
  }

  class SlotPostCallback : public WatchCallbackInterface {
   public:
    SlotPostCallback(ViewHostInterface *view_host,
                     GtkWidget *new_window,
                     GtkWidget *sidebar_window)
        : view_host_(view_host),
          new_window_(new_window),
          sidebar_window_(sidebar_window) {}
    virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
      gdk_pointer_grab(new_window_->window, FALSE,
                       (GdkEventMask)(GDK_BUTTON_RELEASE_MASK |
                                      GDK_POINTER_MOTION_MASK |
                                      GDK_POINTER_MOTION_HINT_MASK),
                       NULL, NULL, gtk_get_current_event_time());
      gtk_window_deiconify(GTK_WINDOW(new_window_));
      gdk_window_focus(new_window_->window, gtk_get_current_event_time());
      gtk_window_set_transient_for(GTK_WINDOW(new_window_),
                                   GTK_WINDOW(sidebar_window_));
      DLOG("call the slot now");
      view_host_->BeginMoveDrag(MouseEvent::BUTTON_LEFT);
      return false;
    }
    virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
      delete this;
    }
    ViewHostInterface *view_host_;
    GtkWidget *new_window_;
    GtkWidget *sidebar_window_;
  };

  bool Undock(View *view, bool move_to_cursor) {
    view->GetGadget()->SetDisplayTarget(Gadget::TARGET_FLOATING_VIEW);
    ViewElement *ele = side_bar_->FindViewElementByView(view);
    double new_native_x, new_native_y, view_x, view_y;
    int native_x, native_y, px, py;
    gtk_widget_get_pointer(main_widget_, &native_x, &native_y);
    if (move_to_cursor) {
      // calculate the cursor coordinate in the view element
      ASSERT(ele);
      View *view = ele->GetChildView()->GetGadget()->GetMainView();
      view->NativeWidgetCoordToViewCoord(native_x, native_y, &view_x, &view_y);
      if (gdk_pointer_is_grabbed())
        gdk_pointer_ungrab(gtk_get_current_event_time());
      //FIXME: a minor bug of inserting null element to a wrong place
      side_bar_->InsertNullElement(native_y, view);
    }
    DecoratedViewHost *new_host = NewSingleViewHost(view, true, native_y);
    if (move_to_cursor)
      new_host->EnableAutoRestoreViewStates(false);
    ViewHostInterface *old = view->SwitchViewHost(new_host);
    if (old) old->Destroy();
    bool r = view->ShowView(false, 0, NULL);
    if (move_to_cursor) {
      View *new_view = down_cast<View *>(new_host->GetView());
      new_view->ViewCoordToNativeWidgetCoord(view_x, view_y,
                                             &new_native_x, &new_native_y);
      gdk_display_get_pointer(gdk_display_get_default(), NULL, &px, &py, NULL);
      // move new gadget to a proper place
      GtkWidget *window =
          gtk_widget_get_toplevel(GTK_WIDGET(new_host->GetNativeWidget()));
      gtk_window_move(GTK_WINDOW(window), px - static_cast<int>(new_native_x),
                      py - static_cast<int>(new_native_y));
      DLOG("wx: %d, wy: %d, px: %d, py: %d, vx: %f vy: %f, nx: %f, ny: %f",
           native_x, native_y, px, py,
           view_x, view_y, new_native_x, new_native_y);
      // posted the slot in to main loop, to avoid it is run before the window is
      // moved
      GetGlobalMainLoop()->AddTimeoutWatch(200, new SlotPostCallback(
          new_host, window, main_widget_));
    }
    return r;
  }

  void HandleUndock() {
    ViewElement *element = side_bar_->GetMouseOverElement();
    if (element) {
      Gadget *gadget = element->GetChildView()->GetGadget();
      Undock(gadget->GetMainView(), true);
    }
  }

  // handle undock event triggered by click menu, the undocked gadget should
  // not move with cursor
  void HandleFloatingUndock() {
    ViewElement *element = side_bar_->GetMouseOverElement();
    if (element) {
      Gadget *gadget = element->GetChildView()->GetGadget();
      Undock(gadget->GetMainView(), false);
    }
  }

  void HandleGeneralPopIn() {
    OnPopInHandler(expanded_original_);
  }

  bool HandleSideBarBeginResizeDrag(int button, int hittest) {
    if (button != MouseEvent::BUTTON_LEFT || (hittest != ViewInterface::HT_LEFT
                                         && hittest != ViewInterface::HT_RIGHT))
        return true;

    if (option_always_on_top_)
      gdk_window_set_type_hint(main_widget_->window,
                               GDK_WINDOW_TYPE_HINT_NORMAL);
    return false;
  }

  void HandleSideBarEndResizeDrag() {
    if (option_always_on_top_)
      AdjustSidebar();
  }

  bool HandleSideBarBeginMoveDrag(int button) {
    DLOG("Hanlde Begin Move sidebar.");
    if (button != MouseEvent::BUTTON_LEFT) return true;
    if (option_always_on_top_)
      gdk_window_set_type_hint(main_widget_->window,
                               GDK_WINDOW_TYPE_HINT_NORMAL);
    return false;
  }

  void HandleSideBarEndMoveDrag() {
    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(main_widget_));
    option_sidebar_monitor_ =
        gdk_screen_get_monitor_at_window(screen, main_widget_->window);
    GdkRectangle rect;
    gdk_screen_get_monitor_geometry(screen, option_sidebar_monitor_, &rect);
    int px, py;
    view_host_->GetWindowPosition(&px, &py);
    if (px >= rect.x + rect.width / 2)
      option_sidebar_position_ = SIDEBAR_POSITION_RIGHT;
    else
      option_sidebar_position_ = SIDEBAR_POSITION_LEFT;

    AdjustSidebar();
  }

#if GTK_CHECK_VERSION(2,10,0)
  void HideOrShowAllGadgets(bool show) {
    for (GadgetsMap::iterator it = gadgets_.begin();
         it != gadgets_.end(); ++it) {
      if (it->second->GetDisplayTarget() != Gadget::TARGET_SIDEBAR)
        if (show)
          it->second->ShowMainView();
        else
          it->second->CloseMainView();
    }
    if (show)
      gtk_widget_show(main_widget_);
    else
      gtk_widget_hide(main_widget_);

    gadgets_shown_ = show;
  }

  static gboolean HandleFocusOutEvent(GtkWidget *widget, GdkEventFocus *event,
                                      Impl *this_p) {
    DLOG("side bar received focus out event, config: %d, widget: %p(%p)",
         this_p->option_auto_hide_, widget, this_p->main_widget_);
    if (this_p->option_auto_hide_) {
      gtk_widget_hide(widget);
      this_p->gadgets_shown_ = false;
    }
    return FALSE;
  }

  static void ToggleAllGadgetsHandler(GtkWidget *widget, Impl *this_p) {
    this_p->HideOrShowAllGadgets(!this_p->gadgets_shown_);
  }

  static void StatusIconPopupMenuHandler(GtkWidget *widget, guint button,
                                         guint activate_time,
                                         Impl *this_p) {
    this_p->side_bar_->GetViewHost()->ShowContextMenu(MouseEvent::BUTTON_LEFT);
  }
#endif

  void InitGadgets() {
    gadget_manager_->ConnectOnNewGadgetInstance(
        NewSlot(this, &Impl::NewGadgetInstanceCallback));
    gadget_manager_->ConnectOnRemoveGadgetInstance(
        NewSlot(this, &Impl::RemoveGadgetInstanceCallback));
  }

  bool LoadGadget(const char *path, const char *options_name, int instance_id) {
    if (gadgets_.find(instance_id) != gadgets_.end()) {
      // Gadget is already loaded.
      return true;
    }

    Gadget *gadget = new Gadget(
        owner_, path, options_name, instance_id,
        gadget_manager_->IsGadgetInstanceTrusted(instance_id));

    DLOG("Gadget %p with view %p", gadget, gadget->GetMainView());

    if (!gadget->IsValid()) {
      LOG("Failed to load gadget %s", path);
      delete gadget;
      return false;
    }

    if (!Dock(gadget->GetMainView(), 0, false)) {
      DLOG("Dock view(%p) failed.", gadget->GetMainView());
      Undock(gadget->GetMainView(), false);
    }

    if (!gadget->ShowMainView()) {
      LOG("Failed to show main view of gadget %s", path);
      delete gadget;
      return false;
    }

    gadgets_[instance_id] = gadget;
    return true;
  }

  DecoratedViewHost *NewSingleViewHost(View *view,
                                       bool remove_on_close, double height) {
    SingleViewHost *view_host =
      new SingleViewHost(ViewHostInterface::VIEW_HOST_MAIN, 1.0,
          decorated_, remove_on_close, false, view_debug_mode_);

    DecoratedViewHost *decorator = new DecoratedViewHost(
        view_host, DecoratedViewHost::MAIN_STANDALONE, true);
    decorator->ConnectOnClose(NewSlot(this, &Impl::OnCloseHandler, decorator));
    decorator->ConnectOnPopIn(NewSlot(this, &Impl::OnPopInHandler, decorator));
    GadgetMoveClosure *closure =
        new GadgetMoveClosure(this, view_host, decorator, view, height);
    move_slots_[view->GetGadget()] = closure;
    DLOG("New decorator %p with vh %p", decorator, view_host);
    return decorator;
  }

  ViewHostInterface *NewViewHost(ViewHostInterface::Type type) {
    ViewHostInterface *view_host;
    DecoratedViewHost *decorator;
    switch (type) {
      case ViewHostInterface::VIEW_HOST_MAIN:
        view_host = side_bar_->NewViewHost(0);
        decorator = new DecoratedViewHost(view_host,
                                          DecoratedViewHost::MAIN_DOCKED,
                                          true);
        decorator->ConnectOnUndock(NewSlot(this, &Impl::HandleFloatingUndock));
        break;
      case ViewHostInterface::VIEW_HOST_OPTIONS:
        // No decorator for options view.
        view_host = new SingleViewHost(type, 1.0, true, true, true,
                                       view_debug_mode_);
        return view_host;
      default:
        {
          DLOG("open detail view.");
          SingleViewHost *sv = new SingleViewHost(type, 1.0, decorated_,
                                                  true, true, view_debug_mode_);
          decorator = new DecoratedViewHost(sv, DecoratedViewHost::DETAILS,
                                            true);
          sv->ConnectOnShowHide(NewSlot(this, &Impl::HandleDetailsViewShow, sv));
          sv->ConnectOnBeginResizeDrag(
              NewSlot(this, &Impl::HandlePopOutBeginResizeDrag));
          sv->ConnectOnBeginMoveDrag(
              NewSlot(this, &Impl::HandlePopoutViewMove));
        }
        break;
    }
    decorator->ConnectOnClose(NewSlot(this, &Impl::OnCloseHandler, decorator));
    decorator->ConnectOnPopIn(NewSlot(this, &Impl::OnPopInHandler, decorator));
    return decorator;
  }

  void RemoveGadget(Gadget *gadget, bool save_data) {
    ASSERT(gadget);
    ViewInterface *main_view = gadget->GetMainView();

    // If this gadget is popped out, popin it first.
    if (main_view->GetViewHost() == expanded_popout_) {
      OnPopInHandler(expanded_original_);
    }

    gadget_manager_->RemoveGadgetInstance(gadget->GetInstanceID());
  }

  void OnCloseHandler(DecoratedViewHost *decorated) {
    ViewInterface *child = decorated->GetView();
    Gadget *gadget = child ? child->GetGadget() : NULL;

    ASSERT(gadget);
    if (!gadget) return;

    switch (decorated->GetDecoratorType()) {
      case DecoratedViewHost::MAIN_STANDALONE:
      case DecoratedViewHost::MAIN_DOCKED:
        gadget->RemoveMe(true);
        break;
      case DecoratedViewHost::MAIN_EXPANDED:
        if (expanded_original_ &&
            expanded_popout_ == decorated)
          OnPopInHandler(expanded_original_);
        break;
      case DecoratedViewHost::DETAILS:
        gadget->CloseDetailsView();
        break;
      default:
        ASSERT("Invalid decorator type.");
    }
  }

  void OnPopOutHandler(DecoratedViewHost *decorated) {
    if (expanded_original_) {
      OnPopInHandler(expanded_original_);
    }

    ViewInterface *child = decorated->GetView();
    ASSERT(child);
    if (child) {
      ViewElement *ele = side_bar_->SetPopoutedView(child);
      expanded_original_ = decorated;
      SingleViewHost *svh = new SingleViewHost(
          ViewHostInterface::VIEW_HOST_MAIN, 1.0, false, false, false,
          static_cast<ViewInterface::DebugMode>(view_debug_mode_));
      svh->ConnectOnBeginMoveDrag(NewSlot(this, &Impl::HandlePopoutViewMove));
      svh->ConnectOnBeginResizeDrag(
          NewSlot(this, &Impl::HandlePopOutBeginResizeDrag));
      expanded_popout_ =
          new DecoratedViewHost(svh, DecoratedViewHost::MAIN_EXPANDED, true);
      expanded_popout_->ConnectOnClose(NewSlot(this, &Impl::OnCloseHandler,
                                               expanded_popout_));

      // Send popout event to decorator first.
      SimpleEvent event(Event::EVENT_POPOUT);
      expanded_original_->GetDecoratedView()->OnOtherEvent(event);

      child->SwitchViewHost(expanded_popout_);
      expanded_popout_->ShowView(false, 0, NULL);
      SetProperPopoutPosition(ele, svh);
    }
  }

  void OnPopInHandler(DecoratedViewHost *decorated) {
    if (expanded_original_ == decorated && expanded_popout_) {
      ViewInterface *child = expanded_popout_->GetView();
      ASSERT(child);
      if (child) {
        ViewHostInterface *old_host = child->SwitchViewHost(expanded_original_);
        SimpleEvent event(Event::EVENT_POPIN);
        expanded_original_->GetDecoratedView()->OnOtherEvent(event);
        // The old host must be destroyed after sending onpopin event.
        old_host->Destroy();
        expanded_original_ = NULL;
        expanded_popout_ = NULL;
        side_bar_->SetPopoutedView(NULL);
      }
    }
  }
  //Refactor To here

  void SetProperPopoutPosition(const BasicElement *element_in_sidebar,
                               SingleViewHost *popout_view_host) {
    double ex, ey;
    element_in_sidebar->SelfCoordToViewCoord(0, 0, &ex, &ey);
    int sx, sy;
    view_host_->GetWindowPosition(&sx, &sy);
    if (option_sidebar_position_ == SIDEBAR_POSITION_RIGHT) {
      int pw = static_cast<int>(popout_view_host->GetView()->GetWidth());
      popout_view_host->SetWindowPosition(sx - pw, sy + static_cast<int>(ey));
    } else {
      int sw, sh;
      view_host_->GetWindowSize(&sw, &sh);
      popout_view_host->SetWindowPosition(sx + sw, sy + static_cast<int>(ey));
    }
  }

  void RemoveGadgetInstanceCallback(int instance_id) {
    GadgetsMap::iterator it = gadgets_.find(instance_id);

    if (it != gadgets_.end()) {
      delete it->second;
      gadgets_.erase(it);
    } else {
      LOG("Can't find gadget instance %d", instance_id);
    }
  }

  void HandleAddGadget() {
    DLOG("Add Gadget now");
    gadget_manager_->ShowGadgetBrowserDialog(&gadget_browser_host_);
  }

  bool HandlePopoutViewMove(int button) {
    // popout view is not allowed to move, just return true
    return true;
  }

  void AddGadgetHandlerWithOneArg(const char *str) {
    DLOG("Add Gadget now, str: %s", str);
    gadget_manager_->ShowGadgetBrowserDialog(&gadget_browser_host_);
  }

  void HandleMenuAutoHide(const char *str) {
    option_auto_hide_ = !option_auto_hide_;
    options_->PutValue(kOptionAutoHide, Variant(option_auto_hide_));
  }

  void HandleMenuAlwaysOnTop(const char *str) {
    option_always_on_top_ = !option_always_on_top_;
    options_->PutValue(kOptionAlwaysOnTop, Variant(option_always_on_top_));
    AdjustSidebar();
  }

  void HandleMenuReplaceSidebar(const char *str) {
    if (!strcmp(GM_("MENU_ITEM_LEFT"), str))
      option_sidebar_position_ = SIDEBAR_POSITION_LEFT;
    else
      option_sidebar_position_ = SIDEBAR_POSITION_RIGHT;
    options_->PutValue(kOptionPosition, Variant(option_sidebar_position_));
    AdjustSidebar();
  }

  void HandleMenuFontSizeChange(const char *str) {
    if (!strcmp(GM_("MENU_ITEM_FONT_SIZE_LARGE"), str))
      option_font_size_ += 2;
    else if (!strcmp(GM_("MENU_ITEM_FONT_SIZE_DEFAULT"), str))
      option_font_size_ = kDefaultFontSize;
    else
      option_font_size_ -= 2;
    options_->PutValue(kOptionFontSize, Variant(option_font_size_));
  }

  void HandleMenuClose(const char *str) {
    HandleClose();
  }

  void HanldeSizeEvent() {
    option_sidebar_width_ = static_cast<int>(side_bar_->GetWidth());
  }

  bool HandleMenuOpen(MenuInterface *menu) {
    int priority = MenuInterface::MENU_ITEM_PRI_HOST;
    menu->AddItem(GM_("MENU_ITEM_ADD_GADGETS"), 0,
                  NewSlot(this, &Impl::AddGadgetHandlerWithOneArg), priority);
    menu->AddItem(NULL, 0, NULL, priority);
    menu->AddItem(GM_("MENU_ITEM_AUTO_HIDE"),
                  option_auto_hide_ ? MenuInterface::MENU_ITEM_FLAG_CHECKED : 0,
                  NewSlot(this, &Impl::HandleMenuAutoHide), priority);
    menu->AddItem(GM_("MENU_ITEM_ALWAYS_ON_TOP"), option_always_on_top_ ?
                  MenuInterface::MENU_ITEM_FLAG_CHECKED : 0,
                  NewSlot(this, &Impl::HandleMenuAlwaysOnTop), priority);
    {
      MenuInterface *sub = menu->AddPopup(GM_("MENU_ITEM_DOCK_SIDEBAR"),
                                          priority);
      sub->AddItem(GM_("MENU_ITEM_LEFT"),
                   option_sidebar_position_ == SIDEBAR_POSITION_LEFT ?
                   MenuInterface::MENU_ITEM_FLAG_CHECKED : 0,
                   NewSlot(this, &Impl::HandleMenuReplaceSidebar), priority);
      sub->AddItem(GM_("MENU_ITEM_RIGHT"),
                   option_sidebar_position_ == SIDEBAR_POSITION_RIGHT ?
                   MenuInterface::MENU_ITEM_FLAG_CHECKED : 0,
                   NewSlot(this, &Impl::HandleMenuReplaceSidebar), priority);
    }
    {
      MenuInterface *sub = menu->AddPopup(GM_("MENU_ITEM_FONT_SIZE"),
                                          priority);
      sub->AddItem(GM_("MENU_ITEM_FONT_SIZE_LARGE"), 0,
                   NewSlot(this, &Impl::HandleMenuFontSizeChange), priority);
      sub->AddItem(GM_("MENU_ITEM_FONT_SIZE_DEFAULT"), 0,
                   NewSlot(this, &Impl::HandleMenuFontSizeChange), priority);
      sub->AddItem(GM_("MENU_ITEM_FONT_SIZE_SMALL"), 0,
                   NewSlot(this, &Impl::HandleMenuFontSizeChange), priority);
    }
    menu->AddItem(NULL, 0, NULL, priority);
    menu->AddItem(GM_("MENU_ITEM_CLOSE"), 0,
                  NewSlot(this, &Impl::HandleMenuClose), priority);
    return false;
  }

  void HandleClose() {
    gtk_main_quit();
  }

  void HandleDetailsViewShow(bool show, SingleViewHost *view_host) {
    if (!show) return;
    ASSERT(side_bar_->GetMouseOverElement());
    SetProperPopoutPosition(side_bar_->GetMouseOverElement(), view_host);
  }

  bool HandlePopOutBeginResizeDrag(int button, int hittest) {
    if (button != MouseEvent::BUTTON_LEFT ||
        hittest == ViewInterface::HT_BOTTOM ||
        hittest == ViewInterface::HT_TOP)
      return true;

    if ((option_sidebar_position_ == SIDEBAR_POSITION_LEFT &&
         (hittest == ViewInterface::HT_LEFT ||
          hittest == ViewInterface::HT_TOPLEFT ||
          hittest == ViewInterface::HT_BOTTOMLEFT)) ||
        (option_sidebar_position_ == SIDEBAR_POSITION_RIGHT &&
         (hittest == ViewInterface::HT_RIGHT ||
          hittest == ViewInterface::HT_TOPRIGHT ||
          hittest == ViewInterface::HT_BOTTOMRIGHT)))
      return true;

    return false;
  }

  void DebugOutput(DebugLevel level, const char *message) const {
    const char *str_level = "";
    switch (level) {
      case DEBUG_TRACE: str_level = "TRACE: "; break;
      case DEBUG_INFO: str_level = "INFO: "; break;
      case DEBUG_WARNING: str_level = "WARNING: "; break;
      case DEBUG_ERROR: str_level = "ERROR: "; break;
      default: break;
    }
    LOG("%s%s", str_level, message);
  }

  void ReportScriptError(const char *message) {
    DebugOutput(DEBUG_ERROR,
                (std::string("Script error: " ) + message).c_str());
  }

  void LoadGadgets() {
    gadget_manager_->EnumerateGadgetInstances(
        NewSlot(this, &Impl::AddGadgetInstanceCallback));
  }

  GadgetBrowserHost gadget_browser_host_;

  typedef std::map<Gadget *, GadgetMoveClosure *> GadgetMoveClosureMap;
  GadgetMoveClosureMap move_slots_;

  typedef std::map<int, Gadget *> GadgetsMap;
  GadgetsMap gadgets_;
  SidebarGtkHost *owner_;

  bool decorated_;
  bool gadgets_shown_;
  int view_debug_mode_;

  SingleViewHost *view_host_;
  DecoratedViewHost *expanded_original_;
  DecoratedViewHost *expanded_popout_;
  SideBar *side_bar_;

  OptionsInterface *options_;
  //TODO: add another option for visible in all workspace?
  bool option_auto_hide_;
  bool option_always_on_top_;
  int  option_sidebar_position_;
  int  option_sidebar_width_;
  int  option_sidebar_monitor_;
  int  option_font_size_;

  GdkAtom net_wm_strut_;
  GdkAtom net_wm_strut_partial_;

  GadgetManagerInterface *gadget_manager_;
#if GTK_CHECK_VERSION(2,10,0)
  GtkStatusIcon *status_icon_;
#endif
  GtkWidget *main_widget_;
};

SidebarGtkHost::SidebarGtkHost(bool decorated, int view_debug_mode)
  : impl_(new Impl(this, decorated, view_debug_mode)) {
  impl_->SetupUI();
  impl_->InitGadgets();
}

SidebarGtkHost::~SidebarGtkHost() {
  delete impl_;
  impl_ = NULL;
}

ViewHostInterface *SidebarGtkHost::NewViewHost(ViewHostInterface::Type type) {
  return impl_->NewViewHost(type);
}

void SidebarGtkHost::RemoveGadget(Gadget *gadget, bool save_data) {
  return impl_->RemoveGadget(gadget, save_data);
}

void SidebarGtkHost::DebugOutput(DebugLevel level, const char *message) const {
  impl_->DebugOutput(level, message);
}

bool SidebarGtkHost::OpenURL(const char *url) const {
  return ggadget::gtk::OpenURL(url);
}

bool SidebarGtkHost::LoadFont(const char *filename) {
  return ggadget::gtk::LoadFont(filename);
}

void SidebarGtkHost::ShowGadgetAboutDialog(ggadget::Gadget *gadget) {
  ggadget::gtk::ShowGadgetAboutDialog(gadget);
}

void SidebarGtkHost::Run() {
  impl_->LoadGadgets();
  gtk_main();
}

} // namespace gtk
} // namespace hosts
