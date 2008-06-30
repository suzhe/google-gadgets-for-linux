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
#include <ggadget/gtk/menu_builder.h>
#include <ggadget/gtk/hotkey.h>
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
#include <ggadget/slot.h>

#include "gadget_browser_host.h"

using namespace ggadget;
using namespace ggadget::gtk;

namespace hosts {
namespace gtk {

static const char kOptionAutoHide[]       = "auto_hide";
static const char kOptionAlwaysOnTop[]    = "always_on_top";
static const char kOptionPosition[]       = "position";
static const char kOptionFontSize[]       = "font_size";
static const char kOptionWidth[]          = "width";
static const char kOptionMonitor[]        = "monitor";
static const char kOptionHotKey[]         = "hotkey";
static const char kOptionSideBarShown[]   = "sidebar_shown";

static const char kOptionDisplayTarget[]  = "display_target";
static const char kOptionPositionInSideBar[] = "position_in_sidebar";

static const int kAutoHideTimeout         = 200;
static const int kAutoShowTimeout         = 500;
static const int kDefaultFontSize         = 14;
static const int kDefaultSideBarWidth     = 200;
static const int kDefaultMonitor          = 0;
static const int kSideBarMinimizedHeight  = 28;
static const int kSideBarMinimizedWidth   = 3;
static const int kDefaultRulerHeight      = 1;
static const int kDefaultRulerWidth       = 1;

enum SideBarPosition {
  SIDEBAR_POSITION_LEFT,
  SIDEBAR_POSITION_RIGHT,
};

class SideBarGtkHost::Impl {
 public:
  struct GadgetViewHostInfo {
    GadgetViewHostInfo(Gadget *g) {
      Reset(g);
    }
    ~GadgetViewHostInfo() {
      if (debug_console)
        gtk_widget_destroy(debug_console);
      delete gadget;
      gadget = NULL;
    }
    void Reset(Gadget *g) {
      gadget = g;
      decorated_view_host = NULL;
      details_view_host = NULL;
      floating_view_host = NULL;
      pop_out_view_host = NULL;
      index_in_sidebar = 0;
      undock_by_drag = false;
      old_keep_above = false;
      debug_console = NULL;
    }

    Gadget *gadget;

    DecoratedViewHost *decorated_view_host;
    SingleViewHost *details_view_host;
    SingleViewHost *floating_view_host;
    SingleViewHost *pop_out_view_host;

    int  index_in_sidebar;
    bool undock_by_drag;
    bool old_keep_above;
    GtkWidget *debug_console;
  };

  Impl(SideBarGtkHost *owner, OptionsInterface *options,
       bool decorated, int view_debug_mode, int debug_console_config)
    : gadget_browser_host_(owner, view_debug_mode),
      owner_(owner),
      decorated_(decorated),
      sidebar_shown_(true),
      transparent_(false),
      view_debug_mode_(view_debug_mode),
      debug_console_config_(debug_console_config),
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
      options_(options),
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
#if GTK_CHECK_VERSION(2,10,0) && defined(GGL_HOST_LINUX)
      status_icon_(NULL),
      status_icon_menu_(NULL),
#endif
      main_widget_(NULL),
      hotkey_grabber_(NULL) {
    ASSERT(gadget_manager_);
    ASSERT(options_);

    hotkey_grabber_.ConnectOnHotKeyPressed(
        NewSlot(this, &Impl::ToggleAllGadgets));

    workarea_.x = 0;
    workarea_.y = 0;
    workarea_.width = 0;
    workarea_.height = 0;

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

#if GTK_CHECK_VERSION(2,10,0) && defined(GGL_HOST_LINUX)
    g_object_unref(G_OBJECT(status_icon_));
    if (status_icon_menu_)
      gtk_widget_destroy(status_icon_menu_);
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
      AdjustSideBar(false);
  }

  // SideBar handlers
  bool HandleSideBarBeginResizeDrag(int button, int hittest) {
    if (sidebar_shown_ && button == MouseEvent::BUTTON_LEFT &&
        ((hittest == ViewInterface::HT_LEFT &&
          option_sidebar_position_ == SIDEBAR_POSITION_RIGHT) ||
         (hittest == ViewInterface::HT_RIGHT &&
          option_sidebar_position_ == SIDEBAR_POSITION_LEFT)))
      return false;

    // Don't allow resize drag in any other situation.
    return true;
  }

