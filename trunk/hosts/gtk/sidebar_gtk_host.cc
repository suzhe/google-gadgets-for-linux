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

#include "sidebar_gtk_host.h"

#include <gtk/gtk.h>
#include <algorithm>
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
const char *kDisplayTarget      = "display_target";
const char *kPositionInSideBar  = "position_in_sidebar";

const int kAutoHideTimeout         = 200;
const int kAutoShowTimeout         = 1000;
const int kDefaultFontSize         = 14;
const int kDefaultSideBarWidth     = 200;
const int kDefaultMonitor          = 0;
const int kSideBarMinimizedHeight  = 28;
const int kSideBarMinimizedWidth   = 2;
const int kDefaultRulerHeight      = 1;
const int kDefaultRulerWidth       = 1;

enum SideBarPosition {
  SIDEBAR_POSITION_LEFT,
  SIDEBAR_POSITION_RIGHT,
};

class SideBarGtkHost::Impl {
 public:
  class GadgetViewHostInfo {
   public:
    GadgetViewHostInfo(Gadget *g) {
      Reset(g);
    }
    ~GadgetViewHostInfo() {
      delete gadget;
      gadget = NULL;
    }
    void Reset(Gadget *g) {
      gadget = g;
      decorated_view_host = NULL;
      details_view_host = NULL;
      floating_view_host = NULL;
      pop_out_view_host = NULL;
      y_in_sidebar = 0;
      undock_by_drag = false;
    }

   public:
    Gadget *gadget;

    DecoratedViewHost *decorated_view_host;
    SingleViewHost *details_view_host;
    SingleViewHost *floating_view_host;
    SingleViewHost *pop_out_view_host;

    double y_in_sidebar;
    bool undock_by_drag;
  };

  Impl(SideBarGtkHost *owner, bool decorated, int view_debug_mode)
    : gadget_browser_host_(owner, view_debug_mode),
      owner_(owner),
      decorated_(decorated),
      gadgets_shown_(true),
      sidebar_shown_(true),
      transparent_(false),
      view_debug_mode_(view_debug_mode),
      sidebar_host_(NULL),
      expanded_original_(NULL),
      expanded_popout_(NULL),
      details_view_opened_gadget_(NULL),
      draging_gadget_(NULL),
      drag_observer_(NULL),
      floating_offset_x_(-1),
      floating_offset_y_(-1),
      sidebar_moving_(false),
      has_strut_(false),
      sidebar_(NULL),
      options_(GetGlobalOptions()),
      option_auto_hide_(false),
      option_always_on_top_(false),
      option_font_size_(kDefaultFontSize),
      option_sidebar_monitor_(kDefaultMonitor),
      option_sidebar_position_(SIDEBAR_POSITION_RIGHT),
      option_sidebar_width_(kDefaultSideBarWidth),
      auto_hide_source_(0),
      net_wm_strut_(GDK_NONE),
      net_wm_strut_partial_(GDK_NONE),
      gadget_manager_(GetGadgetManager()),
#if GTK_CHECK_VERSION(2,10,0)
      status_icon_(NULL),
#endif
      main_widget_(NULL) {
    workarea_.x = 0;
    workarea_.y = 0;
    workarea_.width = 0;
    workarea_.height = 0;

    ASSERT(gadget_manager_);
    sidebar_host_ = new SingleViewHost(ViewHostInterface::VIEW_HOST_MAIN, 1.0,
                                     decorated, false, false, view_debug_mode_);
    sidebar_host_->ConnectOnBeginResizeDrag(
        NewSlot(this, &Impl::HandleSideBarBeginResizeDrag));
    sidebar_host_->ConnectOnEndResizeDrag(
        NewSlot(this, &Impl::HandleSideBarEndResizeDrag));
    sidebar_host_->ConnectOnBeginMoveDrag(
        NewSlot(this, &Impl::HandleSideBarBeginMoveDrag));
    sidebar_host_->ConnectOnShowHide(NewSlot(this, &Impl::HandleSideBarShow));

    sidebar_ = new SideBar(sidebar_host_);
    sidebar_->ConnectOnAddGadget(NewSlot(this, &Impl::HandleAddGadget));
    sidebar_->ConnectOnMenuOpen(NewSlot(this, &Impl::HandleMenuOpen));
    sidebar_->ConnectOnClose(NewSlot(this, &Impl::HandleClose));
    sidebar_->ConnectOnSizeEvent(NewSlot(this, &Impl::HandleSizeEvent));
    sidebar_->ConnectOnUndock(NewSlot(this, &Impl::HandleUndock));
    sidebar_->ConnectOnPopIn(NewSlot(this, &Impl::HandleGeneralPopIn));

    LoadGlobalOptions();
  }

  ~Impl() {
    if (auto_hide_source_)
      g_source_remove(auto_hide_source_);
    auto_hide_source_ = 0;

    for (GadgetsMap::iterator it = gadgets_.begin();
         it != gadgets_.end(); ++it)
      delete it->second;

    delete sidebar_;

#if GTK_CHECK_VERSION(2,10,0)
    g_object_unref(G_OBJECT(status_icon_));
#endif
  }

  void HandleWorkAreaChange() {
    GdkRectangle old = workarea_;
    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(main_widget_));
    int screen_width = gdk_screen_get_width(screen);
    GetWorkAreaGeometry(main_widget_, &workarea_);
    // Remove the portion that occupied by sidebar itself.
    if (has_strut_) {
      if (option_sidebar_position_ == SIDEBAR_POSITION_LEFT &&
          workarea_.x >= option_sidebar_width_) {
        workarea_.x -= option_sidebar_width_;
        workarea_.width += option_sidebar_width_;
      } else if (option_sidebar_position_ == SIDEBAR_POSITION_RIGHT &&
                 workarea_.x + workarea_.width + option_sidebar_width_ <=
                 screen_width) {
        workarea_.width += option_sidebar_width_;
      }
    }
    DLOG("New work area: x:%d y:%d w:%d h:%d",
         workarea_.x, workarea_.y, workarea_.width, workarea_.height);

