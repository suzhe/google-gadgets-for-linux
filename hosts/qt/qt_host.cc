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

#include <limits.h>
#include <string>
#include <map>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QTextStream>
#include <QtGui/QIcon>
#include <QtGui/QSystemTrayIcon>
#include <QtGui/QMenu>
#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#include <QtGui/QFontDatabase>
#include <ggadget/common.h>
#include <ggadget/decorated_view_host.h>
#include <ggadget/floating_main_view_decorator.h>
#include <ggadget/docked_main_view_decorator.h>
#include <ggadget/popout_main_view_decorator.h>
#include <ggadget/details_view_decorator.h>
#include <ggadget/file_manager_factory.h>
#include <ggadget/gadget.h>
#include <ggadget/options_interface.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/gadget_manager_interface.h>
#include <ggadget/locales.h>
#include <ggadget/logger.h>
#include <ggadget/messages.h>
#include <ggadget/qt/qt_view_host.h>
#include <ggadget/qt/utilities.h>
#include <ggadget/script_runtime_manager.h>
#include <ggadget/gadget.h>
#include <ggadget/view.h>
#include <ggadget/permissions.h>
#include <ggadget/options_interface.h>
#include "qt_host.h"
#include "gadget_browser_host.h"
#include "qt_host_internal.h"

using namespace ggadget;
using namespace ggadget::qt;

