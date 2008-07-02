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

#include <string>
#include <map>
#include <QtGui/QIcon>
#include <QtGui/QSystemTrayIcon>
#include <QtGui/QMenu>
#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#include <QtGui/QFontDatabase>
#include <ggadget/common.h>
#include <ggadget/decorated_view_host.h>
#include <ggadget/file_manager_factory.h>
#include <ggadget/gadget.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/gadget_manager_interface.h>
#include <ggadget/locales.h>
#include <ggadget/logger.h>
#include <ggadget/messages.h>
#include <ggadget/qt/qt_view_host.h>
#include <ggadget/qt/utilities.h>
#include <ggadget/script_runtime_manager.h>
#include <ggadget/decorated_view_host.h>
#include <ggadget/gadget.h>
#include <ggadget/view.h>
#include "qt_host.h"
#include "gadget_browser_host.h"
#include "qt_host_internal.h"

using namespace ggadget;
using namespace ggadget::qt;

namespace hosts {
namespace qt {

class QtHost::Impl {
 public:
  struct GadgetInfo {
    GadgetInfo(Gadget *g) : gadget_(g), debug_console_(NULL) {}
    ~GadgetInfo() {
      if (debug_console_) delete debug_console_;
      if (gadget_) delete gadget_;
    }
    Gadget *gadget_;
    QWidget* debug_console_;
  };
  Impl(QtHost *host, bool composite, int view_debug_mode, int dc_option)
    : gadget_manager_(GetGadgetManager()),
      gadget_browser_host_(host, view_debug_mode),
      host_(host),
      view_debug_mode_(view_debug_mode),
      debug_console_option_(dc_option),
      composite_(composite),
      gadgets_shown_(true),
      expanded_popout_(NULL),
      expanded_original_(NULL),
      obj_(new QtHostObject(host, &gadget_browser_host_)) {
    SetupUI();
  }

  ~Impl() {
    DLOG("Going to free %zd gadgets", gadgets_.size());
    for (GadgetsMap::iterator it = gadgets_.begin();
         it != gadgets_.end(); ++it) {
      DLOG("Close Gadget: %s", it->second->gadget_->GetManifestInfo(kManifestName).c_str());
      it->second->gadget_->CloseMainView();  // TODO: Save window state. A little hacky!
      delete it->second;
    }
    delete obj_;
  }

  void SetupUI() {
    qApp->setQuitOnLastWindowClosed(false);
    menu_.addAction(QString::fromUtf8(GM_("MENU_ITEM_ADD_GADGETS")),
                    obj_, SLOT(OnAddGadget()));
    menu_.addAction(QString::fromUtf8(GM_("MENU_ITEM_SHOW_ALL")),
                    obj_, SLOT(OnShowAll()));
    menu_.addAction(QString::fromUtf8(GM_("MENU_ITEM_HIDE_ALL")),
                    obj_, SLOT(OnHideAll()));
    menu_.addSeparator();
    menu_.addAction(QString::fromUtf8(GM_("MENU_ITEM_EXIT")),
                    qApp, SLOT(quit()));
    tray_.setContextMenu(&menu_);
    QObject::connect(&tray_,
                     SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                     obj_,
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
    gadget_manager_->EnumerateGadgetInstances(
        NewSlot(this, &Impl::EnumerateGadgetInstancesCallback));
    gadget_manager_->ConnectOnNewGadgetInstance(
        NewSlot(this, &Impl::NewGadgetInstanceCallback));
    gadget_manager_->ConnectOnRemoveGadgetInstance(
        NewSlot(this, &Impl::RemoveGadgetInstanceCallback));
  }

  bool ConfirmGadget(int id) {
    std::string path = gadget_manager_->GetGadgetInstancePath(id);
    std::string download_url, title, description;
    if (!gadget_manager_->GetGadgetInstanceInfo(id,
                                                GetSystemLocaleName().c_str(),
                                                NULL, &download_url,
                                                &title, &description))
      return false;

    std::string message = GM_("GADGET_CONFIRM_MESSAGE");
    message.append("\n\n")
        .append(title).append("\n")
        .append(download_url).append("\n\n")
        .append(GM_("GADGET_DESCRIPTION"))
        .append(description);
    int ret = QMessageBox::question(
        NULL,
        QString::fromUtf8(GM_("GADGET_CONFIRM_TITLE")),
        QString::fromUtf8(message.c_str()),
        QMessageBox::Yes| QMessageBox::No,
        QMessageBox::Yes);
    return ret == QMessageBox::Yes;
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
        DLOG("Load gadget %s, with option %s, succeeded",
             path.c_str(), options.c_str());
      } else {
        LOG("Load gadget %s, with option %s, failed",
             path.c_str(), options.c_str());
      }
    }
    return result;
  }

  bool LoadGadget(const char *path, const char *options_name,
                  int instance_id) {
    if (gadgets_.find(instance_id) != gadgets_.end()) {
      // Gadget is already loaded.
      return true;
    }

    Gadget *gadget = new Gadget(host_, path, options_name, instance_id,
                                // We still don't trust any user added gadgets
                                // at gadget runtime level.
                                false);

    if (!gadget->IsValid()) {
      LOG("Failed to load gadget %s", path);
      delete gadget;
      return false;
    }
    GadgetInfo *info = new GadgetInfo(gadget);
    if (debug_console_option_ >= 2)
      NewGadgetDebugConsole(gadget, &info->debug_console_);

    if (!gadget->ShowMainView()) {
      LOG("Failed to show main view of gadget %s", path);
      delete info;
      return false;
    }
    gadgets_[instance_id] = info;
    return true;
  }

