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
#include <ctype.h>
#include <cstdlib>
#include <locale.h>
#include <signal.h>
#include <QtGui/QApplication>
#include <QtGui/QCursor>
#include <QtGui/QMenu>
#include <QtGui/QWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>
#include <QtGui/QDesktopWidget>

#include <ggadget/gadget.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/gadget_manager_interface.h>
#include <ggadget/host_interface.h>
#include <ggadget/host_utils.h>
#include <ggadget/system_utils.h>
#include <ggadget/string_utils.h>
#include <ggadget/messages.h>
#include <ggadget/qt/qt_view_widget.h>
#include <ggadget/qt/qt_view_host.h>
#include <ggadget/qt/qt_menu.h>
#include <ggadget/qt/qt_main_loop.h>
#include <ggadget/qt/utilities.h>
#include <ggadget/run_once.h>
#include <ggadget/script_runtime_interface.h>
#include <ggadget/xml_http_request_interface.h>

#include "qt_host.h"
#if defined(Q_WS_X11) && defined(HAVE_X11)
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>
#endif

static ggadget::qt::QtMainLoop *g_main_loop;
static const char kRunOnceSocketName[] = "ggl-host-socket";

static const char *kGlobalExtensions[] = {
  "smjs-script-runtime",
  "default-framework",
  "libxml2-xml-parser",
  "default-options",
  "dbus-script-class",
  "qtwebkit-browser-element",
  "qt-system-framework",
  "qt-edit-element",
  "gst-audio-framework",
  "gst-video-element",
#ifdef GGL_HOST_LINUX
  "linux-system-framework",
#endif
  "qt-xml-http-request",
  "analytics-usage-collector",
  "google-gadget-manager",
  NULL
};

static const char *g_help_string =
  "Google Gadgets for Linux " GGL_VERSION
  " (Gadget API version " GGL_API_VERSION ")\n"
  "Usage: " GGL_APP_NAME " [Options] [Gadgets]\n"
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
  "  -bg Run in background\n"
  "  -l loglevel, --log-level loglevel\n"
  "      Specify the minimum gadget.debug log level.\n"
  "      0 - Trace(All)  1 - Info  2 - Warning  3 - Error  >=4 - No log\n"
  "  -ll, --long-log\n"
  "      Output logs using long format.\n"
  "  -dc, --debug-console debug_console_config\n"
  "      Change debug console configuration:\n"
  "      0 - No debug console allowed\n"
  "      1 - Gadgets has debug console menu item\n"
  "      2 - Open debug console when gadget is added to debug startup code\n"
  "  -nc, --no-collector\n"
  "      Disable the usage collector\n"
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

static void OnClientMessage(const std::string &data) {
  ggadget::GetGadgetManager()->NewGadgetInstanceFromFile(data.c_str());
}

static void DefaultSignalHandler(int sig) {
  DLOG("Signal caught: %d, exit.", sig);
  g_main_loop->Quit();
}

int main(int argc, char* argv[]) {
#ifdef _DEBUG
  int log_level = ggadget::LOG_TRACE;
  bool long_log = true;
#else
  int log_level = ggadget::LOG_WARNING;
  bool long_log = false;
#endif
  bool composite = false;
  int debug_mode = 0;
  bool background = false;
  bool enable_collector = true;
  ggadget::Gadget::DebugConsoleConfig debug_console =
      ggadget::Gadget::DEBUG_CONSOLE_DISABLED;
  // set locale according to env vars
  setlocale(LC_ALL, "");


  // Parse command line.
  std::vector<std::string> gadget_paths;
  for (int i = 1; i < argc; i++) {
    if (strcmp("-h", argv[i]) == 0 || strcmp("--help", argv[i]) == 0) {
      printf("%s", g_help_string);
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
          kGlobalExtensions[0] = "qt-script-runtime";
          printf("QtScript runtime is chosen. It's still incomplete\n");
        }
      }
#endif
    } else if (strcmp("-l", argv[i]) == 0 ||
               strcmp("--log-level", argv[i]) == 0) {
      if (++i < argc)
        log_level = atoi(argv[i]);
    } else if (strcmp("-ll", argv[i]) == 0 ||
               strcmp("--long-log", argv[i]) == 0) {
      long_log = true;
    } else if (strcmp("-dc", argv[i]) == 0 ||
               strcmp("--debug-console", argv[i]) == 0) {
      debug_console = ggadget::Gadget::DEBUG_CONSOLE_ON_DEMMAND;
      if (++i < argc) {
        debug_console =
            static_cast<ggadget::Gadget::DebugConsoleConfig>(atoi(argv[i]));
      }
    } else if (strcmp("-nc", argv[i]) == 0 ||
               strcmp("--no-collector", argv[i]) == 0) {
      enable_collector = false;
    } else if (strcmp("-bg", argv[i]) == 0) {
      background = true;
    } else {
      std::string path = ggadget::GetAbsolutePath(argv[i]);
      gadget_paths.push_back(path);
    }
  }
  // Parse command line before create QApplication because it will eat some
  // argv like -bg
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

  std::string profile_dir = ggadget::BuildFilePath(
      ggadget::GetHomeDirectory().c_str(),
      ggadget::kDefaultProfileDirectory,
      NULL);

  QString error;
  ggadget::qt::GGLInitFlags flags;
  if (long_log)
    flags |= ggadget::qt::GGL_INIT_FLAG_LONG_LOG;
  if (enable_collector)
    flags |= ggadget::qt::GGL_INIT_FLAG_COLLECTOR;

  g_main_loop = new ggadget::qt::QtMainLoop();
  if (!ggadget::qt::InitGGL(g_main_loop, GGL_APP_NAME,
                            profile_dir.c_str(),
                            kGlobalExtensions,
                            log_level,
                            flags,
                            &error)) {
    QMessageBox::information(NULL, "Google Gadgets", error);
    return 1;
  }

  ggadget::RunOnce run_once(
      ggadget::BuildFilePath(profile_dir.c_str(),
                             kRunOnceSocketName,
                             NULL).c_str());
  run_once.ConnectOnMessage(ggadget::NewSlot(OnClientMessage));

  if (run_once.IsRunning()) {
    for (size_t i = 0; i < gadget_paths.size(); ++i)
      run_once.SendMessage(gadget_paths[i]);
    DLOG("Another instance already exists.");
    return 0;
  }

  if (background)
    ggadget::Daemonize();

  hosts::qt::QtHost host(composite, debug_mode, debug_console);

  // Load gadget files.
  ggadget::GadgetManagerInterface *gadget_manager = ggadget::GetGadgetManager();
  for (size_t i = 0; i < gadget_paths.size(); ++i)
    gadget_manager->NewGadgetInstanceFromFile(gadget_paths[i].c_str());

  // Hook popular signals to exit gracefully.
  signal(SIGHUP, DefaultSignalHandler);
  signal(SIGINT, DefaultSignalHandler);
  signal(SIGTERM, DefaultSignalHandler);
  signal(SIGUSR1, DefaultSignalHandler);
  signal(SIGUSR2, DefaultSignalHandler);

  g_main_loop->Run();

  return 0;
}