namespace hosts {
namespace qt {
const static char *kPlasmaID = "kde_plasma";

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
  Impl(QtHost *host, bool composite,
       int view_debug_mode,
       Gadget::DebugConsoleConfig debug_console_config,
       bool with_plasma)
    : gadget_manager_(GetGadgetManager()),
      gadget_browser_host_(host, view_debug_mode),
      host_(host),
      view_debug_mode_(view_debug_mode),
      debug_console_config_(debug_console_config),
      composite_(composite),
      with_plasma_(with_plasma),
      gadgets_shown_(true),
      expanded_popout_(NULL),
      expanded_original_(NULL),
      obj_(new QtHostObject(host, &gadget_browser_host_)) {
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
           it->second->gadget_->GetManifestInfo(kManifestName).c_str());
      it->second->gadget_->CloseMainView();  // TODO: Save window state. A little hacky!
      delete it->second;
    }
    delete obj_;
  }

  void SetupUI() {
    qApp->setQuitOnLastWindowClosed(false);
    menu_.addAction(QString::fromUtf8(GM_("MENU_ITEM_ADD_GADGETS")),
                    obj_, SLOT(OnAddGadget()));
    if (!with_plasma_) {
      menu_.addAction(QString::fromUtf8(GM_("MENU_ITEM_SHOW_ALL")),
                      obj_, SLOT(OnShowAll()));
      menu_.addAction(QString::fromUtf8(GM_("MENU_ITEM_HIDE_ALL")),
                      obj_, SLOT(OnHideAll()));
    }
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
    gadget_manager_->ConnectOnNewGadgetInstance(
        NewSlot(this, &Impl::NewGadgetInstanceCallback));
    if (with_plasma_) return;
    gadget_manager_->EnumerateGadgetInstances(
        NewSlot(this, &Impl::EnumerateGadgetInstancesCallback));
    gadget_manager_->ConnectOnRemoveGadgetInstance(
        NewSlot(this, &Impl::RemoveGadgetInstanceCallback));
  }

  static bool GetPermissionsDescriptionCallback(int permission,
                                                std::string *msg) {
    if (msg->length())
      msg->append("\n");
    msg->append("  ");
    msg->append(Permissions::GetDescription(permission));
    return true;
  }

  bool ConfirmGadget(int id, Permissions *permissions) {
    std::string path = gadget_manager_->GetGadgetInstancePath(id);
    std::string download_url, title, description;
    if (!gadget_manager_->GetGadgetInstanceInfo(id,
                                                GetSystemLocaleName().c_str(),
                                                NULL, &download_url,
                                                &title, &description))
      return false;

    // Get required permissions description.
    std::string permissions_msg;
    permissions->EnumerateAllRequired(
        NewSlot(GetPermissionsDescriptionCallback, &permissions_msg));

    std::string message = GM_("GADGET_CONFIRM_MESSAGE");
    message.append("\n\n")
        .append(title).append("\n")
        .append(download_url).append("\n\n")
        .append(GM_("GADGET_DESCRIPTION"))
        .append(description)
        .append("\n\n")
        .append(GM_("GADGET_REQUIRED_PERMISSIONS"))
        .append("\n")
        .append(permissions_msg);
    int ret = QMessageBox::question(
        NULL,
        QString::fromUtf8(GM_("GADGET_CONFIRM_TITLE")),
        QString::fromUtf8(message.c_str()),
        QMessageBox::Yes| QMessageBox::No,
        QMessageBox::Yes);

    if (ret == QMessageBox::Yes) {
      // TODO: Is it necessary to let user grant individual permissions
      // separately?
      permissions->GrantAllRequired();
      return true;
    }
    return false;
  }

  bool EnumerateGadgetInstancesCallback(int id) {
    if (!LoadGadgetInstance(id))
      gadget_manager_->RemoveGadgetInstance(id);
    // Return true to continue the enumeration.
    return true;
  }

  bool NewGadgetInstanceCallback(int id) {
    Permissions permissions;
    if (gadget_manager_->GetGadgetDefaultPermissions(id, &permissions)) {
      if (!permissions.HasUngranted() || ConfirmGadget(id, &permissions)) {
        // Save initial permissions.
        std::string options_name =
            gadget_manager_->GetGadgetInstanceOptionsName(id);
        OptionsInterface *options = CreateOptions(options_name.c_str());
        // Don't save required permissions.
        permissions.RemoveAllRequired();
        options->PutInternalValue(kPermissionsOption,
                                  Variant(permissions.ToString()));
        options->Flush();
        delete options;
        if (with_plasma_)
          return InstallPlasmaApplet(id);
        else
          return LoadGadgetInstance(id);
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
    return false;
  }

  bool InstallPlasmaApplet(int id) {
    static QString plasma_desktop_template =
        "[Desktop Entry]\n"
        "Encoding=UTF-8\n"
        "Name=%1\n"
        "Comment=%2\n"
        "X-KDE-PluginInfo-Name=%3\n"
        "X-KDE-PluginInfo-Author=%4\n"
        "Icon=%5\n"
        "Type=Service\n"
        "X-KDE-Plasmagik-ApplicationName=\n"
        "X-KDE-Plasmagik-RequiredVersion=\n"
        "X-KDE-PluginInfo-Category=\n"
        "X-KDE-PluginInfo-Email=\n"
        "X-KDE-PluginInfo-EnabledByDefault=true\n"
        "X-KDE-PluginInfo-License=\n"
        "X-KDE-PluginInfo-Version=\n"
        "X-KDE-PluginInfo-Website=\n"
        "X-KDE-ServiceTypes=Plasma/Applet,Plasma/Containment\n"
        "X-Plasma-API=googlegadgets\n";
    // Get the local prefix of kde4
    QString kdedir;
    {
      char path[PATH_MAX];
      FILE *fp = popen("kde-config --localprefix", "r");
      if (fp != NULL) {
        if (fgets(path, PATH_MAX, fp)) {
          int len = strlen(path);
          if (path[len - 1] == '\n') path[len - 1] = '\0';
          kdedir = path;
        }
        pclose(fp);
      }
      if (kdedir == "") {
        kdedir = getenv("KDEHOME");
        if (kdedir == "") {
          LOGE("Can't find localprefix of kde by environment variable KDEHOME"
               " or `kde-config --localprefix`");
          return false;
        }
      }
    }
    LOG("Install plasma applet into %s", kdedir.toUtf8().data());
    std::string author, download_url, title, description;
    if (!gadget_manager_->GetGadgetInstanceInfo(id, "", &author, &download_url,
                                                &title, &description))
      return false;
    std::string path = gadget_manager_->GetGadgetInstancePath(id).c_str();
    std::string options = gadget_manager_->GetGadgetInstanceOptionsName(id);
    QString pkg_name = QString("ggl_%1").arg(id);

    // Create package
    QDir root(kdedir + "/share/apps/plasma/plasmoids/");
    if (!root.cd(pkg_name) && (!root.mkpath(pkg_name) || !root.cd(pkg_name))) {
      LOGE("Failed to create package %s",
           (root.path() + "/" + pkg_name).toUtf8().data());
      return false;
    }
    {
      QFile file(root.path() + "/config.txt");
      file.open(QIODevice::WriteOnly);
      QTextStream out(&file);
      out << QString::fromUtf8(path.c_str())  << "\n";
      out << QString::fromUtf8(options.c_str()) << "\n";
    }

    // Create desktop file
    QString desktop_content = plasma_desktop_template
        .arg(QString::fromUtf8(title.c_str()))          // name
        .arg(QString::fromUtf8(description.c_str()))    // comment
        .arg(pkg_name)                                  // pluginfo-name
        .arg(QString::fromUtf8(author.c_str()))         // author
        .arg("google-gadgets");                         // icon
    QFile file(kdedir + QString("/share/kde4/services/plasma-applet-ggl-%1.desktop").arg(id));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      LOGE("Failed to write plasma-applet-ggl-%d.desktop", id);
      return false;
    }
    QTextStream out(&file);
    out << desktop_content;

    // set option to distinguish this from normal gadgets
    OptionsInterface *opt = CreateOptions(options.c_str());
    opt->Add(kPlasmaID, Variant(true));
    opt->Flush();
    delete opt;

    return true;
  }

  bool LoadGadgetInstance(int id) {
    bool result = false;
    std::string options = gadget_manager_->GetGadgetInstanceOptionsName(id);
    std::string path = gadget_manager_->GetGadgetInstancePath(id);
    if (options.length() && path.length()) {
      OptionsInterface *opt = CreateOptions(options.c_str());
      // Having such option value means this gadget is added as a plasma applet
      // So we just ignore it.
      if (opt->Exists(kPlasmaID)) {
        delete opt;
        return true;
      }
      delete opt;
      result =
          LoadGadget(path.c_str(), options.c_str(), id);
      DLOG("QtHost: Load gadget %s, with option %s, %s",
           path.c_str(), options.c_str(), result ? "succeeded" : "failed");
    }
    return result;
  }

  bool LoadGadget(const char *path, const char *options_name, int instance_id) {
    if (gadgets_.find(instance_id) != gadgets_.end()) {
      // Gadget is already loaded.
      return true;
    }

    Gadget *gadget = new Gadget(host_, path, options_name, instance_id,
                                global_permissions_, debug_console_config_);

    if (!gadget->IsValid()) {
      LOG("Failed to load gadget %s", path);
      delete gadget;
      return false;
    }

    gadget->SetDisplayTarget(Gadget::TARGET_FLOATING_VIEW);
    gadget->GetMainView()->OnOtherEvent(SimpleEvent(Event::EVENT_UNDOCK));

    GadgetInfo *info = new GadgetInfo(gadget);
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
    QWidget *parent = NULL;
    bool record_states = true;
    if (type == ViewHostInterface::VIEW_HOST_DETAILS) {
      parent = static_cast<QWidget*>(gadget->GetMainView()->GetNativeWidget());
      record_states = false;
    }
    QtViewHost *qvh = new QtViewHost(
        type, 1.0, composite_, false, record_states,
        view_debug_mode_, parent);
    QObject::connect(obj_, SIGNAL(show(bool)),
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
           it->second->gadget_->GetManifestInfo(kManifestName).c_str());
      delete it->second;
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
          composite_, false, false, view_debug_mode_,
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
    if (it == gadgets_.end() || !it->second) return;
    GadgetInfo *info = it->second;
    if (info->debug_console_) {
      DLOG("Gadget has already opened a debug console: %p",
           info->debug_console_);
      return;
    }
    NewGadgetDebugConsole(gadget, &info->debug_console_);
  }

  GadgetManagerInterface *gadget_manager_;
  GadgetBrowserHost gadget_browser_host_;
  QtHost *host_;
  int view_debug_mode_;
  Gadget::DebugConsoleConfig debug_console_config_;
  bool composite_;
  bool with_plasma_;
  bool gadgets_shown_;

  DecoratedViewHost *expanded_popout_;
  DecoratedViewHost *expanded_original_;

  QMenu menu_;
  QSystemTrayIcon tray_;
  QtHostObject *obj_;        // provides slots for qt

  typedef std::map<int, GadgetInfo*> GadgetsMap;
  GadgetsMap gadgets_;

  Permissions global_permissions_;
};

QtHost::QtHost(bool composite, int view_debug_mode,
               Gadget::DebugConsoleConfig debug_console, bool with_plasma)
  : impl_(new Impl(this, composite, view_debug_mode,
                   debug_console, with_plasma)) {
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

int QtHost::GetDefaultFontSize() {
  return kDefaultFontSize;
}

bool QtHost::OpenURL(const ggadget::Gadget *gadget, const char *url) {
  return ggadget::qt::OpenURL(gadget, url);
}

} // namespace qt
} // namespace hosts
#include "qt_host_internal.moc"