  void HandleSideBarEndResizeDrag() {
    if (has_strut_)
      AdjustSideBar(false);
  }

  bool HandleSideBarBeginMoveDrag(int button) {
    if (button != MouseEvent::BUTTON_LEFT || draging_gadget_)
      return true;
    if (gdk_pointer_grab(drag_observer_->window, FALSE,
                         (GdkEventMask)(GDK_BUTTON_RELEASE_MASK |
                                        GDK_POINTER_MOTION_MASK),
                         NULL, NULL, gtk_get_current_event_time()) ==
        GDK_GRAB_SUCCESS) {
      DLOG("HandleSideBarBeginMoveDrag");
      int x, y;
      gtk_widget_get_pointer(main_widget_, &x, &y);
      sidebar_host_->SetWindowType(GDK_WINDOW_TYPE_HINT_DOCK);
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
    DLOG("HandleSideBarEndMoveDrag, sidebar_shown_: %d", sidebar_shown_);
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
    sidebar_moving_ = false;
    if (sidebar_shown_)
      AdjustSideBar(false);
  }

  void HandleSideBarShow(bool show) {
    if (show) AdjustSideBar(false);
  }

  void HandleAddGadget() {
    gadget_manager_->ShowGadgetBrowserDialog(&gadget_browser_host_);
  }

  bool HandleMenuOpen(MenuInterface *menu) {
    int priority = MenuInterface::MENU_ITEM_PRI_HOST;
    menu->AddItem(GM_("MENU_ITEM_ADD_GADGETS"), 0,
                  NewSlot(this, &Impl::AddGadgetHandlerWithOneArg), priority);
    menu->AddItem(NULL, 0, NULL, priority);
    if (!sidebar_shown_)
      menu->AddItem(GM_("MENU_ITEM_SHOW_ALL"), 0,
                    NewSlot(this, &Impl::HandleMenuHideOrShowAll), priority);
    else
      menu->AddItem(GM_("MENU_ITEM_HIDE_ALL"), 0,
                    NewSlot(this, &Impl::HandleMenuHideOrShowAll), priority);

    menu->AddItem(GM_("MENU_ITEM_AUTO_HIDE"),
                  option_auto_hide_ ? MenuInterface::MENU_ITEM_FLAG_CHECKED : 0,
                  NewSlot(this, &Impl::HandleMenuAutoHide), priority);
    menu->AddItem(GM_("MENU_ITEM_ALWAYS_ON_TOP"), option_always_on_top_ ?
                  MenuInterface::MENU_ITEM_FLAG_CHECKED : 0,
                  NewSlot(this, &Impl::HandleMenuAlwaysOnTop), priority);
    menu->AddItem(GM_("MENU_ITEM_CHANGE_HOTKEY"), 0,
                  NewSlot(this, &Impl::HandleChangeHotKey), priority);

    {
      MenuInterface *sub = menu->AddPopup(GM_("MENU_ITEM_DOCK_SIDEBAR"),
                                          priority);
      sub->AddItem(GM_("MENU_ITEM_LEFT"),
                   option_sidebar_position_ == SIDEBAR_POSITION_LEFT ?
                   MenuInterface::MENU_ITEM_FLAG_CHECKED : 0,
                   NewSlot(this, &Impl::HandleMenuPositionSideBar), priority);
      sub->AddItem(GM_("MENU_ITEM_RIGHT"),
                   option_sidebar_position_ == SIDEBAR_POSITION_RIGHT ?
                   MenuInterface::MENU_ITEM_FLAG_CHECKED : 0,
                   NewSlot(this, &Impl::HandleMenuPositionSideBar), priority);
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
    HideOrShowSideBar(!sidebar_shown_);
  }

  void HandleSizeEvent() {
    // ignore width changes when the sidebar is hiden
    int  width = static_cast<int>(sidebar_->GetWidth());
    if (width > kSideBarMinimizedWidth) {
      option_sidebar_width_ = width;
      DLOG("set option_sidebar_width_ to %d", option_sidebar_width_);
    }
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
        sidebar_->InsertPlaceholder(info->index_in_sidebar, h);
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
        info->old_keep_above = info->floating_view_host->IsKeepAbove();
        info->floating_view_host->SetKeepAbove(true);
        gdk_window_raise(info->floating_view_host->GetWindow()->window);
      }
    }
  }

  void HandleGeneralPopIn() {
    OnPopInHandler(expanded_original_);
  }

