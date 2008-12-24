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
#ifndef HOSTS_QT_HOST_INTERNAL_H__
#define HOSTS_QT_HOST_INTERNAL_H__

#include <QtGui/QCursor>
#include <ggadget/host_utils.h>

using namespace ggadget;
using namespace ggadget::qt;

namespace hosts {
namespace qt {

class QtHost::Impl : public QObject {
  Q_OBJECT
 public:
  struct GadgetInfo {
    GadgetInfo() : gadget(NULL), debug_console(NULL) {}
    ~GadgetInfo() {
      delete debug_console;
      delete gadget;
    }
    Gadget *gadget;
    QWidget* debug_console;
  };
  Impl(QtHost *host, bool composite,
       int view_debug_mode,
       Gadget::DebugConsoleConfig debug_console_config)
    : gadget_manager_(GetGadgetManager()),
      gadget_browser_host_(host, view_debug_mode),
      host_(host),
      view_debug_mode_(view_debug_mode),
      debug_console_config_(debug_console_config),
      composite_(composite),
      gadgets_shown_(true),
      expanded_popout_(NULL),
      expanded_original_(NULL),
      show_(true) {
    // Initializes global permissions.
    // FIXME: Supports customizable global permissions.
    global_permissions_.SetGranted(Permissions::ALL_ACCESS, true);
    SetupUI();
  }

  ~Impl() {
    DLOG("Going to free %zd gadgets", gadgets_.size());
    for (GadgetsMap::iterator it = gadgets_.begin();
         it != gadgets_.end(); ++it) {
      DLOG("Close Gadget: %s",
           it->second.gadget->GetManifestInfo(kManifestName).c_str());
      it->second.gadget->CloseMainView();  // TODO: Save window state. A little hacky!
    }
  }

  void SetupUI() {
    qApp->setQuitOnLastWindowClosed(false);
    menu_.addAction(QString::fromUtf8(GM_("MENU_ITEM_ADD_GADGETS")),
                    this, SLOT(OnAddGadget()));
    menu_.addAction(QString::fromUtf8(GM_("MENU_ITEM_SHOW_ALL")),
                    this, SLOT(OnShowAll()));
    menu_.addAction(QString::fromUtf8(GM_("MENU_ITEM_HIDE_ALL")),
                    this, SLOT(OnHideAll()));
    menu_.addSeparator();
    menu_.addAction(QString::fromUtf8(GM_("MENU_ITEM_ABOUT")), this, SLOT(OnAbout()));
    menu_.addAction(QString::fromUtf8(GM_("MENU_ITEM_EXIT")),
                    qApp, SLOT(quit()));
    tray_.setContextMenu(&menu_);
    QObject::connect(&tray_,
                     SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                     this,
                     SLOT(OnTrayActivated(QSystemTrayIcon::ActivationReason)));
    std::string icon_data;
    if (GetGlobalFileManager()->ReadFile(kGadgetsIcon, &icon_data)) {
      QPixmap pixmap;
      pixmap.loadFromData(reinterpret_cast<const uchar *>(icon_data.c_str()),
                          static_cast<int>(icon_data.length()));
      tray_.setIcon(pixmap);
    }
    tray_.show();
  }

  void InitGadgets() {
    gadget_manager_->ConnectOnNewGadgetInstance(
        NewSlot(this, &Impl::NewGadgetInstanceCallback));
    gadget_manager_->EnumerateGadgetInstances(
        NewSlot(this, &Impl::EnumerateGadgetInstancesCallback));
    gadget_manager_->ConnectOnRemoveGadgetInstance(
        NewSlot(this, &Impl::RemoveGadgetInstanceCallback));
  }

  bool EnumerateGadgetInstancesCallback(int id) {
    if (!LoadGadgetInstance(id))
      gadget_manager_->RemoveGadgetInstance(id);
    // Return true to continue the enumeration.
    return true;
  }

  bool NewGadgetInstanceCallback(int id) {
    return LoadGadgetInstance(id);
  }

  bool LoadGadgetInstance(int id) {
    bool result = false;
    if (ggadget::qt::ConfirmGadget(gadget_manager_, id)) {
      std::string options = gadget_manager_->GetGadgetInstanceOptionsName(id);
      std::string path = gadget_manager_->GetGadgetInstancePath(id);
      if (options.length() && path.length()) {
        result = (LoadGadget(path.c_str(), options.c_str(), id, false) != NULL);
        DLOG("QtHost: Load gadget %s, with option %s, %s",
             path.c_str(), options.c_str(), result ? "succeeded" : "failed");
      }
    } else {
      QMessageBox::information(
          NULL,
          QString::fromUtf8(GM_("GOOGLE_GADGETS")),
          QString::fromUtf8(
              StringPrintf(
                  GM_("GADGET_LOAD_FAILURE"),
                  gadget_manager_->GetGadgetInstancePath(id).c_str()).c_str()));
    }
    return result;
  }

