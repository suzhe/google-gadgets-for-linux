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

#include <cstdlib>
#include <locale.h>
#include <QApplication>
#include <QCursor>
#include <QMenu>
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ggadget/script_runtime_interface.h>
#include <ggadget/qt/qt_gadget_widget.h>
#include <ggadget/qt/qt_gadget_host.h>
#include <ggadget/qt/qt_view_host.h>
#include <ggadget/qt/qt_menu.h>
#include <ggadget/qt/qt_main_loop.h>
#include <ggadget/extension_manager.h>
#include <ggadget/script_runtime_manager.h>
#include <ggadget/ggadget.h>

static double g_zoom = 1.;
static int g_debug_mode = 0;
static bool g_composited = false;
static bool g_useshapemask = false;
static bool g_decorated = true;
static ggadget::qt::QtMainLoop main_loop;

static const char *kGlobalExtensions[] = {
  "default-framework",
  "libxml2-xml-parser",
  "dbus-script-class",
  "gtkmoz-browser-element",
  "gst-audio-framework",
#ifdef GGL_HOST_LINUX
  "linux-system-framework",
#endif
  "smjs-script-runtime",
  "curl-xml-http-request",
  NULL
};

static ggadget::qt::QtGadgetHost *gadget_host;
static QWidget* CreateGadgetUI(const char *base_path) {
  gadget_host = new ggadget::qt::QtGadgetHost(g_composited,
                                              g_useshapemask, g_zoom,
                                              g_debug_mode);
  if (!gadget_host->LoadGadget(base_path)) {
    LOG("Failed to load gadget from: %s", base_path);
    exit(-1);
    return NULL;
  }
  ggadget::qt::QtViewHost *view_host =
        static_cast<ggadget::qt::QtViewHost*>(
            gadget_host->GetGadget()->GetMainViewHost());

  return view_host->GetWidget();
}

class MainWidget : public QWidget {
  Q_OBJECT
 public slots:
  void OnShowMenu() {
    QMenu qmenu; // = new QMenu();
//    ggadget::MenuInterface *menu = new ggadget::qt::QtMenu(qmenu);
    ggadget::qt::QtMenu menu(&qmenu);
    gadget_host->GetGadget()->OnAddCustomMenuItems(&menu);
    qmenu.addSeparator();
    qmenu.addAction("About", this, SLOT(OnAbout()));
    qmenu.exec(QCursor::pos());
  }
  void OnAbout() {
    LOG("About");
  }
 public:
  MainWidget(const char *base_path) {
    QPushButton *menu_button = new QPushButton("Menu");
    QObject::connect(menu_button, SIGNAL(clicked()), this, SLOT(OnShowMenu()));

    QPushButton *exit = new QPushButton("Exit");
    QObject::connect(exit, SIGNAL(clicked()), qApp, SLOT(quit()));

    QWidget *gadget = CreateGadgetUI(base_path);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(menu_button);
    layout->addWidget(gadget);
    layout->addWidget(exit);

    setLayout(layout);
  }
};
#include "main.moc"

int main(int argc, char* argv[]) {
  // set locale according to env vars
  setlocale(LC_ALL, "");
  const char *gadget_name = "examples/test";

  if (argc >= 2) gadget_name = argv[1];

  if (argc >= 3) {
    sscanf(argv[2], "%lg", &g_zoom);
    if (g_zoom <= 0 || g_zoom > 5) {
      LOG("Zoom level invalid, resetting to 1");
      g_zoom = 1.;
    }
  }

  if (argc >= 4) {
    sscanf(argv[3], "%d", &g_debug_mode);
    if (g_debug_mode < 0 || g_debug_mode > 2) {
      LOG("Debug mode invalid, resetting to 0");
      g_debug_mode = 0;
    }
  }

  if (argc >= 5) {
    int useshapemask;
    sscanf(argv[4], "%d", &useshapemask);
    g_useshapemask = (useshapemask != 0);
  }

  if (argc >= 6) {
    int decorated;
    sscanf(argv[5], "%d", &decorated);
    g_decorated = (decorated != 0);
  }

  // Set global main loop
  ggadget::SetGlobalMainLoop(&main_loop);

  // Load global extensions.
  ggadget::ExtensionManager *ext_manager =
      ggadget::ExtensionManager::CreateExtensionManager();

  // Ignore errors when loading extensions.
  for (size_t i = 0; kGlobalExtensions[i]; ++i)
    ext_manager->LoadExtension(kGlobalExtensions[i], false);

  // Register JavaScript runtime.
  ggadget::ScriptRuntimeManager *manager = ggadget::ScriptRuntimeManager::get();
  ggadget::ScriptRuntimeExtensionRegister script_runtime_register(manager);
  ext_manager->RegisterLoadedExtensions(&script_runtime_register);

  ggadget::ExtensionManager::SetGlobalExtensionManager(ext_manager);

  QApplication app(argc, argv);

  MainWidget widget(gadget_name);
  widget.show();
  return app.exec();
}
