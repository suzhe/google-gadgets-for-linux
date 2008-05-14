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

#include <gtk/gtk.h>
#include <string>
#include <map>

#include "simple_gtk_host.h"

#include <ggadget/common.h>
#include <ggadget/decorated_view_host.h>
#include <ggadget/gadget.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/gadget_manager_interface.h>
#include <ggadget/gtk/single_view_host.h>
#include <ggadget/gtk/utilities.h>
#include <ggadget/locales.h>
#include <ggadget/messages.h>
#include <ggadget/logger.h>
#include <ggadget/script_runtime_manager.h>
#include <ggadget/view.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/file_manager_factory.h>

#include "gadget_browser_host.h"

using namespace ggadget;
using namespace ggadget::gtk;

namespace ggadget {
DECLARE_VARIANT_PTR_TYPE(DecoratedViewHost);
}

namespace hosts {
namespace gtk {

class SimpleGtkHost::Impl {
  struct GadgetViewHostInfo {
    GadgetViewHostInfo()
      : main_(NULL), popout_(NULL), details_(NULL),
        popout_on_right_(false), details_on_right_(false) {
    }

    SingleViewHost *main_;
    SingleViewHost *popout_;
    SingleViewHost *details_;
    DecoratedViewHost *main_decorator_;

    bool popout_on_right_;
    bool details_on_right_;
  };

 public:
  Impl(SimpleGtkHost *owner, double zoom, bool decorated, int view_debug_mode)
    : gadget_browser_host_(owner, view_debug_mode),
      owner_(owner),
      zoom_(zoom),
      decorated_(decorated),
      view_debug_mode_(view_debug_mode),
      gadgets_shown_(true),
      transparent_(SupportsComposite()),
      gadget_manager_(GetGadgetManager()),
      expanded_original_(NULL),
      expanded_popout_(NULL) {
    ASSERT(gadget_manager_);
    ScriptRuntimeManager::get()->ConnectErrorReporter(
        NewSlot(this, &Impl::ReportScriptError));
  }

  ~Impl() {
    for (GadgetsMap::iterator it = gadgets_.begin();
         it != gadgets_.end(); ++it)
      delete it->second;

    gtk_widget_destroy(host_menu_);
#if GTK_CHECK_VERSION(2,10,0) && defined(GGL_HOST_LINUX)
    g_object_unref(G_OBJECT(status_icon_));
#else
    gtk_widget_destroy(main_widget_);
#endif
  }

  void SetupUI() {
    GtkWidget *item;
    host_menu_ = gtk_menu_new();

    item = gtk_menu_item_new_with_label("Add gadget...");
    gtk_widget_show(item);
    g_signal_connect(G_OBJECT(item), "activate",
                     G_CALLBACK(AddGadgetHandler), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(host_menu_), item);

    item = gtk_menu_item_new_with_label("Show all gadgets");
    gtk_widget_show(item);
    g_signal_connect(G_OBJECT(item), "activate",
                     G_CALLBACK(ShowAllGadgetsHandler), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(host_menu_), item);

    item = gtk_menu_item_new_with_label("Hide all gadgets");
    gtk_widget_show(item);
    g_signal_connect(G_OBJECT(item), "activate",
                     G_CALLBACK(HideAllGadgetsHandler), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(host_menu_), item);

    item = gtk_separator_menu_item_new();
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(host_menu_), item);

    item = gtk_menu_item_new_with_label("Exit");
    gtk_widget_show(item);
    g_signal_connect(G_OBJECT(item), "activate",
                     G_CALLBACK(ExitHandler), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(host_menu_), item);

#if GTK_CHECK_VERSION(2,10,0) && defined(GGL_HOST_LINUX)
    // FIXME:
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
#else
    GtkWidget *menu_bar = gtk_menu_bar_new();
    gtk_widget_show(menu_bar);
    item = gtk_menu_item_new_with_label("Gadgets");
    gtk_widget_show(item);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), host_menu_);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), item);
    main_widget_ = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(main_widget_), "Google Gadgets");
    gtk_window_set_resizable(GTK_WINDOW(main_widget_), FALSE);
    gtk_container_add(GTK_CONTAINER(main_widget_), menu_bar);
    gtk_widget_show(main_widget_);
    g_signal_connect(G_OBJECT(main_widget_), "delete_event",
                     G_CALLBACK(DeleteEventHandler), NULL);