  Gadget *LoadGadget(const char *path, const char *options_name,
                     int instance_id, bool show_debug_console) {
    if (gadgets_.find(instance_id) != gadgets_.end()) {
      // Gadget is already loaded.
      return gadgets_[instance_id].gadget;
    }

    // Create GadgetInfo here so when during Gadget creation, if DebugConsole
    // shall be created, this information will be ready.
    GadgetInfo *info = &gadgets_[instance_id];

    Gadget::DebugConsoleConfig dcc = show_debug_console ?
        Gadget::DEBUG_CONSOLE_INITIAL : debug_console_config_;

    Gadget *gadget = new Gadget(host_, path, options_name, instance_id,
                                global_permissions_, dcc);
    info->gadget = gadget;

    if (!gadget->IsValid()) {
      LOG("Failed to load gadget %s", path);
      gadgets_.erase(instance_id);
      return NULL;
    }

    SetupGadgetOpenFeedbackURLHandler(gadget);

    gadget->SetDisplayTarget(Gadget::TARGET_FLOATING_VIEW);
    gadget->GetMainView()->OnOtherEvent(SimpleEvent(Event::EVENT_UNDOCK));

    if (!gadget->ShowMainView()) {
      LOG("Failed to show main view of gadget %s", path);
      gadgets_.erase(instance_id);
      return NULL;
    }
    return gadget;
  }

  ViewHostInterface *NewViewHost(Gadget *gadget,
                                 ViewHostInterface::Type type) {
    QWidget *parent = NULL;
    QtViewHost::Flags flags;

    if (composite_)
      flags |= QtViewHost::FLAG_COMPOSITE;

    if (type == ViewHostInterface::VIEW_HOST_DETAILS)
      parent = static_cast<QWidget*>(gadget->GetMainView()->GetNativeWidget());
    else if (type == ViewHostInterface::VIEW_HOST_MAIN)
      flags |= QtViewHost::FLAG_RECORD_STATES;

    QtViewHost *qvh = new QtViewHost(
        type, 1.0, flags, view_debug_mode_, parent);
    QObject::connect(this, SIGNAL(show(bool)),
                     qvh->GetQObject(), SLOT(OnShow(bool)));

    if (type == ViewHostInterface::VIEW_HOST_OPTIONS)
      return qvh;

    DecoratedViewHost *dvh;

    if (type == ViewHostInterface::VIEW_HOST_MAIN) {
      FloatingMainViewDecorator *view_decorator =
          new FloatingMainViewDecorator(qvh, composite_);
      dvh = new DecoratedViewHost(view_decorator);
      view_decorator->ConnectOnClose(
          NewSlot(this, &Impl::OnCloseMainViewHandler, dvh));
      view_decorator->ConnectOnPopOut(
          NewSlot(this, &Impl::OnPopOutHandler, dvh));
      view_decorator->ConnectOnPopIn(
          NewSlot(this, &Impl::OnPopInHandler, dvh));
      view_decorator->SetButtonVisible(MainViewDecoratorBase::POP_IN_OUT_BUTTON,
                                       false);
    } else {
      DetailsViewDecorator *view_decorator = new DetailsViewDecorator(qvh);
      dvh = new DecoratedViewHost(view_decorator);
      view_decorator->ConnectOnClose(
          NewSlot(this, &Impl::OnCloseDetailsViewHandler, dvh));
    }

    return dvh;
  }

  void RemoveGadget(Gadget *gadget, bool save_data) {
    ViewInterface *main_view = gadget->GetMainView();

    // If this gadget is popped out, popin it first.
    if (main_view->GetViewHost() == expanded_popout_) {
      OnPopInHandler(expanded_original_);
    }

    int id = gadget->GetInstanceID();
    // If RemoveGadgetInstance() returns false, then means this instance is not
    // installed by gadget manager.
    if (!gadget_manager_->RemoveGadgetInstance(id))
      RemoveGadgetInstanceCallback(id);
  }

  void RemoveGadgetInstanceCallback(int instance_id) {
    GadgetsMap::iterator it = gadgets_.find(instance_id);
    if (it != gadgets_.end()) {
      DLOG("Close Gadget: %s",
           it->second.gadget->GetManifestInfo(kManifestName).c_str());
      gadgets_.erase(it);
    } else {
      LOG("Can't find gadget instance %d", instance_id);
    }
  }

