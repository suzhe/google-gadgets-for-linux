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
#include <ggadget/messages.h>
#include <ggadget/logger.h>
#include <ggadget/script_runtime_manager.h>
#include <ggadget/view.h>

using namespace ggadget;
using namespace ggadget::gtk;

namespace hosts {
namespace gtk {

class SimpleGtkHost::Impl {
 public:
  // A special Host for Gadget browser to show browser in a decorated window.
  class GadgetBrowserHost : public HostInterface {
   public:
    GadgetBrowserHost(HostInterface *owner) : owner_(owner) { }
    virtual ViewHostInterface *NewViewHost(ViewHostInterface::Type type) {
      return new SingleViewHost(type, 1.0, true, true, true,
                                ViewInterface::DEBUG_DISABLED);
    }
    virtual void RemoveGadget(Gadget *gadget, bool save_data) {
      GetGadgetManager()->RemoveGadgetInstance(gadget->GetInstanceID());
    }
    virtual void DebugOutput(DebugLevel level, const char *message) const {
      owner_->DebugOutput(level, message);
    }
    virtual bool OpenURL(const char *url) const {
      return owner_->OpenURL(url);
    }
    virtual bool LoadFont(const char *filename) {
      return owner_->LoadFont(filename);
    }
    virtual void ShowGadgetAboutDialog(Gadget *gadget) {
      owner_->ShowGadgetAboutDialog(gadget);
    }
    virtual void Run() {}
   private:
    HostInterface *owner_;
  };

  Impl(SimpleGtkHost *owner, double zoom, bool decorated, int view_debug_mode)
    : gadget_browser_host_(owner),
      owner_(owner),
      zoom_(zoom),
      decorated_(decorated),
      view_debug_mode_(view_debug_mode),
      gadgets_shown_(true),
      gadget_manager_(GetGadgetManager()) {
    ASSERT(gadget_manager_);
    ScriptRuntimeManager::get()->ConnectErrorReporter(
        NewSlot(this, &Impl::ReportScriptError));
  }

  ~Impl() {
    for (GadgetsMap::iterator it = gadgets_.begin();
         it != gadgets_.end(); ++it)
      delete it->second;

    gtk_widget_destroy(host_menu_);
//#if GTK_CHECK_VERSION(2,10,0)
#if 0
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

//#if GTK_CHECK_VERSION(2,10,0)
#if 0
    // FIXME:
    status_icon_ = gtk_status_icon_new_from_stock(GTK_STOCK_ABOUT);
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
    StringMap data;
    if (!Gadget::GetGadgetManifest(path.c_str(), &data))
      return false;

    GtkWidget *dialog = gtk_message_dialog_new(
        NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
        "%s\n\n%s\n%s\n\n%s%s",
        GM_("GADGET_CONFIRM_MESSAGE"), data[kManifestName].c_str(),
        gadget_manager_->GetGadgetInstanceDownloadURL(id).c_str(),
        GM_("GADGET_DESCRIPTION"), data[kManifestDescription].c_str());
    
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
    g_idle_add(DelayedLoadGadgets, this);
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

  ViewHostInterface *NewViewHost(ViewHostInterface::Type type) {
    ViewHostInterface *host = NULL;
    bool decorated = (decorated_ || type != ViewHostInterface::VIEW_HOST_MAIN);

    SingleViewHost *svh = new SingleViewHost(type, zoom_, decorated, false,
        true, static_cast<ViewInterface::DebugMode>(view_debug_mode_));

    DecoratedViewHost *dvh  = new DecoratedViewHost(svh, true);
    host = dvh;

    return host;
  }

  void RemoveGadget(Gadget *gadget, bool save_data) {
    int instance_id = gadget->GetInstanceID();
    GadgetsMap::iterator it = gadgets_.find(instance_id);

    if (it != gadgets_.end()) {
      delete gadget;
      gadgets_.erase(it);
    } else {
      LOG("Can't find gadget instance %d", instance_id);
    }

    gadget_manager_->RemoveGadgetInstance(instance_id);
  }

  void DebugOutput(DebugLevel level, const char *message) const {
    const char *str_level = "";
    switch (level) {
      case DEBUG_TRACE: str_level = "TRACE: "; break;
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

  static gboolean DelayedLoadGadgets(gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    impl->gadget_manager_->EnumerateGadgetInstances(
        NewSlot(impl, &Impl::AddGadgetInstanceCallback));
    return FALSE;
  }

  static void ShowAllGadgetsHandler(GtkWidget *widget, gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    for (GadgetsMap::iterator it = impl->gadgets_.begin();
         it != impl->gadgets_.end(); ++it) {
      it->second->ShowMainView();
    }
    impl->gadgets_shown_ = true;
  }

  static void HideAllGadgetsHandler(GtkWidget *widget, gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    for (GadgetsMap::iterator it = impl->gadgets_.begin();
         it != impl->gadgets_.end(); ++it) {
      it->second->CloseMainView();
    }
    impl->gadgets_shown_ = false;
  }

  static void ExitHandler(GtkWidget *widget, gpointer user_data) {
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

//#if GTK_CHECK_VERSION(2,10,0)
#if 0
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

  GadgetBrowserHost gadget_browser_host_;

  typedef std::map<int, Gadget *> GadgetsMap;
  GadgetsMap gadgets_;
  SimpleGtkHost *owner_;

  double zoom_;
  bool decorated_;
  int view_debug_mode_;
  bool gadgets_shown_;

  GadgetManagerInterface *gadget_manager_;
//#if GTK_CHECK_VERSION(2,10,0)
#if 0
  GtkStatusIcon *status_icon_;
#else
  GtkWidget *main_widget_;
#endif
  GtkWidget *host_menu_;
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

ViewHostInterface *SimpleGtkHost::NewViewHost(ViewHostInterface::Type type) {
  return impl_->NewViewHost(type);
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
  gtk_main();
}

} // namespace gtk
} // namespace hosts
