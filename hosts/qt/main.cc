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

#include <cstdlib>
#include <locale.h>
#include <QtGui/QApplication>
#include <QtGui/QCursor>
#include <QtGui/QMenu>
#include <QtGui/QWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>

#include <ggadget/dir_file_manager.h>
#include <ggadget/extension_manager.h>
#include <ggadget/file_manager_factory.h>
#include <ggadget/file_manager_wrapper.h>
#include <ggadget/gadget.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/gadget_manager_interface.h>
#include <ggadget/host_interface.h>
#include <ggadget/localized_file_manager.h>
#include <ggadget/options_interface.h>
#include <ggadget/qt/qt_view_widget.h>
#include <ggadget/qt/qt_view_host.h>
#include <ggadget/qt/qt_menu.h>
#include <ggadget/qt/qt_main_loop.h>
#include <ggadget/script_runtime_interface.h>
#include <ggadget/script_runtime_manager.h>
#include <ggadget/system_utils.h>
#include "qt_host.h"
#if defined(Q_WS_X11) && defined(HAVE_X11)
#include <X11/extensions/Xrender.h>
#endif

static ggadget::qt::QtMainLoop *g_main_loop;

static const char *kGlobalExtensions[] = {
  "default-framework",
  "libxml2-xml-parser",
  "default-options",
// Disable DBUS script class for now to ensure security.
//  "dbus-script-class",
  "qtwebkit-browser-element",
  "qt-system-framework",
  "qt-edit-element",
// gst and Qt may not work together.
//  "gst-audio-framework",
  "gst-mediaplayer-element",
#ifdef GGL_HOST_LINUX
  "linux-system-framework",
#endif
  "smjs-script-runtime",
  "qt-xml-http-request",
  "google-gadget-manager",
  "gadget-browser-script-utils",
  NULL
};

static const char *kGlobalResourcePaths[] = {
#ifdef _DEBUG
  "resources.gg",
  "resources",
#endif
#ifdef GGL_RESOURCE_DIR
  GGL_RESOURCE_DIR "/resources.gg",
  GGL_RESOURCE_DIR "/resources",
#endif
  NULL
};

static const char *g_help_string =
  "Usage: %s [Options] [Gadgets]\n"
  "Options:\n"
#ifdef _DEBUG
  "  -d mode    Specify debug modes for drawing View:\n"
  "             0 - No debug.\n"
  "             1 - Draw bounding boxes around container elements.\n"
  "             2 - Draw bounding boxes around all elements.\n"
  "             4 - Draw bounding boxes around clip region.\n"
#endif
  "  -h, --help Print this message and exit.\n"
  "\n"
  "Gadgets:\n"
  "  Can specify one or more Desktop Gadget paths.\n"
  "  If any gadgets are specified, they will be installed by using\n"
  "  GadgetManager.\n";

#if defined(Q_WS_X11) && defined(HAVE_X11)
static Display *dpy;
static Colormap colormap = 0;
static Visual *visual = 0;
static void init_argb() {
  dpy = XOpenDisplay(0); // open default display
  if (!dpy) {
    qWarning("Cannot connect to the X server");
    exit(1);
  }

  int screen = DefaultScreen(dpy);
  int eventBase, errorBase;

  if (XRenderQueryExtension(dpy, &eventBase, &errorBase)) {
    int nvi;
    XVisualInfo templ;
    templ.screen  = screen;
    templ.depth   = 32;
    templ.c_class = TrueColor;
    XVisualInfo *xvi = XGetVisualInfo(dpy, VisualScreenMask |
                                      VisualDepthMask |
                                      VisualClassMask, &templ, &nvi);

    for (int i = 0; i < nvi; ++i) {
      XRenderPictFormat *format = XRenderFindVisualFormat(dpy,
                                                          xvi[i].visual);
      if (format->type == PictTypeDirect && format->direct.alphaMask) {
        visual = xvi[i].visual;
        colormap = XCreateColormap(dpy, RootWindow(dpy, screen),
                                   visual, AllocNone);
        break;
      }
    }
  }
}
#endif

int main(int argc, char* argv[]) {
  // set locale according to env vars
  setlocale(LC_ALL, "");

  int debug_mode = 0;

  // Parse command line.
  std::vector<std::string> gadget_paths;
  for (int i = 1; i < argc; i++) {
    if (strcmp("-h", argv[i]) == 0 || strcmp("--help", argv[i]) == 0) {
      printf(g_help_string, argv[0]);
      return 0;
#ifdef _DEBUG
    } else if (strcmp("-d", argv[i]) == 0 || strcmp("--debug", argv[i]) == 0) {
      if (++i < argc) {
        debug_mode = atoi(argv[i]);
      } else {
        debug_mode = 1;
      }
#endif
    } else {
      gadget_paths.push_back(argv[i]);
    }
  }

#if defined(Q_WS_X11) && defined(HAVE_X11)
  init_argb();
  QApplication app(dpy, argc, argv,
                   Qt::HANDLE(visual), Qt::HANDLE(colormap));
#else
  QApplication app(argc, argv);
#endif

  // Set global main loop
  g_main_loop = new ggadget::qt::QtMainLoop();
  ggadget::SetGlobalMainLoop(g_main_loop);

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

  std::string profile_dir =
      ggadget::BuildFilePath(ggadget::GetHomeDirectory().c_str(),
                             ggadget::kDefaultProfileDirectory, NULL);
  fm = ggadget::DirFileManager::Create(profile_dir.c_str(), true);
  if (fm != NULL) {
    fm_wrapper->RegisterFileManager(ggadget::kProfilePrefix, fm);
  } else {
    LOG("Failed to initialize profile directory.");
  }

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
  hosts::qt::QtHost host = hosts::qt::QtHost(debug_mode);

  // Load gadget files.
  if (gadget_paths.size()) {
    ggadget::GadgetManagerInterface *manager = ggadget::GetGadgetManager();

    for (size_t i = 0; i < gadget_paths.size(); ++i)
      manager->NewGadgetInstanceFromFile(gadget_paths[i].c_str());
  }
  g_main_loop->Run();

  return 0;
}