    if (old.x != workarea_.x || old.y != workarea_.y ||
        old.width != workarea_.width || old.height != workarea_.height)
      AdjustSideBar();
  }

  // SideBar handlers
  bool HandleSideBarBeginResizeDrag(int button, int hittest) {
    if (gadgets_shown_ && button == MouseEvent::BUTTON_LEFT &&
        ((hittest == ViewInterface::HT_LEFT &&
          option_sidebar_position_ == SIDEBAR_POSITION_RIGHT) ||
         (hittest == ViewInterface::HT_RIGHT &&
          option_sidebar_position_ == SIDEBAR_POSITION_LEFT)))
      return false;

    // Don't allow resize drag in any other situation.
    return true;
  }

  void HandleSideBarEndResizeDrag() {
    if (option_always_on_top_)
      AdjustSideBar();
  }

  bool HandleSideBarBeginMoveDrag(int button) {
    if (button != MouseEvent::BUTTON_LEFT) return true;
    if (gdk_pointer_grab(drag_observer_->window, FALSE,
                         (GdkEventMask)(GDK_BUTTON_RELEASE_MASK |
                                        GDK_POINTER_MOTION_MASK),
                         NULL, NULL, gtk_get_current_event_time()) ==
        GDK_GRAB_SUCCESS) {
      int x, y;
      gtk_widget_get_pointer(main_widget_, &x, &y);
      gdk_window_set_override_redirect(main_widget_->window, true);
      floating_offset_x_ = x;
      floating_offset_y_ = y;
      sidebar_moving_ = true;
    }
    return true;
  }

  void HandleSideBarMove() {
    int px, py;
    gdk_display_get_pointer(gdk_display_get_default(), NULL, &px, &py, NULL);
    sidebar_host_->SetWindowPosition(px - static_cast<int>(floating_offset_x_),
                                     py - static_cast<int>(floating_offset_y_));
  }

  void HandleSideBarEndMoveDrag() {
    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(main_widget_));
    option_sidebar_monitor_ =
        gdk_screen_get_monitor_at_window(screen, main_widget_->window);
    GdkRectangle rect;
    gdk_screen_get_monitor_geometry(screen, option_sidebar_monitor_, &rect);
    int px, py;
    sidebar_host_->GetWindowPosition(&px, &py);
    if (px >= rect.x + (rect.width - option_sidebar_width_) / 2)
      option_sidebar_position_ = SIDEBAR_POSITION_RIGHT;
    else
      option_sidebar_position_ = SIDEBAR_POSITION_LEFT;
    gdk_window_set_override_redirect(main_widget_->window, false);
    sidebar_moving_ = false;

    if (gadgets_shown_) AdjustSideBar();
  }

  void HandleSideBarShow(bool show) {
    if (show) AdjustSideBar();
  }

  void HandleAddGadget() {
    gadget_manager_->ShowGadgetBrowserDialog(&gadget_browser_host_);
  }

  bool HandleMenuOpen(MenuInterface *menu) {
    int priority = MenuInterface::MENU_ITEM_PRI_HOST;
    menu->AddItem(GM_("MENU_ITEM_ADD_GADGETS"), 0,
                  NewSlot(this, &Impl::AddGadgetHandlerWithOneArg), priority);
    menu->AddItem(NULL, 0, NULL, priority);
    if (!gadgets_shown_)
      menu->AddItem(GM_("MENU_ITEM_SHOW_ALL"), 0,
                    NewSlot(this, &Impl::HandleMenuShowAll), priority);
    // TODO: Auto hide feature is not ready yet.
    //menu->AddItem(GM_("MENU_ITEM_AUTO_HIDE"),
    //              option_auto_hide_ ? MenuInterface::MENU_ITEM_FLAG_CHECKED : 0,
    //              NewSlot(this, &Impl::HandleMenuAutoHide), priority);
    menu->AddItem(GM_("MENU_ITEM_ALWAYS_ON_TOP"), option_always_on_top_ ?
                  MenuInterface::MENU_ITEM_FLAG_CHECKED : 0,
                  NewSlot(this, &Impl::HandleMenuAlwaysOnTop), priority);
    {
      MenuInterface *sub = menu->AddPopup(GM_("MENU_ITEM_DOCK_SIDEBAR"),
                                          priority);
      sub->AddItem(GM_("MENU_ITEM_LEFT"),
                   option_sidebar_position_ == SIDEBAR_POSITION_LEFT ?
                   MenuInterface::MENU_ITEM_FLAG_CHECKED : 0,
                   NewSlot(this, &Impl::HandleMenuReplaceSideBar), priority);
      sub->AddItem(GM_("MENU_ITEM_RIGHT"),
                   option_sidebar_position_ == SIDEBAR_POSITION_RIGHT ?
                   MenuInterface::MENU_ITEM_FLAG_CHECKED : 0,
                   NewSlot(this, &Impl::HandleMenuReplaceSideBar), priority);
    }
  /* comment since font size change is not supported yet. {
      MenuInterface *sub = menu->AddPopup(GM_("MENU_ITEM_FONT_SIZE"),
                                          priority);
      sub->AddItem(GM_("MENU_ITEM_FONT_SIZE_LARGER"), 0,
                   NewSlot(this, &Impl::HandleMenuFontSizeChange), priority);
      sub->AddItem(GM_("MENU_ITEM_FONT_SIZE_DEFAULT"), 0,
                   NewSlot(this, &Impl::HandleMenuFontSizeChange), priority);
      sub->AddItem(GM_("MENU_ITEM_FONT_SIZE_SMALLER"), 0,
                   NewSlot(this, &Impl::HandleMenuFontSizeChange), priority);
    }  */
    menu->AddItem(NULL, 0, NULL, priority);
    menu->AddItem(GM_("MENU_ITEM_EXIT"), 0,
                  NewSlot(this, &Impl::HandleExit), priority);
    return false;
  }

  void HandleClose() {
    HideOrShowAllGadgets(!gadgets_shown_);
  }

  void HandleSizeEvent() {
    // ignore width changes when the sidebar is auto hided
    if (!option_auto_hide_)
      option_sidebar_width_ = static_cast<int>(sidebar_->GetWidth());
  }

  void HandleUndock(double offset_x, double offset_y) {
    ViewElement *element = sidebar_->GetMouseOverElement();
    if (element) {
      int id = element->GetChildView()->GetGadget()->GetInstanceID();
      GadgetViewHostInfo *info = gadgets_[id];
      // calculate the cursor coordinate in the view element
      View *view = info->decorated_view_host->IsMinimized() ?
          down_cast<View *>(info->decorated_view_host->GetDecoratedView()) :
          down_cast<View *>(info->gadget->GetMainView());
      double view_x, view_y;
      double w = view->GetWidth(), h = element->GetPixelHeight();
      view->NativeWidgetCoordToViewCoord(offset_x, offset_y, &view_x, &view_y);

      Undock(id, true);
      if (gdk_pointer_grab(drag_observer_->window, FALSE,
                           (GdkEventMask)(GDK_BUTTON_RELEASE_MASK |
                                          GDK_POINTER_MOTION_MASK),
                           NULL, NULL, gtk_get_current_event_time()) ==
          GDK_GRAB_SUCCESS) {
        draging_gadget_ = info->gadget;
        sidebar_->InsertPlaceholder(info->y_in_sidebar, h);
        View *new_view = info->decorated_view_host->IsMinimized() ?
            down_cast<View *>(info->decorated_view_host->GetDecoratedView()) :
            down_cast<View *>(draging_gadget_->GetMainView());
        if (info->decorated_view_host->IsMinimized())
          new_view->SetSize(w, new_view->GetHeight());

        new_view->ViewCoordToNativeWidgetCoord(view_x, view_y,
                                               &floating_offset_x_,
                                               &floating_offset_y_);
        info->undock_by_drag = true;

        // make sure that the floating window can move on to the sidebar.
        info->floating_view_host->SetWindowType(GDK_WINDOW_TYPE_HINT_DOCK);
        // move window to the cursor position.
        int x, y;
        gdk_display_get_pointer(gdk_display_get_default(), NULL, &x, &y, NULL);
        info->floating_view_host->SetWindowPosition(
            x - static_cast<int>(floating_offset_x_),
            y - static_cast<int>(floating_offset_y_));
        info->floating_view_host->ShowView(false, 0, NULL);
        gdk_window_raise(info->floating_view_host->GetWindow()->window);
      }
    }
  }

  void HandleGeneralPopIn() {
    OnPopInHandler(expanded_original_);
  }

  // option load / save methods
  void LoadGlobalOptions() {
    // in first time only save default valus
    if (!options_->GetCount()) {
      FlushGlobalOptions();
      return;
    }

    bool corrupt_data = false;
    Variant value;
    value = options_->GetInternalValue(kOptionAutoHide);
    if (!value.ConvertToBool(&option_auto_hide_)) corrupt_data = true;
    value = options_->GetInternalValue(kOptionAlwaysOnTop);
    if (!value.ConvertToBool(&option_always_on_top_)) corrupt_data = true;
    value = options_->GetInternalValue(kOptionPosition);
    if (!value.ConvertToInt(&option_sidebar_position_)) corrupt_data = true;
    value = options_->GetInternalValue(kOptionWidth);
    if (!value.ConvertToInt(&option_sidebar_width_)) corrupt_data = true;
    value = options_->GetInternalValue(kOptionMonitor);
    if (!value.ConvertToInt(&option_sidebar_monitor_)) corrupt_data = true;
    value = options_->GetInternalValue(kOptionFontSize);
    if (!value.ConvertToInt(&option_font_size_)) corrupt_data = true;

    if (corrupt_data) FlushGlobalOptions();
  }

  void FlushGlobalOptions() {
    // save gadgets' information
    for (GadgetsMap::iterator it = gadgets_.begin();
         it != gadgets_.end(); ++it) {
      OptionsInterface *opt = it->second->gadget->GetOptions();
      opt->PutInternalValue(kDisplayTarget,
                            Variant(it->second->gadget->GetDisplayTarget()));
      opt->PutInternalValue(kPositionInSideBar,
                            Variant(it->second->y_in_sidebar));
    }

    // save sidebar's information
    options_->PutInternalValue(kOptionAutoHide, Variant(option_auto_hide_));
    options_->PutInternalValue(kOptionAlwaysOnTop,
                               Variant(option_always_on_top_));
    options_->PutInternalValue(kOptionPosition,
                               Variant(option_sidebar_position_));
    options_->PutInternalValue(kOptionWidth, Variant(option_sidebar_width_));
    options_->PutInternalValue(kOptionMonitor,
                               Variant(option_sidebar_monitor_));
    options_->PutInternalValue(kOptionFontSize, Variant(option_font_size_));
    options_->Flush();
  }

  void SetupUI() {
    main_widget_ = sidebar_host_->GetWindow();
    transparent_ = SupportsComposite(main_widget_);

    g_signal_connect_after(G_OBJECT(main_widget_), "focus-out-event",
                           G_CALLBACK(HandleFocusOutEvent), this);
    g_signal_connect_after(G_OBJECT(main_widget_), "focus-in-event",
                           G_CALLBACK(HandleFocusInEvent), this);
    g_signal_connect_after(G_OBJECT(main_widget_), "enter-notify-event",
                           G_CALLBACK(HandleEnterNotifyEvent), this);

    MonitorWorkAreaChange(main_widget_,
                          NewSlot(this, &Impl::HandleWorkAreaChange));

    // AdjustSideBar() will be called by this function.
    HandleWorkAreaChange();

#if GTK_CHECK_VERSION(2,10,0)
    std::string icon_data;
    if (GetGlobalFileManager()->ReadFile(kGadgetsIcon, &icon_data)) {
      GdkPixbuf *icon_pixbuf = LoadPixbufFromData(icon_data);
      status_icon_ = gtk_status_icon_new_from_pixbuf(icon_pixbuf);
      g_object_unref(icon_pixbuf);
    } else {
      status_icon_ = gtk_status_icon_new_from_stock(GTK_STOCK_ABOUT);
    }
    g_signal_connect(G_OBJECT(status_icon_), "activate",
                     G_CALLBACK(ToggleAllGadgetsHandler), this);
    g_signal_connect(G_OBJECT(status_icon_), "popup-menu",
                     G_CALLBACK(StatusIconPopupMenuHandler), this);
#else
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(main_widget_), FALSE);
#endif

    gtk_window_set_title(GTK_WINDOW(main_widget_), GM_("GOOGLE_GADGETS"));

    // create drag observer
    drag_observer_ = gtk_invisible_new();
    gtk_widget_show(drag_observer_);
    g_signal_connect(G_OBJECT(drag_observer_), "motion-notify-event",
                     G_CALLBACK(HandleDragMove), this);
    g_signal_connect(G_OBJECT(drag_observer_), "button-release-event",
                     G_CALLBACK(HandleDragEnd), this);
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
      LOG("SideBarGtkHost: Load gadget %s, with option %s, %s",
          path.c_str(), options.c_str(), result ? "succeeded" : "failed");
    }
    return true;
  }

  void AdjustSideBar() {
    GdkRectangle monitor_geometry;
    // got monitor information
    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(main_widget_));
    int screen_width = gdk_screen_get_width(screen);
    int monitor_number = gdk_screen_get_n_monitors(screen);
    if (option_sidebar_monitor_ >= monitor_number) {
      DLOG("want to put sidebar in %d monitor, but this screen(%p) has "
           "only %d monitor(s), put to last monitor.",
           option_sidebar_monitor_, screen, monitor_number);
      option_sidebar_monitor_ = monitor_number - 1;
    }
    gdk_screen_get_monitor_geometry(screen, option_sidebar_monitor_,
                                    &monitor_geometry);
    DLOG("monitor %d's rect: %d %d %d %d", option_sidebar_monitor_,
         monitor_geometry.x, monitor_geometry.y,
         monitor_geometry.width, monitor_geometry.height);

    DLOG("Set SideBar size: %dx%d", option_sidebar_width_, workarea_.height);
    sidebar_->SetSize(option_sidebar_width_, workarea_.height);

    int x = 0;
    if (option_sidebar_position_ == SIDEBAR_POSITION_LEFT) {
      x = std::max(monitor_geometry.x, workarea_.x);
    } else {
      x = std::min(monitor_geometry.x + monitor_geometry.width,
                   workarea_.x + workarea_.width) - option_sidebar_width_;
    }

    DLOG("move sidebar to %dx%d", x, workarea_.y);
    sidebar_host_->SetWindowPosition(x, workarea_.y);

    // if sidebar is on the edge, do strut
    if (option_always_on_top_ &&
        ((monitor_geometry.x <= 0 &&
          option_sidebar_position_ == SIDEBAR_POSITION_LEFT) ||
         (monitor_geometry.x + monitor_geometry.width >= screen_width &&
          option_sidebar_position_ == SIDEBAR_POSITION_RIGHT))) {
      has_strut_ = true;
      sidebar_host_->SetWindowType(GDK_WINDOW_TYPE_HINT_DOCK);

      // lazy initial gdk atoms
      if (net_wm_strut_ == GDK_NONE)
        net_wm_strut_ = gdk_atom_intern("_NET_WM_STRUT", FALSE);
      if (net_wm_strut_partial_ == GDK_NONE)
        net_wm_strut_partial_ = gdk_atom_intern("_NET_WM_STRUT_PARTIAL", FALSE);

      // change strut property now
      gulong struts[12];
      memset(struts, 0, sizeof(struts));
      if (option_sidebar_position_ == SIDEBAR_POSITION_LEFT) {
        struts[0] = x + option_sidebar_width_;
        struts[4] = workarea_.y;
        struts[5] = workarea_.y + workarea_.height;
      } else {
        struts[1] = screen_width - x;
        struts[6] = workarea_.y;
        struts[7] = workarea_.y + workarea_.height;
      }
      gdk_property_change(main_widget_->window, net_wm_strut_,
                          gdk_atom_intern("CARDINAL", FALSE),
                          32, GDK_PROP_MODE_REPLACE,
                          reinterpret_cast<guchar *>(&struts), 4);
      gdk_property_change(main_widget_->window, net_wm_strut_partial_,
                          gdk_atom_intern("CARDINAL", FALSE),
                          32, GDK_PROP_MODE_REPLACE,
                          reinterpret_cast<guchar *>(&struts), 12);
    } else {
      has_strut_ = false;
      sidebar_host_->SetWindowType(GDK_WINDOW_TYPE_HINT_NORMAL);

      // delete the properties
      gdk_property_delete(main_widget_->window, net_wm_strut_);
      gdk_property_delete(main_widget_->window, net_wm_strut_partial_);
    }
    sidebar_host_->SetKeepAbove(option_always_on_top_);

    // adjust the orientation of the arrow of each gadget in the sidebar
    for (GadgetsMap::iterator it = gadgets_.begin();
         it != gadgets_.end(); ++it) {
      if (it->second->gadget->GetDisplayTarget() == Gadget::TARGET_SIDEBAR) {
        it->second->decorated_view_host->SetDockEdge(
            option_sidebar_position_ == SIDEBAR_POSITION_RIGHT);
      }
    }
  }

  // close details view if have
  void CloseDetailsView(int gadget_id) {
    GadgetViewHostInfo *info = gadgets_[gadget_id];
    if (info->details_view_host) {
      info->gadget->CloseDetailsView();
      info->details_view_host = NULL;
    }
  }

  bool Dock(int gadget_id, bool force_insert) {
    GadgetViewHostInfo *info = gadgets_[gadget_id];
    ASSERT(info);
    info->gadget->SetDisplayTarget(Gadget::TARGET_SIDEBAR);
    ViewHostInterface *view_host = sidebar_->NewViewHost(info->y_in_sidebar);
    DecoratedViewHost *dvh =
        new DecoratedViewHost(view_host, DecoratedViewHost::MAIN_DOCKED,
                              transparent_);
    info->decorated_view_host = dvh;
    dvh->ConnectOnUndock(NewSlot(this, &Impl::HandleFloatingUndock, gadget_id));
    dvh->ConnectOnClose(NewSlot(this, &Impl::OnCloseHandler, dvh));
    dvh->ConnectOnPopOut(NewSlot(this, &Impl::OnPopOutHandler, dvh));
    dvh->ConnectOnPopIn(NewSlot(this, &Impl::OnPopInHandler, dvh));
    dvh->SetDockEdge(option_sidebar_position_ == SIDEBAR_POSITION_RIGHT);
    CloseDetailsView(gadget_id);
    ViewHostInterface *old = info->gadget->GetMainView()->SwitchViewHost(dvh);
    info->floating_view_host = NULL;
    if (old) old->Destroy();
    view_host->ShowView(false, 0, NULL);
    return true;
  }

  bool Undock(int gadget_id, bool move_to_cursor) {
    GadgetViewHostInfo *info = gadgets_[gadget_id];
    info->gadget->SetDisplayTarget(Gadget::TARGET_FLOATING_VIEW);
    CloseDetailsView(gadget_id);
    double view_x, view_y;
    View *view = info->gadget->GetMainView();
    ViewElement *view_element = sidebar_->FindViewElementByView(view);
    view_element->SelfCoordToViewCoord(0, 0, &view_x, &view_y);
    info->y_in_sidebar = view_y;
    DecoratedViewHost *new_host = NewSingleViewHost(gadget_id);
    if (move_to_cursor)
      new_host->EnableAutoRestoreViewSize(false);
    ViewHostInterface *old = view->SwitchViewHost(new_host);
    if (old) old->Destroy();
    return true;
  }

  void HandleDock(int gadget_id) {
    Dock(gadget_id, true);
  }

  bool HandleViewHostBeginMoveDrag(int button, int gadget_id) {
    GadgetViewHostInfo *info = gadgets_[gadget_id];
    ASSERT(info);
    if (gdk_pointer_grab(drag_observer_->window, FALSE,
                         (GdkEventMask)(GDK_BUTTON_RELEASE_MASK |
                                        GDK_POINTER_MOTION_MASK),
                         NULL, NULL, gtk_get_current_event_time()) ==
        GDK_GRAB_SUCCESS) {
      draging_gadget_ = info->gadget;
      int x, y;
      GtkWidget *window = info->floating_view_host->GetWindow();
      gtk_widget_get_pointer(window, &x, &y);
      floating_offset_x_ = x;
      floating_offset_y_ = y;
      // make sure that the floating window can move on to the sidebar.
      info->floating_view_host->SetWindowType(GDK_WINDOW_TYPE_HINT_DOCK);
      gdk_window_raise(window->window);
    }
    return true;
  }

  void HandleViewHostMove(int gadget_id) {
    int h, x, y;
    GadgetViewHostInfo *info = gadgets_[gadget_id];
    ASSERT(info);
    gdk_display_get_pointer(gdk_display_get_default(), NULL, &x, &y, NULL);
    info->floating_view_host->SetWindowPosition(
        x - static_cast<int>(floating_offset_x_),
        y - static_cast<int>(floating_offset_y_));
    if (info->details_view_host)
      SetPopoutPosition(gadget_id, info->details_view_host);
    if (IsOverlapWithSideBar(gadget_id, &h)) {
      sidebar_->InsertPlaceholder(
          h, info->floating_view_host->GetView()->GetHeight());
      info->y_in_sidebar = h;
    } else {
      sidebar_->ClearPlaceHolder();
    }
  }

  void HandleViewHostEndMoveDrag(int gadget_id) {
    int h, x, y;
    GadgetViewHostInfo *info = gadgets_[gadget_id];
    ASSERT(info);
    gdk_display_get_pointer(gdk_display_get_default(), NULL, &x, &y, NULL);
    if (IsOverlapWithSideBar(gadget_id, &h)) {
      info->y_in_sidebar = h;
      HandleDock(gadget_id);
    } else {
      if (info->undock_by_drag) {
        info->decorated_view_host->EnableAutoRestoreViewSize(true);
        info->decorated_view_host->RestoreViewSize();
        info->undock_by_drag = false;
      }
      // The floating window must be normal window when not dragging,
      // otherwise it'll always on top.
      info->floating_view_host->SetWindowType(GDK_WINDOW_TYPE_HINT_NORMAL);
    }
    sidebar_->ClearPlaceHolder();
    draging_gadget_ = NULL;
  }

  bool IsOverlapWithSideBar(int gadget_id, int *height) {
    int w, h, x, y;
    gadgets_[gadget_id]->floating_view_host->GetWindowSize(&w, &h);
    gadgets_[gadget_id]->floating_view_host->GetWindowPosition(&x, &y);
    int sx, sy, sw, sh;
    sidebar_host_->GetWindowPosition(&sx, &sy);
    sidebar_host_->GetWindowSize(&sw, &sh);
    if ((x + w >= sx) && (sx + sw >= x)) {
      if (height) {
        int dummy;
        gtk_widget_get_pointer(main_widget_, &dummy, height);
      }
      return true;
    }
    return false;
  }

  // handle undock event triggered by click menu, the undocked gadget should
  // not move with cursor
  void HandleFloatingUndock(int gadget_id) {
    Undock(gadget_id, false);
    gadgets_[gadget_id]->floating_view_host->ShowView(false, 0, NULL);
  }

  void HideOrShowAllGadgets(bool show) {
    for (GadgetsMap::iterator it = gadgets_.begin();
         it != gadgets_.end(); ++it) {
      Gadget *gadget = it->second->gadget;
      if (gadget->GetDisplayTarget() != Gadget::TARGET_SIDEBAR){
        if (show)
          gadget->ShowMainView();
        else
          gadget->CloseMainView();
      }
    }

    HideOrShowSideBar(show);
    gadgets_shown_ = show;
  }

  void HideOrShowSideBar(bool show) {
#if GTK_CHECK_VERSION(2,10,0)
    if (show) {
      AdjustSideBar();
      gtk_widget_show(main_widget_);
    } else {
      if (option_auto_hide_) {
        // TODO:
        //int x, y;
        //sidebar_host_->GetWindowPosition(&x, &y);
        // adjust to the edge of screen
        //AdjustSideBarPositionAndSize(y, kSideBarMinimizedWidth,
        //                             static_cast<int>(sidebar_->GetHeight()));
      } else {
        gtk_widget_hide(main_widget_);
      }
    }
#else
    if (show) {
      AdjustSideBar();
    } else {
      sidebar_->SetSize(option_sidebar_width_, kSideBarMinimizedHeight);
      if (option_always_on_top_) {
        gdk_property_delete(main_widget_->window, net_wm_strut_);
        gdk_property_delete(main_widget_->window, net_wm_strut_partial_);
      }
    }
#endif
    sidebar_shown_ = show;
  }

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

    Gadget *gadget = new Gadget(owner_, path, options_name, instance_id,
                                // We still don't trust any user added gadgets
                                // at gadget runtime level.
                                false);
    GadgetsMap::iterator it = gadgets_.find(instance_id);

    if (!gadget->IsValid()) {
      LOG("Failed to load gadget %s", path);
      if (it != gadgets_.end()) {
        delete it->second;
        gadgets_.erase(it);
      } else {
        delete gadget;
      }
      return false;
    }

    if (!gadget->ShowMainView()) {
      LOG("Failed to show main view of gadget %s", path);
      if (it != gadgets_.end()) {
        delete it->second;
        gadgets_.erase(it);
      } else {
        delete gadget;
      }
      return false;
    }

    if (gadget->GetDisplayTarget() == Gadget::TARGET_SIDEBAR)
      it->second->decorated_view_host->SetDockEdge(
          option_sidebar_position_ == SIDEBAR_POSITION_RIGHT);

    return true;
  }

  DecoratedViewHost *NewSingleViewHost(int gadget_id) {
    SingleViewHost *view_host =
      new SingleViewHost(ViewHostInterface::VIEW_HOST_MAIN, 1.0,
          decorated_, false, true, view_debug_mode_);
    gadgets_[gadget_id]->floating_view_host = view_host;
    view_host->ConnectOnBeginMoveDrag(
        NewSlot(this, &Impl::HandleViewHostBeginMoveDrag, gadget_id));
    DecoratedViewHost *decorator = new DecoratedViewHost(
        view_host, DecoratedViewHost::MAIN_STANDALONE, transparent_);
    gadgets_[gadget_id]->decorated_view_host = decorator;
    decorator->ConnectOnClose(NewSlot(this, &Impl::OnCloseHandler, decorator));
    decorator->ConnectOnPopIn(NewSlot(this, &Impl::OnPopInHandler, decorator));
    decorator->ConnectOnDock(NewSlot(this, &Impl::HandleDock, gadget_id));

    // Make sure that the floating gadget will always on top of the sidebar.
    gtk_window_set_transient_for(GTK_WINDOW(view_host->GetWindow()),
                                 GTK_WINDOW(sidebar_host_->GetWindow()));
    return decorator;
  }

  void LoadGadgetOptions(Gadget *gadget) {
    OptionsInterface *opt = gadget->GetOptions();
    Variant value = opt->GetInternalValue(kDisplayTarget);
    int target;
    if (value.ConvertToInt(&target) && target < Gadget::TARGET_INVALID)
      gadget->SetDisplayTarget(static_cast<Gadget::DisplayTarget>(target));
    else  // default value is TARGET_SIDEBAR
      gadget->SetDisplayTarget(Gadget::TARGET_SIDEBAR);
    value = opt->GetInternalValue(kPositionInSideBar);
    value.ConvertToDouble(&gadgets_[gadget->GetInstanceID()]->y_in_sidebar);
  }

  ViewHostInterface *NewViewHost(Gadget *gadget,
                                 ViewHostInterface::Type type) {
    if (!gadget) return NULL;
    int id = gadget->GetInstanceID();
    if (gadgets_.find(id) == gadgets_.end() || !gadgets_[id]) {
      gadgets_[id] = new GadgetViewHostInfo(gadget);
    }
    if (gadgets_[id]->gadget != gadget)
      gadgets_[id]->Reset(gadget);

    ViewHostInterface *view_host;
    DecoratedViewHost *decorator;
    switch (type) {
      case ViewHostInterface::VIEW_HOST_MAIN:
        LoadGadgetOptions(gadget);
        if (gadget->GetDisplayTarget() == Gadget::TARGET_SIDEBAR) {
          view_host = sidebar_->NewViewHost(gadgets_[id]->y_in_sidebar);
          decorator = new DecoratedViewHost(view_host,
                                            DecoratedViewHost::MAIN_DOCKED,
                                            transparent_);
          gadgets_[id]->decorated_view_host = decorator;
          decorator->ConnectOnUndock(
              NewSlot(this, &Impl::HandleFloatingUndock, id));
          decorator->ConnectOnPopOut(
              NewSlot(this, &Impl::OnPopOutHandler, decorator));
          decorator->ConnectOnPopIn(
              NewSlot(this, &Impl::OnPopInHandler, decorator));
        } else {
          return NewSingleViewHost(id);
        }
        break;
      case ViewHostInterface::VIEW_HOST_OPTIONS:
        // No decorator for options view.
        view_host = new SingleViewHost(type, 1.0, true, false, true,
                                       view_debug_mode_);
        return view_host;
      default:
        {
          SingleViewHost *sv = new SingleViewHost(type, 1.0, decorated_, false,
                                                  false, view_debug_mode_);
          gadgets_[id]->details_view_host = sv;
          sv->ConnectOnShowHide(NewSlot(this, &Impl::HandleDetailsViewShow, id));
          sv->ConnectOnResized(NewSlot(this, &Impl::HandleDetailsViewResize, id));
          sv->ConnectOnBeginResizeDrag(
              NewSlot(this, &Impl::HandlePopOutBeginResizeDrag));
          sv->ConnectOnBeginMoveDrag(NewSlot(this, &Impl::HandlePopoutViewMove));
          decorator = new DecoratedViewHost(sv, DecoratedViewHost::DETAILS,
                                            transparent_);
          // record the opened details view
          if (gadget->GetDisplayTarget() == Gadget::TARGET_SIDEBAR) {
            sidebar_->SetPopOutedView(gadget->GetMainView());
            details_view_opened_gadget_ = gadget;
          }
        }
        break;
    }
    decorator->ConnectOnClose(NewSlot(this, &Impl::OnCloseHandler, decorator));
    return decorator;
  }

  void RemoveGadget(Gadget *gadget, bool save_data) {
    ASSERT(gadget);
    // If this gadget is popped out, popin it first.
    ViewInterface *main_view = gadget->GetMainView();
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
        CloseDetailsView(gadget->GetInstanceID());
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
      GadgetViewHostInfo *info = gadgets_[child->GetGadget()->GetInstanceID()];
      CloseDetailsView(child->GetGadget()->GetInstanceID());
      sidebar_->SetPopOutedView(child);
      expanded_original_ = decorated;
      SingleViewHost *svh = new SingleViewHost(
          ViewHostInterface::VIEW_HOST_MAIN, 1.0, false, false, false,
          static_cast<ViewInterface::DebugMode>(view_debug_mode_));
      info->pop_out_view_host = svh;
      svh->ConnectOnBeginMoveDrag(NewSlot(this, &Impl::HandlePopoutViewMove));
      svh->ConnectOnBeginResizeDrag(
          NewSlot(this, &Impl::HandlePopOutBeginResizeDrag));
      svh->ConnectOnResized(NewSlot(this, &Impl::HandlePopOutViewResized,
                                    child->GetGadget()->GetInstanceID()));

      expanded_popout_ =
          new DecoratedViewHost(svh, DecoratedViewHost::MAIN_EXPANDED,
                                transparent_);
      expanded_popout_->ConnectOnClose(NewSlot(this, &Impl::OnCloseHandler,
                                               expanded_popout_));

      // Send popout event to decorator first.
      SimpleEvent event(Event::EVENT_POPOUT);
      expanded_original_->GetDecoratedView()->OnOtherEvent(event);

      child->SwitchViewHost(expanded_popout_);
      SetPopoutPosition(child->GetGadget()->GetInstanceID(), svh);
      expanded_popout_->ShowView(false, 0, NULL);
    }
  }

  void OnPopInHandler(DecoratedViewHost *decorated) {
    if (expanded_original_ == decorated && expanded_popout_) {
      ViewInterface *child = expanded_popout_->GetView();
      ASSERT(child);
      if (child) {
        GadgetViewHostInfo *info = gadgets_[child->GetGadget()->GetInstanceID()];
        expanded_popout_->CloseView();
        ViewHostInterface *old_host = child->SwitchViewHost(expanded_original_);
        SimpleEvent event(Event::EVENT_POPIN);
        expanded_original_->GetDecoratedView()->OnOtherEvent(event);
        // The old host must be destroyed after sending onpopin event.
        old_host->Destroy();
        expanded_original_ = NULL;
        expanded_popout_ = NULL;
        info->pop_out_view_host = NULL;
        sidebar_->SetPopOutedView(NULL);
      }
    }

    if (details_view_opened_gadget_) {
      CloseDetailsView(details_view_opened_gadget_->GetInstanceID());
      details_view_opened_gadget_ = NULL;
    }
  }

  void SetPopoutPosition(int gadget_id, SingleViewHost *popout_view_host) {
    // got position
    int sx, sy;
    SingleViewHost *main = NULL;
    GadgetViewHostInfo *info = gadgets_[gadget_id];
    if (info->details_view_host == popout_view_host) {
      if (info->gadget->GetDisplayTarget() == Gadget::TARGET_SIDEBAR)
        main = info->pop_out_view_host;
      else
        main = info->floating_view_host;
    }
    if (!main) {
      sidebar_host_->GetWindowPosition(&sx, &sy);
      double ex, ey;
      ViewElement *element =
          sidebar_->FindViewElementByView(info->gadget->GetMainView());
      element->SelfCoordToViewCoord(0, 0, &ex, &ey);
      sy += static_cast<int>(ey);
    } else {
      main->GetWindowPosition(&sx, &sy);
    }

    int pw = static_cast<int>(popout_view_host->GetView()->GetWidth());
    if ((option_sidebar_position_ == SIDEBAR_POSITION_RIGHT &&
         info->gadget->GetDisplayTarget() == Gadget::TARGET_SIDEBAR) ||
        (main && sx > pw &&
         info->gadget->GetDisplayTarget() != Gadget::TARGET_SIDEBAR)) {
      popout_view_host->SetWindowPosition(sx - pw, sy);
    } else {
      int sw, sh;
      main ? main->GetWindowSize(&sw, &sh) :
          sidebar_host_->GetWindowSize(&sw, &sh);
      popout_view_host->SetWindowPosition(sx + sw, sy);
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

  bool HandlePopoutViewMove(int button) {
    // popout view is not allowed to move, just return true
    return true;
  }

  // handlers for menu items
  void AddGadgetHandlerWithOneArg(const char *str) {
    gadget_manager_->ShowGadgetBrowserDialog(&gadget_browser_host_);
  }

  void HandleMenuShowAll(const char *str) {
    HideOrShowAllGadgets(true);
  }

  void HandleMenuAutoHide(const char *str) {
    option_auto_hide_ = !option_auto_hide_;
    options_->PutInternalValue(kOptionAutoHide, Variant(option_auto_hide_));
  }

  void HandleMenuAlwaysOnTop(const char *str) {
    option_always_on_top_ = !option_always_on_top_;
    options_->PutInternalValue(kOptionAlwaysOnTop, Variant(option_always_on_top_));
    AdjustSideBar();
  }

  void HandleMenuReplaceSideBar(const char *str) {
    if (!strcmp(GM_("MENU_ITEM_LEFT"), str))
      option_sidebar_position_ = SIDEBAR_POSITION_LEFT;
    else
      option_sidebar_position_ = SIDEBAR_POSITION_RIGHT;
    options_->PutInternalValue(kOptionPosition, Variant(option_sidebar_position_));
    AdjustSideBar();
  }

  void HandleMenuFontSizeChange(const char *str) {
    if (!strcmp(GM_("MENU_ITEM_FONT_SIZE_LARGE"), str))
      option_font_size_ += 2;
    else if (!strcmp(GM_("MENU_ITEM_FONT_SIZE_DEFAULT"), str))
      option_font_size_ = kDefaultFontSize;
    else
      option_font_size_ -= 2;
    options_->PutInternalValue(kOptionFontSize, Variant(option_font_size_));
  }

  void HandleExit(const char *str) {
    gtk_main_quit();
    FlushGlobalOptions();
  }

  void HandleDetailsViewShow(bool show, int gadget_id) {
    if (!show) return;
    SetPopoutPosition(gadget_id, gadgets_[gadget_id]->details_view_host);
  }

  void HandleDetailsViewResize(int dump1, int dump2, int gadget_id) {
    SetPopoutPosition(gadget_id, gadgets_[gadget_id]->details_view_host);
  }

  void HandlePopOutViewResized(int dump1, int dump2, int gadget_id) {
    SetPopoutPosition(gadget_id, gadgets_[gadget_id]->pop_out_view_host);
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

  void LoadGadgets() {
    gadget_manager_->EnumerateGadgetInstances(
        NewSlot(this, &Impl::AddGadgetInstanceCallback));
  }

  bool ShouldHideSideBar() const {
    // first check if the cursor is in sidebar
    int size_x, size_y, x, y;
    gtk_widget_get_pointer(main_widget_, &x, &y);
    sidebar_host_->GetWindowSize(&size_x, &size_y);
    if (x >= 0 && y >= 0 && x <= size_x && y <= size_y) return false;

    // second check if the focus is given to the popout window
    if (expanded_popout_) {
      GtkWidget *win = gtk_widget_get_toplevel(
          GTK_WIDGET(expanded_popout_->GetNativeWidget()));
      if (gtk_window_is_active(GTK_WINDOW(win))) return false;
    }
    if (details_view_opened_gadget_) {
      GadgetsMap::const_iterator it =
          gadgets_.find(details_view_opened_gadget_->GetInstanceID());
      GtkWidget *win = it->second->details_view_host->GetWindow();
      if (gtk_window_is_active(GTK_WINDOW(win))) return false;
    }
    return true;
  }

  // gtk call-backs
  static gboolean HandleFocusOutEvent(GtkWidget *widget, GdkEventFocus *event,
                                      Impl *this_p) {
    if (this_p->option_auto_hide_) {
      if (this_p->ShouldHideSideBar()) {
        this_p->HideOrShowSideBar(false);
      } else {
        this_p->auto_hide_source_ =
            g_timeout_add(kAutoHideTimeout, HandleAutoHideTimeout, this_p);
      }
    }
    return FALSE;
  }

  static gboolean HandleAutoHideTimeout(gpointer user_data) {
    Impl *this_p = reinterpret_cast<Impl *>(user_data);
    if (this_p->ShouldHideSideBar()) {
      this_p->HideOrShowSideBar(false);
      this_p->auto_hide_source_ = 0;
      return FALSE;
    }
    return TRUE;
  }

  static gboolean HandleFocusInEvent(GtkWidget *widget, GdkEventFocus *event,
                                     Impl *this_p) {
    if (this_p->auto_hide_source_) {
      g_source_remove(this_p->auto_hide_source_);
      this_p->auto_hide_source_ = 0;
    }
    if (!this_p->sidebar_shown_)
      this_p->HideOrShowSideBar(true);
    return FALSE;
  }

  static gboolean HandleEnterNotifyEvent(GtkWidget *widget,
                                         GdkEventCrossing *event, Impl *this_p) {
    if (this_p->option_auto_hide_ && !this_p->sidebar_shown_)
      g_timeout_add(kAutoShowTimeout, HandleAutoShowTimeout, this_p);
    return FALSE;
  }

  static gboolean HandleAutoShowTimeout(gpointer user_data) {
    Impl *this_p = reinterpret_cast<Impl *>(user_data);
    if (!this_p->ShouldHideSideBar())
      this_p->HideOrShowSideBar(true);
    return FALSE;
  }

  static gboolean HandleDragMove(GtkWidget *widget,
                                 GdkEventMotion *event, Impl *impl) {
    if (impl->sidebar_moving_) {
      impl->HandleSideBarMove();
    } else if (impl->draging_gadget_) {
      impl->HandleViewHostMove(impl->draging_gadget_->GetInstanceID());
    }
    return FALSE;
  }

  static gboolean HandleDragEnd(GtkWidget *widget,
                                GdkEventMotion *event, Impl *impl) {
    gdk_pointer_ungrab(event->time);
    if (impl->sidebar_moving_) {
      impl->HandleSideBarEndMoveDrag();
    } else {
      ASSERT(impl->draging_gadget_);
      impl->HandleViewHostEndMoveDrag(impl->draging_gadget_->GetInstanceID());
    }
    return FALSE;
  }

#if GTK_CHECK_VERSION(2,10,0)
  static void ToggleAllGadgetsHandler(GtkWidget *widget, Impl *this_p) {
    this_p->HideOrShowAllGadgets(!this_p->gadgets_shown_);
  }

  static void StatusIconPopupMenuHandler(GtkWidget *widget, guint button,
                                         guint activate_time, Impl *this_p) {
    this_p->sidebar_->GetSideBarViewHost()->ShowContextMenu(
        MouseEvent::BUTTON_LEFT);
  }
#endif

 public:  // members
  GadgetBrowserHost gadget_browser_host_;

  typedef std::map<int, GadgetViewHostInfo *> GadgetsMap;
  GadgetsMap gadgets_;

  SideBarGtkHost *owner_;

  bool decorated_;
  bool gadgets_shown_;
  bool sidebar_shown_;
  bool transparent_;
  int view_debug_mode_;

  SingleViewHost *sidebar_host_;
  DecoratedViewHost *expanded_original_;
  DecoratedViewHost *expanded_popout_;
  Gadget *details_view_opened_gadget_;
  Gadget *draging_gadget_;
  GtkWidget *drag_observer_;
  GdkRectangle workarea_;

  double floating_offset_x_;
  double floating_offset_y_;
  bool   sidebar_moving_;

  bool has_strut_;

  SideBar *sidebar_;

  OptionsInterface *options_;
  //TODO: add another option for visible in all workspace?
  bool option_auto_hide_;
  bool option_always_on_top_;
  int  option_font_size_;
  int  option_sidebar_monitor_;
  int  option_sidebar_position_;
  int  option_sidebar_width_;

  int auto_hide_source_;

  GdkAtom net_wm_strut_;
  GdkAtom net_wm_strut_partial_;

  GadgetManagerInterface *gadget_manager_;
#if GTK_CHECK_VERSION(2,10,0)
  GtkStatusIcon *status_icon_;
#endif
  GtkWidget *main_widget_;
};

SideBarGtkHost::SideBarGtkHost(bool decorated, int view_debug_mode)
  : impl_(new Impl(this, decorated, view_debug_mode)) {
  impl_->SetupUI();
  impl_->InitGadgets();
  impl_->sidebar_host_->ShowView(false, 0, NULL);
}

SideBarGtkHost::~SideBarGtkHost() {
  delete impl_;
  impl_ = NULL;
}

ViewHostInterface *SideBarGtkHost::NewViewHost(Gadget *gadget,
                                               ViewHostInterface::Type type) {
  return impl_->NewViewHost(gadget, type);
}

void SideBarGtkHost::RemoveGadget(Gadget *gadget, bool save_data) {
  return impl_->RemoveGadget(gadget, save_data);
}

void SideBarGtkHost::DebugOutput(DebugLevel level, const char *message) const {
  impl_->DebugOutput(level, message);
}

bool SideBarGtkHost::OpenURL(const char *url) const {
  return ggadget::gtk::OpenURL(url);
}

bool SideBarGtkHost::LoadFont(const char *filename) {
  return ggadget::gtk::LoadFont(filename);
}

void SideBarGtkHost::ShowGadgetAboutDialog(ggadget::Gadget *gadget) {
  ggadget::gtk::ShowGadgetAboutDialog(gadget);
}

void SideBarGtkHost::Run() {
  impl_->LoadGadgets();
  gtk_main();
}

} // namespace gtk
} // namespace hosts