  // option load / save methods
  void LoadGlobalOptions() {
    Variant value;
    value = options_->GetInternalValue(kOptionAutoHide);
    value.ConvertToBool(&option_auto_hide_);
    value = options_->GetInternalValue(kOptionAlwaysOnTop);
    value.ConvertToBool(&option_always_on_top_);
    value = options_->GetInternalValue(kOptionPosition);
    value.ConvertToInt(&option_sidebar_position_);
    value = options_->GetInternalValue(kOptionWidth);
    value.ConvertToInt(&option_sidebar_width_);
    value = options_->GetInternalValue(kOptionMonitor);
    value.ConvertToInt(&option_sidebar_monitor_);
    value = options_->GetInternalValue(kOptionFontSize);
    value.ConvertToInt(&option_font_size_);

    std::string hotkey;
    if (options_->GetInternalValue(kOptionHotKey).ConvertToString(&hotkey) &&
        hotkey.length()) {
      hotkey_grabber_.SetHotKey(hotkey);
      hotkey_grabber_.SetEnableGrabbing(true);
    }

    // The default value of sidebar_shown_ is true.
    value = options_->GetInternalValue(kOptionSideBarShown);
    if (value.type() == Variant::TYPE_BOOL)
      sidebar_shown_ = VariantValue<bool>()(value);
  }

  bool FlushGadgetOrder(int index, ViewElement *view_element) {
    Gadget *gadget = view_element->GetChildView()->GetGadget();
    OptionsInterface *opt = gadget->GetOptions();
    opt->PutInternalValue(kOptionPositionInSideBar, Variant(index));
    return true;
  }

  void FlushGlobalOptions() {
    // save gadgets' information
    for (GadgetsMap::iterator it = gadgets_.begin();
         it != gadgets_.end(); ++it) {
      OptionsInterface *opt = it->second->gadget->GetOptions();
      opt->PutInternalValue(kOptionDisplayTarget,
                            Variant(it->second->gadget->GetDisplayTarget()));
    }
    sidebar_->EnumerateViewElements(NewSlot(this, &Impl::FlushGadgetOrder));

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
    options_->PutInternalValue(kOptionSideBarShown, Variant(sidebar_shown_));
    options_->PutInternalValue(kOptionHotKey,
                               Variant(hotkey_grabber_.GetHotKey()));
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

#if GTK_CHECK_VERSION(2,10,0) && defined(GGL_HOST_LINUX)
    std::string icon_data;
    if (GetGlobalFileManager()->ReadFile(kGadgetsIcon, &icon_data)) {
      GdkPixbuf *icon_pixbuf = LoadPixbufFromData(icon_data);
      status_icon_ = gtk_status_icon_new_from_pixbuf(icon_pixbuf);
      g_object_unref(icon_pixbuf);
    } else {
      status_icon_ = gtk_status_icon_new_from_stock(GTK_STOCK_ABOUT);
    }
    gtk_status_icon_set_tooltip(status_icon_, GM_("GOOGLE_GADGETS"));
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

#if GTK_CHECK_VERSION(2,10,0) && defined(GGL_HOST_LINUX)
  void UpdateStatusIconTooltip() {
    if (hotkey_grabber_.IsGrabbing()) {
      gtk_status_icon_set_tooltip(status_icon_,
          StringPrintf(GM_("STATUS_ICON_TOOLTIP_WITH_HOTKEY"),
                       hotkey_grabber_.GetHotKey().c_str()).c_str());
    } else {
      gtk_status_icon_set_tooltip(status_icon_, GM_("STATUS_ICON_TOOLTIP"));
    }
  }
#endif

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

  bool EnumerateGadgetInstancesCallback(int id) {
    AddGadgetInstanceCallback(id); // Ignore the error.
    return true;
  }

  bool NewGadgetInstanceCallback(int id) {
    if (gadget_manager_->IsGadgetInstanceTrusted(id) ||
        ConfirmGadget(id)) {
      return AddGadgetInstanceCallback(id);
    }
    return false;
  }

  bool AddGadgetInstanceCallback(int id) {
    bool result = false;
    std::string options = gadget_manager_->GetGadgetInstanceOptionsName(id);
    std::string path = gadget_manager_->GetGadgetInstancePath(id);
    if (options.length() && path.length()) {
      result = LoadGadget(path.c_str(), options.c_str(), id);
      if (result) {
        DLOG("SideBarGtkHost: Load gadget %s, with option %s, succeeded",
             path.c_str(), options.c_str());
      } else {
        LOG("SideBarGtkHost: Load gadget %s, with option %s, failed",
             path.c_str(), options.c_str());
      }
    }
    return result;
  }

  void AdjustSideBar(bool hide) {
    int width = hide ? kSideBarMinimizedWidth : option_sidebar_width_;

    sidebar_host_->SetKeepAbove(option_always_on_top_);

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

    int x = 0;
    if (option_sidebar_position_ == SIDEBAR_POSITION_LEFT) {
      x = std::max(monitor_geometry.x, workarea_.x);
    } else {
      x = std::min(monitor_geometry.x + monitor_geometry.width,
                   workarea_.x + workarea_.width) - width;
    }

    if (hide) {
      DLOG("Set SideBar size: %dx%d and position: %dx%d",
           width, workarea_.height, x, workarea_.y);
      gdk_window_move_resize(main_widget_->window,
                             x, workarea_.y, width, workarea_.height);
    } else {
      DLOG("Set SideBar size: %dx%d", width, workarea_.height);
      sidebar_->SetSize(width, workarea_.height);

      DLOG("move sidebar to %dx%d", x, workarea_.y);
      sidebar_host_->SetWindowPosition(x, workarea_.y);
    }

    // if sidebar is on the edge, do strut
    if (option_always_on_top_ && !option_auto_hide_ &&
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
        struts[0] = x + width;
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
      if (has_strut_) {
        has_strut_ = false;
        gdk_property_delete(main_widget_->window, net_wm_strut_);
        gdk_property_delete(main_widget_->window, net_wm_strut_partial_);
      }
      if (!option_always_on_top_) {
        sidebar_host_->SetWindowType(GDK_WINDOW_TYPE_HINT_NORMAL);
      }
    }

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
      if (details_view_opened_gadget_ &&
          details_view_opened_gadget_->GetInstanceID() == gadget_id)
        sidebar_->SetPopOutedView(NULL);
    }
  }

