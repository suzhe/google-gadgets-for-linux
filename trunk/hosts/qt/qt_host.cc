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
#include <ggadget/gadget_consts.h>
#include "qt_host.h"

using namespace ggadget;
using namespace ggadget::qt;

namespace hosts {
namespace qt {

QtHost::QtHost(int view_debug_mode)
  : gadget_manager_(GetGadgetManager()),
    view_debug_mode_(view_debug_mode),
    gadgets_shown_(true),
    obj_(new QtObject(this)) {
  ASSERT(gadget_manager_);
  ScriptRuntimeManager::get()->ConnectErrorReporter(
      NewSlot(this, &QtHost::ReportScriptError));

  SetupUI();
  InitGadgets();
}

QtHost::~QtHost() {
  delete obj_;
}

ViewHostInterface *QtHost::NewViewHost(ViewHostInterface::Type type) {
  ViewHostInterface *host = new QtViewHost(type, 1.0, true, view_debug_mode_);
//  DecoratedViewHost *dvh  = new DecoratedViewHost(host, true);
  return host;
}

void QtHost::RemoveGadget(Gadget *gadget, bool save_data) {
  int instance_id = gadget->GetInstanceID();
  delete gadget;
  gadget_manager_->RemoveGadgetInstance(instance_id);
}

void QtHost::DebugOutput(DebugLevel level, const char *message) const {
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

void QtHost::SetupUI() {
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

void QtHost::InitGadgets() {
  gadget_manager_->EnumerateGadgetInstances(
      NewSlot(this, &QtHost::AddGadgetInstanceCallback));
  gadget_manager_->ConnectOnNewGadgetInstance(
      NewSlot(this, &QtHost::NewGadgetInstanceCallback));
}

bool QtHost::ConfirmGadget(int id) {
  std::string path = gadget_manager_->GetGadgetInstancePath(id);
  StringMap data;
  if (!Gadget::GetGadgetManifest(path.c_str(), &data))
    return false;

  std::string message = GM_("GADGET_CONFIRM_MESSAGE");
  message.append("\n\n")
      .append(data[kManifestName] + "\n")
      .append(gadget_manager_->GetGadgetInstanceDownloadURL(id) + "\n\n")
      .append(GM_("GADGET_DESCRIPTION"))
      .append(data[kManifestDescription]);
  int ret = QMessageBox::question(NULL,
                                  GM_("GADGET_CONFIRM_TITLE"),
                                  message.c_str(),
                                  QMessageBox::Yes| QMessageBox::No,
                                  QMessageBox::Yes);
  return ret == QMessageBox::Yes;
}

bool QtHost::NewGadgetInstanceCallback(int id) {
  if (gadget_manager_->IsGadgetInstanceTrusted(id) ||
      ConfirmGadget(id)) {
    return AddGadgetInstanceCallback(id);
  }
  return false;
}

bool QtHost::AddGadgetInstanceCallback(int id) {
  std::string options = gadget_manager_->GetGadgetInstanceOptionsName(id);
  std::string path = gadget_manager_->GetGadgetInstancePath(id);
  if (options.length() && path.length()) {
    bool result = LoadGadget(path.c_str(), options.c_str(), id);
    LOG("Load gadget %s, with option %s, %s",
        path.c_str(), options.c_str(), result ? "succeeded" : "failed");
  }
  return true;
}

void QtHost::ReportScriptError(const char *message) {
  DebugOutput(DEBUG_ERROR,
              (std::string("Script error: " ) + message).c_str());
}

bool QtHost::LoadGadget(const char *path, const char *options_name,
                        int instance_id) {
  Gadget *gadget =
      new Gadget(this, path, options_name, instance_id,
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

#include "qt_host.moc"

} // namespace qt
} // namespace hosts