  ViewHostInterface *NewViewHost(Gadget *gadget,
                                 ViewHostInterface::Type type) {
    QtViewHost *qvh = new QtViewHost(
        type, 1.0, composite_, false, true,
        static_cast<ViewInterface::DebugMode>(view_debug_mode_));
    QObject::connect(obj_, SIGNAL(show(bool)),
                     qvh->GetQObject(), SLOT(OnShow(bool)));

    if (type == ViewHostInterface::VIEW_HOST_OPTIONS)
      return qvh;

    DecoratedViewHost *dvh;

    if (type == ViewHostInterface::VIEW_HOST_MAIN)
      dvh = new DecoratedViewHost(qvh, DecoratedViewHost::MAIN_STANDALONE,
                                  composite_);
    else
      dvh = new DecoratedViewHost(qvh, DecoratedViewHost::DETAILS,
                                  composite_);

    dvh->ConnectOnClose(NewSlot(this, &Impl::OnCloseHandler, dvh));
    dvh->ConnectOnPopOut(NewSlot(this, &Impl::OnPopOutHandler, dvh));
    // dvh->ConnectOnPopIn(NewSlot(this, &Impl::OnPopInHandler, dvh));

    return dvh;
  }

  void RemoveGadget(Gadget *gadget, bool save_data) {
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
      DLOG("Close Gadget: %s", it->second->gadget_->GetManifestInfo(kManifestName).c_str());
      delete it->second;
      gadgets_.erase(it);
    } else {
      LOG("Can't find gadget instance %d", instance_id);
    }
  }

  void OnCloseHandler(DecoratedViewHost *decorated) {
    ViewInterface *child = decorated->GetView();
    Gadget *gadget = child ? child->GetGadget() : NULL;

    ASSERT(gadget);
    if (!gadget) return;

    switch (decorated->GetDecoratorType()) {
      case DecoratedViewHost::MAIN_STANDALONE:
      case DecoratedViewHost::MAIN_DOCKED:
        gadget->CloseMainView();  // TODO: Save window state. A little hacky!
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
      QtViewHost *qvh = new QtViewHost(
          ViewHostInterface::VIEW_HOST_MAIN, 1.0, composite_, false, false,
          view_debug_mode_);
      // qvh->ConnectOnBeginMoveDrag(NewSlot(this, &Impl::HandlePopoutViewMove));
      expanded_popout_ =
          new DecoratedViewHost(qvh, DecoratedViewHost::MAIN_EXPANDED, true);
      expanded_popout_->ConnectOnClose(NewSlot(this, &Impl::OnCloseHandler,
                                               expanded_popout_));

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
        ViewHostInterface *old_host = child->SwitchViewHost(expanded_original_);
        SimpleEvent event(Event::EVENT_POPIN);
        expanded_original_->GetDecoratedView()->OnOtherEvent(event);
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
    if (it == gadgets_.end() || !it->second) return;
    GadgetInfo *info = it->second;
    if (info->debug_console_) {
      DLOG("Gadget has already debug console opened: %p", info->debug_console_);
      return;
    }
    NewGadgetDebugConsole(gadget, &info->debug_console_);
  }

  GadgetManagerInterface *gadget_manager_;
  GadgetBrowserHost gadget_browser_host_;
  QtHost *host_;
  int view_debug_mode_;
  int debug_console_option_;
  bool composite_;
  bool gadgets_shown_;

  DecoratedViewHost *expanded_popout_;
  DecoratedViewHost *expanded_original_;

  QMenu menu_;
  QSystemTrayIcon tray_;
  QtHostObject *obj_;        // provides slots for qt

  typedef std::map<int, GadgetInfo*> GadgetsMap;
  GadgetsMap gadgets_;
};

QtHost::QtHost(bool composite, int view_debug_mode, int debug_console)
  : impl_(new Impl(this, composite, view_debug_mode, debug_console)) {
  impl_->InitGadgets();
}

QtHost::~QtHost() {
  DLOG("Removing QtHost");
  delete impl_;
  DLOG("QtHost removed");
}

ViewHostInterface *QtHost::NewViewHost(Gadget *gadget,
                                       ViewHostInterface::Type type) {
  return impl_->NewViewHost(gadget, type);
}

void QtHost::RemoveGadget(Gadget *gadget, bool save_data) {
  impl_->RemoveGadget(gadget, save_data);
}

bool QtHost::OpenURL(const char *url) const {
  return ggadget::qt::OpenURL(url);
}

bool QtHost::LoadFont(const char *filename) {
 if (QFontDatabase::addApplicationFont(filename) != -1)
   return true;
 else
   return false;
}

void QtHost::Run() {
}

void QtHost::ShowGadgetAboutDialog(ggadget::Gadget *gadget) {
  ggadget::qt::ShowGadgetAboutDialog(gadget);
}

void QtHost::ShowGadgetDebugConsole(ggadget::Gadget *gadget) {
  impl_->ShowGadgetDebugConsole(gadget);
}

} // namespace qt
} // namespace hosts
#include "qt_host_internal.moc"