  bool Dock(int gadget_id, bool force_insert) {
    GadgetViewHostInfo *info = gadgets_[gadget_id];
    ASSERT(info);
    ViewHostInterface *view_host = sidebar_->NewViewHost(info->index_in_sidebar);
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
    // Display target must be set after switching to the new view host and
    // before destroying the old view host.
    // Browser element relies on it to reparent the browser widget.
    // Otherwise the browser widget might be destroyed along with the old view
    // host.
    info->gadget->SetDisplayTarget(Gadget::TARGET_SIDEBAR);
    if (old) old->Destroy();
    view_host->ShowView(false, 0, NULL);
    info->floating_view_host = NULL;
    return true;
  }

  bool Undock(int gadget_id, bool move_to_cursor) {
    GadgetViewHostInfo *info = gadgets_[gadget_id];
    CloseDetailsView(gadget_id);
    double view_x, view_y;
    View *view = info->gadget->GetMainView();
    ViewElement *view_element = sidebar_->FindViewElementByView(view);
    view_element->SelfCoordToViewCoord(0, 0, &view_x, &view_y);
    info->index_in_sidebar = sidebar_->GetIndexFromHeight(view_y);
    DecoratedViewHost *new_host = NewSingleViewHost(gadget_id);
    if (move_to_cursor)
      new_host->EnableAutoRestoreViewSize(false);
    ViewHostInterface *old = view->SwitchViewHost(new_host);
    // Display target must be set after switching to the new view host and
    // before destroying the old view host.
    // Browser element relies on it to reparent the browser widget.
    // Otherwise the browser widget might be destroyed along with the old view
    // host.
    // In drag undock mode, the display target will be set until the end of
    // move drag.
    if (!move_to_cursor)
      info->gadget->SetDisplayTarget(Gadget::TARGET_FLOATING_VIEW);
    if (old) old->Destroy();
    // ShowView will be called in HandleFloatingUndock() or HandleUndock().
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
      info->old_keep_above = info->floating_view_host->IsKeepAbove();
      info->floating_view_host->SetKeepAbove(true);

      // Raise the sidebar window to make sure that there is no other window on
      // top of sidebar window.
      gdk_window_raise(main_widget_->window);

      // Raise gadget window after raising sidebar window, to make sure it's on
      // top of sidebar window.
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
      // show sidebar first if it is auto hiden
      // note that we don't use flag sidebar_shown_ to judge if sidebar is
      // shown, since resize action is async in GTK, so the status of the flag
      // may not be right.
      sidebar_host_->GetWindowSize(&x, &y);
      if (option_auto_hide_ && x <= kSideBarMinimizedWidth) {
        HideOrShowSideBar(true);
        info->floating_view_host->SetKeepAbove(true);
        gdk_window_raise(info->floating_view_host->GetWindow()->window);
      }

      sidebar_->InsertPlaceholder(
          sidebar_->GetIndexFromHeight(h),
          info->floating_view_host->GetView()->GetHeight());
      info->index_in_sidebar = sidebar_->GetIndexFromHeight(h);
    } else {
      sidebar_->ClearPlaceHolder();
    }
  }

