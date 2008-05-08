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

#include <string>
#include <map>
#include <QtGui/QIcon>
#include <QtGui/QMessageBox>
#include <QtGui/QFontDatabase>
#include <ggadget/common.h>
#include <ggadget/messages.h>
#include <ggadget/logger.h>
#include <ggadget/file_manager_factory.h>
#include <ggadget/qt/qt_view_host.h>
#include <ggadget/qt/utilities.h>
#include <ggadget/gadget_manager_interface.h>
#include <ggadget/script_runtime_manager.h>
#include <ggadget/decorated_view_host.h>
#include <ggadget/ggadget.h>
#include <ggadget/view.h>
#include <ggadget/gadget_consts.h>
#include "qt_host.h"
#include "qt_host_internal.h"

using namespace ggadget;
using namespace ggadget::qt;

namespace hosts {
namespace qt {

class QtHost::Impl {
 public:
  Impl(QtHost *host, int view_debug_mode)
    : gadget_manager_(GetGadgetManager()),
      host_(host),
      view_debug_mode_(view_debug_mode),
      gadgets_shown_(true),
      expanded_popout_(NULL),
      expanded_original_(NULL),
      obj_(new QtHostObject(host)) {
    ScriptRuntimeManager::get()->ConnectErrorReporter(
      NewSlot(this, &Impl::ReportScriptError));
    SetupUI();
    InitGadgets();
  }

  ~Impl() {
    delete obj_;
  }

  void SetupUI() {
    menu_.addAction("Add gadget", obj_, SLOT(OnAddGadget()));
    menu_.addAction("Exit", qApp, SLOT(quit()));
    tray_.setContextMenu(&menu_);
    std::string icon_data;
    if (GetGlobalFileManager()->ReadFile(kGadgetsIcon, &icon_data)) {
      QPixmap pixmap;
      pixmap.loadFromData(reinterpret_cast<const uchar *>(icon_data.c_str()),
                          icon_data.length());
      tray_.setIcon(pixmap);
    }
    tray_.show();
  }
  void InitGadgets() {
    gadget_manager_->EnumerateGadgetInstances(
        NewSlot(this, &Impl::AddGadgetInstanceCallback));
    gadget_manager_->ConnectOnNewGadgetInstance(
        NewSlot(this, &Impl::NewGadgetInstanceCallback));
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
        .append(description));
    int ret = QMessageBox::question(NULL,
                                    GM_("GADGET_CONFIRM_TITLE"),
                                    message.c_str(),
                                    QMessageBox::Yes| QMessageBox::No,
                                    QMessageBox::Yes);
    return ret == QMessageBox::Yes;
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
      LOG("Load gadget %s, with option %s, %s",
          path.c_str(), options.c_str(), result ? "succeeded" : "failed");
    }
    return true;
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

  bool LoadGadget(const char *path, const char *options_name,
                  int instance_id) {
    Gadget *gadget =
        new Gadget(host_, path, options_name, instance_id,
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
    return true;
  }

  ViewHostInterface *NewViewHost(ViewHostInterface::Type type) {
    //  bool decorated = type == ViewHostInterface::VIEW_HOST_OPTIONS;

    QtViewHost *qvh = new QtViewHost(
        type, 1.0, true,
        static_cast<ViewInterface::DebugMode>(view_debug_mode_));

    if (type == ViewHostInterface::VIEW_HOST_OPTIONS)
      return qvh;

    DecoratedViewHost *dvh;
    if (type == ViewHostInterface::VIEW_HOST_MAIN)
      dvh = new DecoratedViewHost(qvh, DecoratedViewHost::MAIN_STANDALONE,
                                  true);
    else
      dvh = new DecoratedViewHost(qvh, DecoratedViewHost::DETAILS,
                                  true);

    dvh->ConnectOnClose(NewSlot(this, &Impl::OnCloseHandler, dvh));
    dvh->ConnectOnPopOut(NewSlot(this, &Impl::OnPopOutHandler, dvh));
    dvh->ConnectOnPopIn(NewSlot(this, &Impl::OnPopInHandler, dvh));

    return dvh;
  }

  void RemoveGadget(Gadget *gadget, bool save_data) {
    ViewInterface *main_view = gadget->GetMainView();

    // If this gadget is popped out, popin it first.
    if (main_view->GetViewHost() == expanded_popout_) {
      OnPopInHandler(expanded_original_);
    }
    int instance_id = gadget->GetInstanceID();
    delete gadget;
    gadget_manager_->RemoveGadgetInstance(instance_id);
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
      QtViewHost *qvh = new QtViewHost(
          ViewHostInterface::VIEW_HOST_MAIN, 1.0, false,
          static_cast<ViewInterface::DebugMode>(view_debug_mode_));
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

  GadgetManagerInterface *gadget_manager_;
  QtHost *host_;
  int view_debug_mode_;
  bool gadgets_shown_;

  DecoratedViewHost *expanded_popout_;
  DecoratedViewHost *expanded_original_;

  QMenu menu_;
  QSystemTrayIcon tray_;
  QtHostObject *obj_;        // provides slots for qt
};

QtHost::QtHost(int view_debug_mode)
  : impl_(new Impl(this, view_debug_mode)) {
}

QtHost::~QtHost() {
  delete impl_;
}

ViewHostInterface *QtHost::NewViewHost(ViewHostInterface::Type type) {
  return impl_->NewViewHost(type);
}

void QtHost::RemoveGadget(Gadget *gadget, bool save_data) {
  impl_->RemoveGadget(gadget, save_data);
}

void QtHost::DebugOutput(DebugLevel level, const char *message) const {
  impl_->DebugOutput(level, message);
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

#include "qt_host_internal.moc"

} // namespace qt
} // namespace hosts