  void OnCloseMainViewHandler(DecoratedViewHost *decorated) {
    // Closing a main view which has popout view causes the popout view close
    // first
    if (expanded_original_ == decorated && expanded_popout_)
      OnPopInHandler(decorated);

    ViewInterface *child = decorated->GetView();
    Gadget *gadget = child ? child->GetGadget() : NULL;

    if (gadget) {
      gadget->CloseMainView();  // TODO: Save window state. A little hacky!
      gadget->RemoveMe(true);
    }
  }

  void OnClosePopOutViewHandler(DecoratedViewHost *decorated) {
    if (expanded_original_ && expanded_popout_ == decorated)
      OnPopInHandler(expanded_original_);
  }

  void OnCloseDetailsViewHandler(DecoratedViewHost *decorated) {
    ViewInterface *child = decorated->GetView();
    Gadget *gadget = child ? child->GetGadget() : NULL;
    if (gadget)
      gadget->CloseDetailsView();
  }

  void OnPopOutHandler(DecoratedViewHost *decorated) {
    if (expanded_original_) {
      bool just_hide = (decorated == expanded_original_);
      OnPopInHandler(expanded_original_);
      if (just_hide) return;
    }

    ViewInterface *child = decorated->GetView();
    ASSERT(child);
    if (child) {
      expanded_original_ = decorated;
      QtViewHost *qvh = new QtViewHost(
          ViewHostInterface::VIEW_HOST_MAIN, 1.0,
          composite_ ?
          QtViewHost::FLAG_COMPOSITE :
          QtViewHost::FLAG_NONE,
          view_debug_mode_,
          static_cast<QWidget*>(decorated->GetNativeWidget()));
      // qvh->ConnectOnBeginMoveDrag(NewSlot(this, &Impl::HandlePopoutViewMove));
      PopOutMainViewDecorator *view_decorator =
          new PopOutMainViewDecorator(qvh);
      expanded_popout_ = new DecoratedViewHost(view_decorator);
      view_decorator->ConnectOnClose(
          NewSlot(this, &Impl::OnClosePopOutViewHandler, expanded_popout_));

      // Send popout event to decorator first.
      SimpleEvent event(Event::EVENT_POPOUT);
      expanded_original_->GetViewDecorator()->OnOtherEvent(event);

      child->SwitchViewHost(expanded_popout_);
      expanded_popout_->ShowView(false, 0, NULL);
    }
  }

  void OnPopInHandler(DecoratedViewHost *decorated) {
    if (expanded_original_ == decorated && expanded_popout_) {
      ViewInterface *child = expanded_popout_->GetView();
      ASSERT(child);
      if (child) {
        // Close details view
        child->GetGadget()->CloseDetailsView();

        ViewHostInterface *old_host = child->SwitchViewHost(expanded_original_);
        SimpleEvent event(Event::EVENT_POPIN);
        expanded_original_->GetViewDecorator()->OnOtherEvent(event);
        // The old host must be destroyed after sending onpopin event.
        old_host->Destroy();
        expanded_original_ = NULL;
        expanded_popout_ = NULL;
      }
    }
  }

  void ShowGadgetDebugConsole(Gadget *gadget) {
    if (!gadget) return;
    GadgetsMap::iterator it = gadgets_.find(gadget->GetInstanceID());
    if (it == gadgets_.end()) return;
    GadgetInfo *info = &(it->second);
    if (info->debug_console) {
      DLOG("Gadget has already opened a debug console: %p",
           info->debug_console);
      return;
    }
    NewGadgetDebugConsole(gadget, &info->debug_console);
  }

  GadgetManagerInterface *gadget_manager_;
  GadgetBrowserHost gadget_browser_host_;
  QtHost *host_;
  int view_debug_mode_;
  Gadget::DebugConsoleConfig debug_console_config_;
  bool composite_;
  bool gadgets_shown_;

  DecoratedViewHost *expanded_popout_;
  DecoratedViewHost *expanded_original_;

  typedef std::map<int, GadgetInfo> GadgetsMap;
  GadgetsMap gadgets_;

  QMenu menu_;
  QSystemTrayIcon tray_;
  bool show_;

  Permissions global_permissions_;

  // QObject stuffs
 signals:
  void show(bool);

 public slots:
  void OnAddGadget() {
    GetGadgetManager()->ShowGadgetBrowserDialog(&gadget_browser_host_);
  }
  void OnShowAll() {
    emit show(true);
    show_ = true;
  }
  void OnHideAll() {
    emit show(false);
    show_ = false;
  }

  void OnTrayActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::DoubleClick) {
      if (show_)
        OnHideAll();
      else
        OnShowAll();
    }
  }
  void OnAbout() {
    ShowAboutDialog(host_);
  }
};

}
}

#endif // HOSTS_QT_HOST_INTERNAL_H__