  void HandleViewHostEndMoveDrag(int gadget_id) {
    int h, x, y;
    GadgetViewHostInfo *info = gadgets_[gadget_id];
    ASSERT(info);
    gdk_display_get_pointer(gdk_display_get_default(), NULL, &x, &y, NULL);
    // The floating window must be normal window when not dragging,
    // otherwise it'll always on top.
    info->floating_view_host->SetWindowType(GDK_WINDOW_TYPE_HINT_NORMAL);
    info->floating_view_host->SetKeepAbove(info->old_keep_above);
    if (IsOverlapWithSideBar(gadget_id, &h)) {
      info->index_in_sidebar = sidebar_->GetIndexFromHeight(h);
      HandleDock(gadget_id);
      // update the index for all elements in sidebar after dock by drag
      sidebar_->UpdateElememtsIndex();
    } else if (info->undock_by_drag) {
      // In drag undock mode, Undock() will not set the display target.
      info->gadget->SetDisplayTarget(Gadget::TARGET_FLOATING_VIEW);
      info->decorated_view_host->EnableAutoRestoreViewSize(true);
      info->decorated_view_host->RestoreViewSize();
      info->undock_by_drag = false;
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
    if ((x + w >= sx) && (sx + sw >= x) && (y + h >= sy) && (sy + sh >= y)) {
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

    SingleViewHost *vh = gadgets_[gadget_id]->floating_view_host;
    vh->ShowView(false, 0, NULL);

    // Move the floating gadget to the center of the monitor, if the gadget
    // window overlaps with sidebar window.
    if (IsOverlapWithSideBar(gadget_id, NULL)) {
      GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(main_widget_));
      GdkRectangle rect;
      gdk_screen_get_monitor_geometry(screen, option_sidebar_monitor_, &rect);
      int width, height;
      vh->GetWindowSize(&width, &height);
      int x = (rect.x + (rect.width - width) / 2);
      int y = (rect.y + (rect.height - height) / 2);

      vh->SetWindowPosition(x, y);
    }
  }

  void HideOrShowAllGadgets(bool show) {
    DLOG("HideOrShowAllGadgets");
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

    if (sidebar_shown_ != show)
      HideOrShowSideBar(show);
  }

  void HideOrShowSideBar(bool show) {
    sidebar_shown_ = show;
#if GTK_CHECK_VERSION(2,10,0) && defined(GGL_HOST_LINUX)
    if (show) {
      AdjustSideBar(false);
      sidebar_host_->ShowView(false, 0, NULL);
    } else {
      if (option_auto_hide_) {
        AdjustSideBar(true);
      } else {
        sidebar_host_->CloseView();
      }
    }
#else
    if (show) {
      AdjustSideBar(false);
      sidebar_host_->ShowView(false, 0, NULL);
    } else {
      sidebar_->SetSize(option_sidebar_width_, kSideBarMinimizedHeight);
      if (has_strut_) {
        gdk_property_delete(main_widget_->window, net_wm_strut_);
        gdk_property_delete(main_widget_->window, net_wm_strut_partial_);
      }
    }
#endif
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

    if (gadget->GetDisplayTarget() == Gadget::TARGET_SIDEBAR || sidebar_shown_)
      gadget->ShowMainView();

    if (gadget->GetDisplayTarget() == Gadget::TARGET_SIDEBAR)
      it->second->decorated_view_host->SetDockEdge(
          option_sidebar_position_ == SIDEBAR_POSITION_RIGHT);

    // If debug console is opened during view host creation, the title is
    // not set then because main view is not available. Set the title now.
    if (it->second->debug_console) {
      gtk_window_set_title(GTK_WINDOW(it->second->debug_console),
                           gadget->GetMainView()->GetCaption().c_str());
    }

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
    return decorator;
  }

  void LoadGadgetOptions(Gadget *gadget) {
    OptionsInterface *opt = gadget->GetOptions();
    Variant value = opt->GetInternalValue(kOptionDisplayTarget);
    int target;
    if (value.ConvertToInt(&target) && target < Gadget::TARGET_INVALID)
      gadget->SetDisplayTarget(static_cast<Gadget::DisplayTarget>(target));
    else  // default value is TARGET_SIDEBAR
      gadget->SetDisplayTarget(Gadget::TARGET_SIDEBAR);
    value = opt->GetInternalValue(kOptionPositionInSideBar);
    value.ConvertToInt(&gadgets_[gadget->GetInstanceID()]->index_in_sidebar);
  }

  ViewHostInterface *NewViewHost(Gadget *gadget,
                                 ViewHostInterface::Type type) {
    if (!gadget) return NULL;
    int id = gadget->GetInstanceID();
    if (gadgets_.find(id) == gadgets_.end() || !gadgets_[id]) {
      gadgets_[id] = new GadgetViewHostInfo(gadget);
    }
    if (gadgets_[id]->gadget != gadget) {
      // How will this occur?
      gadgets_[id]->Reset(gadget);
    }

    ViewHostInterface *view_host;
    DecoratedViewHost *decorator;
    switch (type) {
      case ViewHostInterface::VIEW_HOST_MAIN:
        if (debug_console_config_ >= 2)
          ShowGadgetDebugConsole(gadget);

        LoadGadgetOptions(gadget);
        if (gadget->GetDisplayTarget() == Gadget::TARGET_SIDEBAR) {
          view_host = sidebar_->NewViewHost(gadgets_[id]->index_in_sidebar);
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
        if (details_view_opened_gadget_ == gadget) {
          CloseDetailsView(gadget->GetInstanceID());
          details_view_opened_gadget_ = NULL;
          sidebar_->SetPopOutedView(NULL);
        }
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

  void HandleMenuHideOrShowAll(const char *str) {
    HideOrShowAllGadgets(!sidebar_shown_);
  }

  void HandleMenuAutoHide(const char *str) {
    option_auto_hide_ = !option_auto_hide_;
    options_->PutInternalValue(kOptionAutoHide, Variant(option_auto_hide_));

    // always on top if auto hide is chosen. Since the sidebar could not
    // "autoshow" if it is not always on top
    if (option_auto_hide_) {
      option_always_on_top_ = true;
      options_->PutInternalValue(kOptionAlwaysOnTop,
                                 Variant(option_always_on_top_));
    }
    HideOrShowSideBar(true);
  }

  void HandleMenuAlwaysOnTop(const char *str) {
    option_always_on_top_ = !option_always_on_top_;
    options_->PutInternalValue(kOptionAlwaysOnTop, Variant(option_always_on_top_));

    // uncheck auto hide too if "always on top" is unchecked.
    if (!option_always_on_top_) {
      option_auto_hide_ = false;
      options_->PutInternalValue(kOptionAutoHide, Variant(option_auto_hide_));
    }
    HideOrShowSideBar(true);
  }

  void HandleChangeHotKey(const char *) {
    HotKeyDialog dialog;
    dialog.SetHotKey(hotkey_grabber_.GetHotKey());
    hotkey_grabber_.SetEnableGrabbing(false);
    if (dialog.Show()) {
      std::string hotkey = dialog.GetHotKey();
      hotkey_grabber_.SetHotKey(hotkey);
      // The hotkey will not be enabled if it's invalid.
      hotkey_grabber_.SetEnableGrabbing(true);
#if GTK_CHECK_VERSION(2,10,0) && defined(GGL_HOST_LINUX)
      UpdateStatusIconTooltip();
#endif
    }
  }

  void HandleMenuPositionSideBar(const char *str) {
    if (!strcmp(GM_("MENU_ITEM_LEFT"), str))
      option_sidebar_position_ = SIDEBAR_POSITION_LEFT;
    else
      option_sidebar_position_ = SIDEBAR_POSITION_RIGHT;
    options_->PutInternalValue(kOptionPosition, Variant(option_sidebar_position_));
    HideOrShowSideBar(true);
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

  void LoadGadgets() {
    gadget_manager_->EnumerateGadgetInstances(
        NewSlot(this, &Impl::EnumerateGadgetInstancesCallback));
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
    if (!this_p->option_auto_hide_) {
      // user unchecked "auto hide" option
      this_p->auto_hide_source_ = 0;
      return FALSE;
    }
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
    if (this_p->option_auto_hide_ && !this_p->sidebar_shown_)
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
    if (!this_p->ShouldHideSideBar()) {
      this_p->HideOrShowSideBar(true);
      if (!gtk_window_has_toplevel_focus(GTK_WINDOW(this_p->main_widget_))) {
        this_p->auto_hide_source_ =
            g_timeout_add(kAutoHideTimeout, HandleAutoHideTimeout, this_p);
      }
    }
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

  void ToggleAllGadgets() {
    HideOrShowAllGadgets(!sidebar_shown_);
  }

#if GTK_CHECK_VERSION(2,10,0) && defined(GGL_HOST_LINUX)
  static void ToggleAllGadgetsHandler(GtkWidget *widget, Impl *this_p) {
    this_p->HideOrShowAllGadgets(!this_p->sidebar_shown_);
  }

  static void StatusIconPopupMenuHandler(GtkWidget *widget, guint button,
                                         guint activate_time, Impl *this_p) {
    if (this_p->status_icon_menu_)
      gtk_widget_destroy(this_p->status_icon_menu_);

    this_p->status_icon_menu_ = gtk_menu_new();
    MenuBuilder menu_builder(GTK_MENU_SHELL(this_p->status_icon_menu_));

    this_p->HandleMenuOpen(&menu_builder);
    gtk_menu_popup(GTK_MENU(this_p->status_icon_menu_), NULL, NULL,
                   gtk_status_icon_position_menu, this_p->status_icon_,
                   button, activate_time);
  }
#endif

  void ShowGadgetDebugConsole(Gadget *gadget) {
    if (!gadget)
      return;
    GadgetsMap::iterator it = gadgets_.find(gadget->GetInstanceID());
    if (it == gadgets_.end() || !it->second)
      return;
    GadgetViewHostInfo *info = it->second;
    if (info->debug_console) {
      DLOG("Gadget has already debug console opened: %p", info->debug_console);
      return;
    }
    info->debug_console = NewGadgetDebugConsole(gadget);
    g_signal_connect(info->debug_console, "destroy",
                     G_CALLBACK(gtk_widget_destroyed), &info->debug_console);
  }

 public:  // members
  GadgetBrowserHost gadget_browser_host_;

  typedef std::map<int, GadgetViewHostInfo *> GadgetsMap;
  GadgetsMap gadgets_;

  SideBarGtkHost *owner_;

  bool decorated_;
  bool sidebar_shown_;
  bool transparent_;
  int view_debug_mode_;
  int debug_console_config_;

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
#if GTK_CHECK_VERSION(2,10,0) && defined(GGL_HOST_LINUX)
  GtkStatusIcon *status_icon_;
  GtkWidget *status_icon_menu_;
#endif
  GtkWidget *main_widget_;

  HotKeyGrabber hotkey_grabber_;
};

SideBarGtkHost::SideBarGtkHost(OptionsInterface *options, bool decorated,
                               int view_debug_mode, int debug_console_config)
  : impl_(new Impl(this, options, decorated, view_debug_mode,
                   debug_console_config)) {
  impl_->SetupUI();
  impl_->InitGadgets();
#if !GTK_CHECK_VERSION(2,10,0) || !defined(GGL_HOST_LINUX)
  impl_->sidebar_host_->ShowView(false, 0, NULL);
#endif
  impl_->HideOrShowSideBar(impl_->sidebar_shown_);
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

bool SideBarGtkHost::OpenURL(const char *url) const {
  return ggadget::gtk::OpenURL(url);
}

bool SideBarGtkHost::LoadFont(const char *filename) {
  return ggadget::gtk::LoadFont(filename);
}

void SideBarGtkHost::Run() {
  impl_->LoadGadgets();
  gtk_main();
}

void SideBarGtkHost::ShowGadgetAboutDialog(Gadget *gadget) {
  ggadget::gtk::ShowGadgetAboutDialog(gadget);
}

void SideBarGtkHost::ShowGadgetDebugConsole(Gadget *gadget) {
  impl_->ShowGadgetDebugConsole(gadget);
}

} // namespace gtk
} // namespace hosts
