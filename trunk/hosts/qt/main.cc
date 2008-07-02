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

#include <sys/time.h>
#include <time.h>
#include <cstdlib>
#include <locale.h>
#include <QtGui/QApplication>
#include <QtGui/QCursor>
#include <QtGui/QMenu>
#include <QtGui/QWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>

#include <ggadget/dir_file_manager.h>
#include <ggadget/extension_manager.h>
#include <ggadget/file_manager_factory.h>
#include <ggadget/file_manager_wrapper.h>
#include <ggadget/gadget.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/gadget_manager_interface.h>
#include <ggadget/host_interface.h>
#include <ggadget/localized_file_manager.h>
#include <ggadget/messages.h>
#include <ggadget/options_interface.h>
#include <ggadget/qt/qt_view_widget.h>
#include <ggadget/qt/qt_view_host.h>
#include <ggadget/qt/qt_menu.h>
#include <ggadget/qt/qt_main_loop.h>
#include <ggadget/run_once.h>
#include <ggadget/script_runtime_interface.h>
#include <ggadget/script_runtime_manager.h>
#include <ggadget/system_utils.h>
#include <ggadget/xml_http_request_interface.h>
#include <ggadget/xml_parser_interface.h>
#include "qt_host.h"
#if defined(Q_WS_X11) && defined(HAVE_X11)
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>
#endif

static ggadget::qt::QtMainLoop *g_main_loop;
static const char kRunOnceSocketName[] = "qt-host-socket";

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
  "qt-xml-http-request",
  "google-gadget-manager",
  NULL
};

static bool CheckRequiredExtensions() {
  if (!ggadget::GetGlobalFileManager()->FileExists(ggadget::kCommonJS, NULL)) {
    // We can't use localized message here because resource failed to load. 
    QMessageBox::information(
        NULL, "Google Gadgets",
        "Program can't start because it failed to load resources");
    return false;
  }

  if (!ggadget::GetXMLParser()) {
    // We can't use localized message here because XML parser is not available.
    QMessageBox::information(
        NULL, "Google Gadgets",
        "Program can't start because it failed to load the "
        "libxml2-xml-parser module.");
    return false;
  }

  std::string message;
  if (!ggadget::ScriptRuntimeManager::get()->GetScriptRuntime("js"))
    message += "smjs-script-runtime\n";
  if (!ggadget::GetXMLHttpRequestFactory())
    message += "qt-xml-http-request\n";
  if (!ggadget::GetGadgetManager())
    message += "google-gadget-manager\n";

  if (!message.empty()) {
    message = GMS_("LOAD_EXTENSIONS_FAIL") + "\n\n" + message;
    QMessageBox::information(
        NULL, QString::fromUtf8(GM_("GOOGLE_GADGETS")),
        QString::fromUtf8(message.c_str()));
    return false;
  }
  return true;
}

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

#ifdef _DEBUG
static int g_log_level = ggadget::LOG_TRACE;
static bool g_long_log = true;
#else
static int g_log_level = ggadget::LOG_WARNING;
static bool g_long_log = false;
#endif

static std::string LogListener(ggadget::LogLevel level, const char *filename,
                               int line, const std::string &message) {
  if (level >= g_log_level) {
    if (g_long_log) {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      printf("%02d:%02d.%03d: ",
             static_cast<int>(tv.tv_sec / 60 % 60),
             static_cast<int>(tv.tv_sec % 60),
             static_cast<int>(tv.tv_usec / 1000));
      if (filename) {
        // Print only the last part of the file name.
        const char *name = strrchr(filename, '/');
        if (name)
          filename = name + 1;
        printf("%s:%d: ", filename, line);
      }
    }
    printf("%s\n", message.c_str());
    fflush(stdout);
  }
  return message;
}

static const char *g_help_string =
  "Google Gadgets for Linux " GGL_VERSION "\n"
  "Usage: %s [Options] [Gadgets]\n"
  "Options:\n"
#ifdef _DEBUG
  "  -d mode, --debug mode\n"
  "      Specify debug modes for drawing View:\n"
  "      0 - No debug.\n"
  "      1 - Draw bounding boxes around container elements.\n"
  "      2 - Draw bounding boxes around all elements.\n"
  "      4 - Draw bounding boxes around clip region.\n"
#endif
#if QT_VERSION >= 0x040400
  "  -s script_runtime, --script-runtime script_runtime\n"
  "      Specify which script runtime to use\n"
  "      smjs - spidermonkey js runtime\n"
  "      qt   - QtScript js runtime(experimental)\n"
#endif
  "  -l loglevel, --log-level loglevel\n"
  "      Specify the minimum gadget.debug log level.\n"
  "      0 - Trace(All)  1 - Info  2 - Warning  3 - Error  >=4 - No log\n"
  "  -ll, --long-log\n"
  "      Output logs using long format.\n"
  "  -dc, --debug-console debug_console_config\n"
  "      Change debug console configuration (will be saved in config file):\n"
  "      0 - No debug console allowed\n"
  "      1 - Gadgets has debug console menu item\n"
  "      2 - Open debug console when gadget is added to debug startup code\n"
  "  -h, --help\n"
  "      Print this message and exit.\n"
  "\n"
  "Gadgets:\n"
  "  Can specify one or more Desktop Gadget paths.\n"
  "  If any gadgets are specified, they will be installed by using\n"
  "  GadgetManager.\n";

