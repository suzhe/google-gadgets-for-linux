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
#include <ggadget/messages.h>
#include <ggadget/logger.h>
#include <ggadget/script_runtime_manager.h>
#include <ggadget/sidebar.h>
#include <ggadget/view.h>

using namespace ggadget;
using namespace ggadget::gtk;

namespace hosts {
namespace gtk {

class SidebarGtkHost::Impl {
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

  class GadgetMoveClosure {
   public:
    GadgetMoveClosure(SidebarGtkHost::Impl *owner,
                      SingleViewHost *view_host,
                      View *view,
                      int height)
        : owner_(owner),
          view_host_(view_host),
          view_(view),
          sidebar_(NULL),
          height_(height) {
      sidebar_ = gtk_widget_get_toplevel(GTK_WIDGET(down_cast<SingleViewHost *>(
            owner->side_bar_->GetViewHost())->GetNativeWidget()));
      AddConnection(view_host->ConnectOnMoveDrag(
            NewSlot(this, &GadgetMoveClosure::HandleMove)));
      AddConnection(view_host->ConnectOnEndMoveDrag(
            NewSlot(this, &GadgetMoveClosure::HandleMoveEnd)));
      AddConnection(view_host->ConnectOnDock(
            NewSlot(this, &GadgetMoveClosure::HandleDock)));
      AddConnection(view_host->ConnectOnUnexpand(
            NewSlot(this, &GadgetMoveClosure::HandleUnexpand)));
    }
    ~GadgetMoveClosure() {
      for (size_t i = 0; i < connections_.size(); ++i)
        connections_[i]->Disconnect();
    }
    void AddConnection(Connection *connection) {
      connections_.push_back(connection);
    }
    void HandleMove(int button) {
      int h;
      if (IsOverlapWithSideBar(&h))
        owner_->side_bar_->InsertNullElement(h, view_);
      else
        owner_->side_bar_->ClearNullElement();
    }
    void HandleMoveEnd(int button) {
      int h;
      owner_->side_bar_->ClearNullElement();
      if (IsOverlapWithSideBar(&h)) {
        view_->GetGadget()->SetDisplayTarget(Gadget::TARGET_SIDEBAR);
        view_host_->Dock();
      }
    }
    void HandleDock() {
      owner_->Dock(view_->GetGadget(), height_, true);
    }
    void HandleUnexpand() {
      owner_->Unexpand(NULL);
    }
   private:
    bool IsOverlapWithSideBar(int *height) {
      int x, y, w, h;
      GtkWidget *floating = gtk_widget_get_toplevel(GTK_WIDGET(
          view_host_->GetNativeWidget()));
      gtk_window_get_position(GTK_WINDOW(floating), &x, &y);
      gtk_window_get_size(GTK_WINDOW(floating), &w, &h);
      int sx, sy, sw, sh;
      gtk_window_get_position(GTK_WINDOW(sidebar_), &sx, &sy);
      gtk_window_get_size(GTK_WINDOW(sidebar_), &sw, &sh);
      if ((x < sx && x + w > sx) || (x > sx && sx + sw > x)) {
        if (height) {
          int px, py;
          gtk_widget_get_pointer(floating, &px, &py);
          *height = y + py - sy;
        }
        return true;
      }
      return false;
    }
    SidebarGtkHost::Impl *owner_;
    SingleViewHost *view_host_;
    View *view_;
    GtkWidget *sidebar_;
    int height_;
    std::vector<Connection *> connections_;
  };

