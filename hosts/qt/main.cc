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
#include <QtGui/QApplication>
#include <QtGui/QCursor>
#include <QtGui/QMenu>
#include <QtGui/QWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ggadget/script_runtime_interface.h>
#include <ggadget/qt/qt_gadget_widget.h>
#include <ggadget/qt/qt_view_host.h>
#include <ggadget/qt/qt_menu.h>
#include <ggadget/qt/qt_main_loop.h>
#include <ggadget/extension_manager.h>
#include <ggadget/script_runtime_manager.h>
#include <ggadget/ggadget.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/file_manager_factory.h>
#include <ggadget/file_manager_wrapper.h>
#include <ggadget/localized_file_manager.h>
#include <ggadget/host_interface.h>
#include <ggadget/gadget_manager_interface.h>
#include "qt_host.h"

static double g_zoom = 1.;
static int g_debug_mode = 0;
static bool g_useshapemask = false;
static bool g_decorated = true;
static ggadget::qt::QtMainLoop g_main_loop;

static const char *kGlobalExtensions[] = {
  "default-framework",
  "libxml2-xml-parser",
  "default-options",
  "dbus-script-class",
// gtkmoz browser element doesn't support qt.
//  "gtkmoz-browser-element",
  "qt-system-framework",
  "qt-edit-element",
// gst and Qt may not work together.
//  "gst-audio-framework",
#ifdef GGL_HOST_LINUX
  "linux-system-framework",
#endif
  "smjs-script-runtime",
  "curl-xml-http-request",
  "google-gadget-manager",
  "gadget-browser-script-utils",
  NULL
};

static const char *kGlobalResourcePaths[] = {
#ifdef GGL_RESOURCE_DIR
  GGL_RESOURCE_DIR "/resources.gg",
  GGL_RESOURCE_DIR "/resources",
#endif
  "resources.gg",
  "resources",
  NULL
};

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

  QApplication app(argc, argv);
  // Set global main loop
  ggadget::SetGlobalMainLoop(&g_main_loop);

  // Set global file manager.
  ggadget::FileManagerWrapper *fm_wrapper = new ggadget::FileManagerWrapper();
  ggadget::FileManagerInterface *fm;

  for (size_t i = 0; kGlobalResourcePaths[i]; ++i) {
    fm = ggadget::CreateFileManager(kGlobalResourcePaths[i]);
    if (fm) {
      fm_wrapper->RegisterFileManager(ggadget::kGlobalResourcePrefix,
          new ggadget::LocalizedFileManager(fm));
      break;
    }
  }

  if ((fm = ggadget::CreateFileManager(ggadget::kDirSeparatorStr)) != NULL)
    fm_wrapper->RegisterFileManager(ggadget::kDirSeparatorStr, fm);
#ifdef _DEBUG
  std::string dot_slash(".");
  dot_slash += ggadget::kDirSeparatorStr;
  if ((fm = ggadget::CreateFileManager(dot_slash.c_str())) != NULL) {
    fm_wrapper->RegisterFileManager(dot_slash.c_str(), fm);
  }
#endif

  // TODO: Proper profile directory.
  if ((fm = ggadget::CreateFileManager(".")) != NULL)
    fm_wrapper->RegisterFileManager(ggadget::kProfilePrefix, fm);

  ggadget::SetGlobalFileManager(fm_wrapper);

  // Load global extensions.
  ggadget::ExtensionManager *ext_manager =
      ggadget::ExtensionManager::CreateExtensionManager();
  ggadget::ExtensionManager::SetGlobalExtensionManager(ext_manager);

  // Ignore errors when loading extensions.
  for (size_t i = 0; kGlobalExtensions[i]; ++i)
    ext_manager->LoadExtension(kGlobalExtensions[i], false);

  // Register JavaScript runtime.
  ggadget::ScriptRuntimeManager *manager = ggadget::ScriptRuntimeManager::get();
  ggadget::ScriptRuntimeExtensionRegister script_runtime_register(manager);
  ext_manager->RegisterLoadedExtensions(&script_runtime_register);

  ext_manager->SetReadonly();
  hosts::qt::QtHost host(g_debug_mode);

  // Load gadget files.
  if (argc >= 2) {
    ggadget::GadgetManagerInterface *manager = ggadget::GetGadgetManager();

    for (int i = 1; i < argc; ++i)
      manager->NewGadgetInstanceFromFile(argv[i]);
  }
  app.exec();

  return 0;
}