#if defined(Q_WS_X11) && defined(HAVE_X11)
static Display *dpy;
static Colormap colormap = 0;
static Visual *visual = 0;
static bool InitArgb() {
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
        return true;
      }
    }
  }
  return false;
}
static bool CheckCompositingManager(Display *dpy) {
  Atom net_wm_state = XInternAtom(dpy, "_NET_WM_CM_S0", False);
  return XGetSelectionOwner(dpy, net_wm_state);
}
#endif

static void Handler(const std::string &data) {
  ggadget::GetGadgetManager()->NewGadgetInstanceFromFile(data.c_str());
}

int main(int argc, char* argv[]) {
  bool composite = false;
  int debug_mode = 0;
  int debug_console = -1;
  const char* js_runtime = "smjs-script-runtime";
  // set locale according to env vars
  setlocale(LC_ALL, "");

#if defined(Q_WS_X11) && defined(HAVE_X11)
  if (InitArgb() && CheckCompositingManager(dpy)) {
    composite = true;
  } else {
    visual = NULL;
    if (colormap) {
      XFreeColormap(dpy, colormap);
      colormap = 0;
    }
  }
  QApplication app(dpy, argc, argv,
                   Qt::HANDLE(visual), Qt::HANDLE(colormap));
#else
  QApplication app(argc, argv);
#endif

  // Set global main loop
  g_main_loop = new ggadget::qt::QtMainLoop();
  ggadget::SetGlobalMainLoop(g_main_loop);

  std::string profile_dir =
      ggadget::BuildFilePath(ggadget::GetHomeDirectory().c_str(),
                             ggadget::kDefaultProfileDirectory, NULL);

  ggadget::EnsureDirectories(profile_dir.c_str());

  ggadget::RunOnce run_once(
      ggadget::BuildFilePath(profile_dir.c_str(),
                             kRunOnceSocketName,
                             NULL).c_str());
  run_once.ConnectOnMessage(ggadget::NewSlot(Handler));

  ggadget::ConnectGlobalLogListener(ggadget::NewSlot(LogListener));


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
#if QT_VERSION >= 0x040400
    } else if (strcmp("-s", argv[i]) == 0 ||
               strcmp("--script-runtime", argv[i]) == 0) {
      if (++i < argc) {
        if (strcmp(argv[i], "qt") == 0) {
          js_runtime = "qt-script-runtime";
          printf("QtScript runtime is chosen. It's still incomplete\n"); 
        }
      }
#endif
    } else if (strcmp("-l", argv[i]) == 0 ||
               strcmp("--log-level", argv[i]) == 0) {
      if (++i < argc)
        g_log_level = atoi(argv[i]);
    } else if (strcmp("-ll", argv[i]) == 0 ||
               strcmp("--long-log", argv[i]) == 0) {
      g_long_log = true;
    } else if (strcmp("-dc", argv[i]) == 0 ||
               strcmp("--debug-console", argv[i]) == 0) {
      debug_console = 1;
      if (++i < argc)
        debug_console = atoi(argv[i]);
   } else {
      std::string path = ggadget::GetAbsolutePath(argv[i]);
      if (run_once.IsRunning()) {
        run_once.SendMessage(path);
      } else {
        gadget_paths.push_back(path);
      }
    }
  }

  if (run_once.IsRunning()) {
    DLOG("Another instance already exists.");
    exit(0);
  }

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
  ext_manager->LoadExtension(js_runtime, false);

  // Register JavaScript runtime.
  ggadget::ScriptRuntimeManager *manager = ggadget::ScriptRuntimeManager::get();
  ggadget::ScriptRuntimeExtensionRegister script_runtime_register(manager);
  ext_manager->RegisterLoadedExtensions(&script_runtime_register);

  if (!CheckRequiredExtensions())
    return 1;

  ext_manager->SetReadonly();

  ggadget::OptionsInterface *global_options = ggadget::GetGlobalOptions();
  if (global_options) {
    if (debug_console == -1) {
      debug_console = 0;
      global_options->GetValue(ggadget::kDebugConsoleOption)
          .ConvertToInt(&debug_console);
    } else {
      global_options->PutValue(ggadget::kDebugConsoleOption,
                               ggadget::Variant(debug_console));
    }
  }

  hosts::qt::QtHost host = hosts::qt::QtHost(composite, debug_mode, debug_console);

  // Load gadget files.
  if (gadget_paths.size()) {
    ggadget::GadgetManagerInterface *manager = ggadget::GetGadgetManager();

    for (size_t i = 0; i < gadget_paths.size(); ++i)
      manager->NewGadgetInstanceFromFile(gadget_paths[i].c_str());
  }
  g_main_loop->Run();

  return 0;
}