  Impl(SidebarGtkHost *owner, bool decorated, int view_debug_mode)
    : gadget_browser_host_(owner),
      owner_(owner),
      decorated_(decorated),
      view_debug_mode_(static_cast<ViewInterface::DebugMode>(view_debug_mode)),
      view_host_(NULL),
      expand_view_host_(NULL),
      side_bar_(NULL),
      gadget_manager_(GetGadgetManager()) {
    ASSERT(gadget_manager_);
    ScriptRuntimeManager::get()->ConnectErrorReporter(
        NewSlot(this, &Impl::ReportScriptError));
    view_host_ = new SingleViewHost(ViewHostInterface::VIEW_HOST_MAIN, 1.0,
                                    decorated, false, true, view_debug_mode_);
    side_bar_ = new SideBar(owner_, view_host_);
    side_bar_->SetAddGadgetSlot(NewSlot(this, &Impl::AddGadgetHandler));
    side_bar_->SetCloseSlot(NewSlot(this, &Impl::ExitHandler));

    connections_.push_back(view_host_->ConnectOnUndock(
          NewSlot(this, &Impl::HandleUndock)));
    connections_.push_back(view_host_->ConnectOnExpand(
          NewSlot(this, &Impl::HandleExpand)));
    connections_.push_back(view_host_->ConnectOnUnexpand(
          NewSlot(this, &Impl::HandleUnexpand)));
  }

  ~Impl() {
    for (size_t i = 0; i < connections_.size(); ++i)
      connections_[i]->Disconnect();

    for (GadgetsMap::iterator it = gadgets_.begin();
         it != gadgets_.end(); ++it)
      delete it->second;

    gtk_widget_destroy(main_widget_);
    delete side_bar_;
  }

  void SetupUI() {
    // add the menu into the main widget
    main_widget_ = gtk_widget_get_toplevel(GTK_WIDGET(
        view_host_->GetNativeWidget()));
#ifdef _DEBUG
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(main_widget_), FALSE);
#endif
    gtk_window_set_title(GTK_WINDOW(main_widget_), "Google Gadgets");
    gtk_widget_show(main_widget_);
    g_assert(GTK_WIDGET_REALIZED(main_widget_));
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
      LOG("SidebarGtkHost: Load gadget %s, with option %s, %s",
          path.c_str(), options.c_str(), result ? "succeeded" : "failed");
    }
    return true;
  }

  bool Dock(Gadget *gadget, int height, bool force_insert) {
    ASSERT(gadget);
    gadget->SetDisplayTarget(Gadget::TARGET_SIDEBAR);
    ViewInterface *view = gadget->GetMainView();
    DLOG("Dock in SidebarGtkHost, view: %p, decorate view: %p",
        gadget->GetMainView(), view);
    return side_bar_->Dock(height, down_cast<View *>(view), force_insert);
  }

  bool Undock(Gadget *gadget) {
    ASSERT(gadget);
    side_bar_->Undock(gadget->GetMainView());
    gadget->SetDisplayTarget(Gadget::TARGET_FLOATING_VIEW);
    ViewHostInterface *new_host = NewSingleViewHost(gadget->GetMainView(), true);
    ViewHostInterface *old = gadget->GetMainView()->SwitchViewHost(new_host);
    if (old) old->Destroy();
    return gadget->ShowMainView();
  }

  void HandleUndock() {
    Gadget *gadget = side_bar_->GetMouseOverGadget();
    if (gadget)
      Undock(gadget);
  }

  void Expand(Gadget *gadget) {
    if (!expand_view_host_) {
      expand_view_host_ = NewSingleViewHost(gadget->GetMainView(), false);
    }
    expand_view_host_->SetView(gadget->GetMainView());
    expand_view_host_->ShowView(true, 0, NULL);
    expand_view_host_->Expand();
    side_bar_->Expand(gadget->GetMainView());
  }

  void Unexpand(Gadget *gadget) {
    expand_view_host_->Unexpand();
    expand_view_host_->SetView(NULL);
    expand_view_host_->CloseView();
    side_bar_->Unexpand(NULL);
  }

  void HandleExpand() {
    Gadget *gadget = side_bar_->GetMouseOverGadget();
    if (gadget)
      Expand(gadget);
  }