#endif
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
      LOG("SimpleGtkHost: Load gadget %s, with option %s, %s",
          path.c_str(), options.c_str(), result ? "succeeded" : "failed");
    }
    return true;
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

    Gadget *gadget = new Gadget(
        owner_, path, options_name, instance_id,
        gadget_manager_->IsGadgetInstanceTrusted(instance_id));

    if (!gadget->IsValid()) {
      LOG("Failed to load gadget %s", path);
      delete gadget;
      return false;
    }

    if (!gadget->ShowMainView()) {
      LOG("Failed to show main view of gadget %s", path);
      delete gadget;
      return false;
    }

    gadgets_[instance_id] = gadget;
    return true;
  }

  ViewHostInterface *NewViewHost(Gadget *gadget,
                                 ViewHostInterface::Type type) {
    ASSERT(gadget);
    int gadget_id = gadget->GetInstanceID();

    bool decorated =
        (decorated_ || type == ViewHostInterface::VIEW_HOST_OPTIONS);

    SingleViewHost *svh = new SingleViewHost(type, zoom_, decorated, false,
                                             true, view_debug_mode_);

    if (type == ViewHostInterface::VIEW_HOST_OPTIONS)
      return svh;

    DecoratedViewHost *dvh;
    if (type == ViewHostInterface::VIEW_HOST_MAIN) {
      dvh = new DecoratedViewHost(svh, DecoratedViewHost::MAIN_STANDALONE,
                                  transparent_);
      GadgetViewHostInfo *info = &view_hosts_[gadget_id];
      ASSERT(!info->main_);
      info->main_ = svh;
      info->main_decorator_ = dvh;

      svh->ConnectOnShowHide(
          NewSlot(this, &Impl::OnMainViewShowHideHandler, gadget_id));
      svh->ConnectOnResized(
          NewSlot(this, &Impl::OnMainViewResizedHandler, gadget_id));
      svh->ConnectOnMoved(
          NewSlot(this, &Impl::OnMainViewMovedHandler, gadget_id));
    } else {
      dvh = new DecoratedViewHost(svh, DecoratedViewHost::DETAILS,
                                  transparent_);
      GadgetViewHostInfo *info = &view_hosts_[gadget_id];
      ASSERT(info->main_);
      ASSERT(!info->details_);
      info->details_ = svh;

      svh->ConnectOnShowHide(
          NewSlot(this, &Impl::OnDetailsViewShowHideHandler, gadget_id));
      svh->ConnectOnBeginResizeDrag(
          NewSlot(this, &Impl::OnDetailsViewBeginResizeHandler, gadget_id));
      svh->ConnectOnResized(
          NewSlot(this, &Impl::OnDetailsViewResizedHandler, gadget_id));
      svh->ConnectOnBeginMoveDrag(
          NewSlot(this, &Impl::OnDetailsViewBeginMoveHandler, gadget_id));
    }

    dvh->ConnectOnClose(NewSlot(this, &Impl::OnCloseHandler, dvh));
    dvh->ConnectOnPopOut(NewSlot(this, &Impl::OnPopOutHandler, dvh));
    dvh->ConnectOnPopIn(NewSlot(this, &Impl::OnPopInHandler, dvh));

    return dvh;
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

  void RemoveGadgetInstanceCallback(int instance_id) {
    GadgetsMap::iterator it = gadgets_.find(instance_id);

    if (it != gadgets_.end()) {
      view_hosts_.erase(instance_id);
      delete it->second;
      gadgets_.erase(it);
    } else {
      LOG("Can't find gadget instance %d", instance_id);
    }
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
    // TODO: actual debug console.
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

  static void ShowAllGadgetsHandler(GtkWidget *widget, gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    for (GadgetViewHostsMap::iterator it = impl->view_hosts_.begin();
         it != impl->view_hosts_.end(); ++it) {
      it->second.main_->ShowView(false, 0, NULL);
    }
    impl->gadgets_shown_ = true;
  }

  static void HideAllGadgetsHandler(GtkWidget *widget, gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    for (GadgetViewHostsMap::iterator it = impl->view_hosts_.begin();
         it != impl->view_hosts_.end(); ++it) {
      it->second.main_->CloseView();
    }
    impl->gadgets_shown_ = false;
  }

  static void ExitHandler(GtkWidget *widget, gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    // Close the poppped out view, to make sure that the main view decorator
    // can save its states correctly.
    if (impl->expanded_popout_)
      impl->OnPopInHandler(impl->expanded_original_);

    gtk_main_quit();
  }

  static void AddGadgetHandler(GtkMenuItem *item, gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    impl->gadget_manager_->ShowGadgetBrowserDialog(&impl->gadget_browser_host_);
  }

  static void ToggleAllGadgetsHandler(GtkWidget *widget, gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    if (impl->gadgets_shown_)
      HideAllGadgetsHandler(widget, user_data);
    else
      ShowAllGadgetsHandler(widget, user_data);
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
      expanded_original_ = decorated;
      SingleViewHost *svh =
          new SingleViewHost(ViewHostInterface::VIEW_HOST_MAIN, zoom_,
                             false, false, false, view_debug_mode_);
      expanded_popout_ =
          new DecoratedViewHost(svh, DecoratedViewHost::MAIN_EXPANDED,
                                transparent_);
      expanded_popout_->ConnectOnClose(NewSlot(this, &Impl::OnCloseHandler,
                                               expanded_popout_));

      int gadget_id = child->GetGadget()->GetInstanceID();

      GadgetViewHostInfo *info = &view_hosts_[gadget_id];
      ASSERT(info->main_);
      ASSERT(!info->popout_);
      info->popout_ = svh;

      svh->ConnectOnShowHide(
          NewSlot(this, &Impl::OnPopOutViewShowHideHandler, gadget_id));
      svh->ConnectOnBeginResizeDrag(
          NewSlot(this, &Impl::OnPopOutViewBeginResizeHandler, gadget_id));
      svh->ConnectOnResized(
          NewSlot(this, &Impl::OnPopOutViewResizedHandler, gadget_id));
      svh->ConnectOnBeginMoveDrag(
          NewSlot(this, &Impl::OnPopOutViewBeginMoveHandler, gadget_id));

      // Send popout event to decorator first.
      SimpleEvent event(Event::EVENT_POPOUT);
      expanded_original_->GetDecoratedView()->OnOtherEvent(event);

      child->SwitchViewHost(expanded_popout_);
      expanded_popout_->ShowView(false, 0, NULL);
    }
  }

  void OnPopInHandler(DecoratedViewHost *decorated) {
    if (expanded_original_ == decorated && expanded_popout_) {
      ViewInterface *child = expanded_popout_->GetView();
      ASSERT(child);
      if (child) {
        expanded_popout_->CloseView();
        ViewHostInterface *old_host = child->SwitchViewHost(expanded_original_);
        SimpleEvent event(Event::EVENT_POPIN);
        expanded_original_->GetDecoratedView()->OnOtherEvent(event);
        // The old host must be destroyed after sending onpopin event.
        old_host->Destroy();
        expanded_original_ = NULL;
        expanded_popout_ = NULL;

        // Clear the popout info.
        int gadget_id = child->GetGadget()->GetInstanceID();
        view_hosts_[gadget_id].popout_ = NULL;
      }
    }
  }

  void AdjustViewHostPosition(GadgetViewHostInfo *info) {
    ASSERT(info && info->main_ && info->main_decorator_);
    int x, y;
    int width, height;
    info->main_->GetWindowPosition(&x, &y);
    info->main_->GetWindowSize(&width, &height);
    int screen_width = gdk_screen_get_width(
        gtk_widget_get_screen(info->main_->GetWindow()));
    int screen_height = gdk_screen_get_height(
        gtk_widget_get_screen(info->main_->GetWindow()));

    bool main_dock_right = (x > width);

    if (info->popout_ && info->popout_->IsVisible()) {
      int popout_width, popout_height;
      info->popout_->GetWindowSize(&popout_width, &popout_height);
      if (info->popout_on_right_ && popout_width < x &&
          x + width + popout_width > screen_width)
        info->popout_on_right_ = false;
      else if (!info->popout_on_right_ && popout_width > x &&
               x + width + popout_width < screen_width)
        info->popout_on_right_ = true;

      if (y + popout_height > screen_height)
        y = screen_height - popout_height;

      if (info->popout_on_right_) {
        info->popout_->SetWindowPosition(x + width, y);
        width += popout_width;
      } else {
        info->popout_->SetWindowPosition(x - popout_width, y);
        x -= popout_width;
        width += popout_width;
      }

      main_dock_right = !info->popout_on_right_;
    }

    if (info->details_ && info->details_->IsVisible()) {
      int details_width, details_height;
      info->details_->GetWindowSize(&details_width, &details_height);
      if (info->details_on_right_ && details_width < x &&
          x + width + details_width > screen_width)
        info->details_on_right_ = false;
      else if (!info->details_on_right_ && details_width > x &&
               x + width + details_width < screen_width)
        info->details_on_right_ = true;

      if (y + details_height > screen_height)
        y = screen_height - details_height;

      if (info->details_on_right_) {
        info->details_->SetWindowPosition(x + width, y);
      } else {
        info->details_->SetWindowPosition(x - details_width, y);
      }
    }

    info->main_decorator_->SetDockEdge(main_dock_right);
  }

  void OnMainViewShowHideHandler(bool show, int gadget_id) {
    GadgetViewHostsMap::iterator it = view_hosts_.find(gadget_id);
    if (it != view_hosts_.end()) {
      if (show) {
        if (it->second.popout_ && !it->second.popout_->IsVisible())
          it->second.popout_->ShowView(false, 0, NULL);
        AdjustViewHostPosition(&it->second);
      } else {
        if (it->second.popout_) {
          it->second.popout_->CloseView();
        }
        if (it->second.details_) {
          // The details view won't be shown again.
          it->second.details_->CloseView();
          it->second.details_ = NULL;
        }
      }
    }
  }

  void OnMainViewResizedHandler(int width, int height, int gadget_id) {
    GadgetViewHostsMap::iterator it = view_hosts_.find(gadget_id);
    if (it != view_hosts_.end()) {
      AdjustViewHostPosition(&it->second);
    }
  }

  void OnMainViewMovedHandler(int x, int y, int gadget_id) {
    GadgetViewHostsMap::iterator it = view_hosts_.find(gadget_id);
    if (it != view_hosts_.end()) {
      AdjustViewHostPosition(&it->second);
    }
  }

  void OnPopOutViewShowHideHandler(bool show, int gadget_id) {
    GadgetViewHostsMap::iterator it = view_hosts_.find(gadget_id);
    if (it != view_hosts_.end() && it->second.popout_) {
      if (it->second.details_) {
        // Close Details whenever the popout view shows or hides.
        it->second.details_->CloseView();
        it->second.details_ = NULL;
      }
      if (show)
        AdjustViewHostPosition(&it->second);
    }
  }

  bool OnPopOutViewBeginResizeHandler(int button, int hittest, int gadget_id) {
    GadgetViewHostsMap::iterator it = view_hosts_.find(gadget_id);
    if (it != view_hosts_.end() && it->second.popout_) {
      if (it->second.popout_on_right_)
        return hittest == ViewInterface::HT_LEFT ||
               hittest == ViewInterface::HT_TOPLEFT ||
               hittest == ViewInterface::HT_BOTTOMLEFT ||
               hittest == ViewInterface::HT_TOP ||
               hittest == ViewInterface::HT_TOPRIGHT;
      else
        return hittest == ViewInterface::HT_RIGHT ||
               hittest == ViewInterface::HT_TOPRIGHT ||
               hittest == ViewInterface::HT_BOTTOMRIGHT ||
               hittest == ViewInterface::HT_TOP ||
               hittest == ViewInterface::HT_TOPLEFT;
    }
    return false;
  }

  void OnPopOutViewResizedHandler(int width, int height, int gadget_id) {
    GadgetViewHostsMap::iterator it = view_hosts_.find(gadget_id);
    if (it != view_hosts_.end() && it->second.popout_) {
      AdjustViewHostPosition(&it->second);
    }
  }

  bool OnPopOutViewBeginMoveHandler(int button, int gadget_id) {
    // User can't move popout view window.
    return true;
  }

  void OnDetailsViewShowHideHandler(bool show, int gadget_id) {
    GadgetViewHostsMap::iterator it = view_hosts_.find(gadget_id);
    if (it != view_hosts_.end() && it->second.details_) {
      if (show) {
        AdjustViewHostPosition(&it->second);
      } else {
        // The same details view will never shown again.
        it->second.details_ = NULL;
      }
    }
  }

  bool OnDetailsViewBeginResizeHandler(int button, int hittest, int gadget_id) {
    GadgetViewHostsMap::iterator it = view_hosts_.find(gadget_id);
    if (it != view_hosts_.end() && it->second.details_) {
      if (it->second.details_on_right_)
        return hittest == ViewInterface::HT_LEFT ||
               hittest == ViewInterface::HT_TOPLEFT ||
               hittest == ViewInterface::HT_BOTTOMLEFT ||
               hittest == ViewInterface::HT_TOP ||
               hittest == ViewInterface::HT_TOPRIGHT;
      else
        return hittest == ViewInterface::HT_RIGHT ||
               hittest == ViewInterface::HT_TOPRIGHT ||
               hittest == ViewInterface::HT_BOTTOMRIGHT ||
               hittest == ViewInterface::HT_TOP ||
               hittest == ViewInterface::HT_TOPLEFT;
    }
    return false;
  }

  void OnDetailsViewResizedHandler(int width, int height, int gadget_id) {
    GadgetViewHostsMap::iterator it = view_hosts_.find(gadget_id);
    if (it != view_hosts_.end() && it->second.details_) {
      AdjustViewHostPosition(&it->second);
    }
  }

  bool OnDetailsViewBeginMoveHandler(int button, int gadget_id) {
    // User can't move popout view window.
    return true;
  }

#if GTK_CHECK_VERSION(2,10,0) && defined(GGL_HOST_LINUX)
  static void StatusIconPopupMenuHandler(GtkWidget *widget, guint button,
                                         guint activate_time,
                                         gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    gtk_menu_popup(GTK_MENU(impl->host_menu_), NULL, NULL,
                   gtk_status_icon_position_menu, impl->status_icon_,
                   button, activate_time);
  }
#endif

  static gboolean DeleteEventHandler(GtkWidget *widget,
                                     GdkEvent *event,
                                     gpointer data) {
    gtk_main_quit();
    return TRUE;
  }

  typedef std::map<int, GadgetViewHostInfo> GadgetViewHostsMap;
  typedef std::map<int, Gadget *> GadgetsMap;

  GadgetBrowserHost gadget_browser_host_;

  GadgetViewHostsMap view_hosts_;
  GadgetsMap gadgets_;

  SimpleGtkHost *owner_;

  double zoom_;
  bool decorated_;
  int view_debug_mode_;
  bool gadgets_shown_;
  bool transparent_;

  GadgetManagerInterface *gadget_manager_;
#if GTK_CHECK_VERSION(2,10,0) && defined(GGL_HOST_LINUX)
  GtkStatusIcon *status_icon_;
#else
  GtkWidget *main_widget_;
#endif
  GtkWidget *host_menu_;

  DecoratedViewHost *expanded_original_;
  DecoratedViewHost *expanded_popout_;
};