  void HandleUnexpand() {
    Unexpand(NULL);
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

    DLOG("Gadget %p with view %p", gadget, gadget->GetMainView());

    if (!gadget->IsValid()) {
      LOG("Failed to load gadget %s", path);
      delete gadget;
      return false;
    }

    if (!Dock(gadget, 0, false)) {
      DLOG("Dock view(%p) failed.", gadget->GetMainView());
      Undock(gadget);
    }

    if (!gadget->ShowMainView()) {
      LOG("Failed to show main view of gadget %s", path);
      delete gadget;
      return false;
    }

    gadgets_[instance_id] = gadget;
    return true;
  }

  ViewHostInterface *NewSingleViewHost(View *view, bool remove_on_close) {
    SingleViewHost *view_host =
      new SingleViewHost(ViewHostInterface::VIEW_HOST_MAIN, 1.0,
          decorated_, remove_on_close, false, view_debug_mode_);
    DLOG("New decorator for vh %p", view_host);
    DecoratedViewHost *decorator = new DecoratedViewHost(view_host, true);
    GadgetMoveClosure *closure = new GadgetMoveClosure(this, view_host, view, 0);
    move_slots_[view->GetGadget()] = closure;
    DLOG("New decorator %p with vh %p", decorator, view_host);
    return decorator;
  }

  ViewHostInterface *NewViewHost(ViewHostInterface::Type type) {
    ViewHostInterface *inner;
    DecoratedViewHost *decorator;
    switch (type) {
      case ViewHostInterface::VIEW_HOST_MAIN:
        inner = side_bar_->NewViewHost(type);
        decorator = new DecoratedViewHost(inner, false);
        break;
      case ViewHostInterface::VIEW_HOST_OPTIONS:
        inner = new SingleViewHost(type, 1.0, true, true, true,
                                   view_debug_mode_);
        decorator = new DecoratedViewHost(inner, false);
        break;
      default:
        inner = new SingleViewHost(type, 1.0, decorated_, true, true,
                                   view_debug_mode_);
        decorator = new DecoratedViewHost(inner, true);
        break;
    }
    return decorator;
  }

  void RemoveGadget(Gadget *gadget, bool save_data) {
    int instance_id = gadget->GetInstanceID();
    GadgetsMap::iterator it = gadgets_.find(instance_id);

    if (it != gadgets_.end()) {
      side_bar_->Undock(gadget->GetMainView());
      delete gadget;
      gadgets_.erase(it);
    } else {
      LOG("Can't find gadget instance %d", instance_id);
    }

    gadget_manager_->RemoveGadgetInstance(instance_id);
  }

  void AddGadgetHandler() {
    DLOG("Add Gadget now");
    gadget_manager_->ShowGadgetBrowserDialog(&gadget_browser_host_);
  }

  void ExitHandler() {
    gtk_main_quit();
  }

  void DebugOutput(DebugLevel level, const char *message) const {
    const char *str_level = "";
    switch (level) {
      case DEBUG_TRACE: str_level = "TRACE: "; break;
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

  static gboolean DelayedLoadGadgets(gpointer user_data) {
    Impl *impl = reinterpret_cast<Impl *>(user_data);
    impl->gadget_manager_->EnumerateGadgetInstances(
        NewSlot(impl, &Impl::AddGadgetInstanceCallback));
    return FALSE;
  }

  GadgetBrowserHost gadget_browser_host_;

  typedef std::map<Gadget *, GadgetMoveClosure *> GadgetMoveClosureMap;
  GadgetMoveClosureMap move_slots_;

  typedef std::map<int, Gadget *> GadgetsMap;
  GadgetsMap gadgets_;
  SidebarGtkHost *owner_;

  bool decorated_;
  ViewInterface::DebugMode view_debug_mode_;

  SingleViewHost *view_host_;
  ViewHostInterface *expand_view_host_;
  SideBar *side_bar_;

  GadgetManagerInterface *gadget_manager_;
  GtkWidget *main_widget_;

  std::vector<Connection *> connections_;
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
  gtk_main();
}

} // namespace gtk
} // namespace hosts