SimpleGtkHost::SimpleGtkHost(double zoom, bool decorated, int view_debug_mode)
  : impl_(new Impl(this, zoom, decorated, view_debug_mode)) {
  impl_->SetupUI();
  impl_->InitGadgets();
}

SimpleGtkHost::~SimpleGtkHost() {
  delete impl_;
  impl_ = NULL;
}

ViewHostInterface *SimpleGtkHost::NewViewHost(Gadget *gadget,
                                              ViewHostInterface::Type type) {
  return impl_->NewViewHost(gadget, type);
}

void SimpleGtkHost::RemoveGadget(Gadget *gadget, bool save_data) {
  return impl_->RemoveGadget(gadget, save_data);
}

void SimpleGtkHost::DebugOutput(DebugLevel level, const char *message) const {
  impl_->DebugOutput(level, message);
}

bool SimpleGtkHost::OpenURL(const char *url) const {
  return ggadget::gtk::OpenURL(url);
}

bool SimpleGtkHost::LoadFont(const char *filename) {
  return ggadget::gtk::LoadFont(filename);
}

void SimpleGtkHost::ShowGadgetAboutDialog(ggadget::Gadget *gadget) {
  ggadget::gtk::ShowGadgetAboutDialog(gadget);
}

void SimpleGtkHost::Run() {
  impl_->LoadGadgets();
  gtk_main();
}

} // namespace gtk
} // namespace hosts
