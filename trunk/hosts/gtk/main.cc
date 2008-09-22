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

#include <gtk/gtk.h>
#include <glib/gthread.h>
#include <locale.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include <map>
#include <cstdlib>

#include <ggadget/extension_manager.h>
#include <ggadget/file_manager_factory.h>
#include <ggadget/gadget.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/gadget_manager_interface.h>
#include <ggadget/gtk/main_loop.h>
#include <ggadget/gtk/single_view_host.h>
#include <ggadget/gtk/utilities.h>
#include <ggadget/host_interface.h>
#include <ggadget/host_utils.h>
#include <ggadget/logger.h>
#include <ggadget/messages.h>
#include <ggadget/run_once.h>
#include <ggadget/script_runtime_interface.h>
#include <ggadget/script_runtime_manager.h>
#include <ggadget/slot.h>
#include <ggadget/string_utils.h>
#include <ggadget/system_utils.h>
#include <ggadget/xml_http_request_interface.h>
#include "sidebar_gtk_host.h"
#include "simple_gtk_host.h"

static ggadget::gtk::MainLoop g_main_loop;

static const char kOptionsName[] = "gtk-host-options";
static const char kRunOnceSocketName[] = "ggl-host-socket";

static const char *kGlobalExtensions[] = {
// default framework must be loaded first, so that the default properties can
// be overrode.
  "default-framework",
  "libxml2-xml-parser",
  "default-options",
  "dbus-script-class",
  "gtk-edit-element",
  "gtkmoz-browser-element",
  "gtk-flash-element",
  "gst-video-element",
  "gtk-system-framework",
  "gst-audio-framework",
#ifdef GGL_HOST_LINUX
  "linux-system-framework",
#endif
  "smjs-script-runtime",
  "curl-xml-http-request",
  "google-gadget-manager",
  NULL
};

static const char *g_help_string =
  "Google Gadgets for Linux " GGL_VERSION
  " (Gadget API version " GGL_API_VERSION ")\n"
  "Usage: " GGL_APP_NAME "[Options] [Gadgets]\n"
  "Options:\n"
#ifdef _DEBUG
  "  -d mode, --debug mode\n"
  "      Specify debug modes for drawing View:\n"
  "      0 - No debug.\n"
  "      1 - Draw bounding boxes around container elements.\n"
  "      2 - Draw bounding boxes around all elements.\n"
  "      4 - Draw bounding boxes around clip region.\n"
#endif
  "  -z zoom, --zoom zoom\n"
  "      Specify initial zoom factor for View, no effect for sidebar.\n"
  "  -b, --border\n"
  "      Draw window border for Main View.\n"
  "  -ns, --no-sidebar\n"
  "      Use dashboard mode instead of sidebar mode.\n"
  "  -bg, --background\n"
  "      Run in background.\n"
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
  "  -h, --help\n"
  "      Print this message and exit.\n"
  "\n"
  "Gadgets:\n"
  "  Can specify one or more Desktop Gadget paths.\n"
  "  If any gadgets are specified, they will be installed by using\n"
  "  GadgetManager.\n";

static void OnClientMessage(const std::string &data) {
  ggadget::GetGadgetManager()->NewGadgetInstanceFromFile(data.c_str());
}

static void DefaultSignalHandler(int sig) {
  DLOG("Signal caught: %d, exit.", sig);
  gtk_main_quit();
}

int main(int argc, char* argv[]) {
  gtk_init(&argc, &argv);

#ifdef _DEBUG
  int log_level = ggadget::LOG_TRACE;
  bool long_log = true;
#else
  int log_level = ggadget::LOG_WARNING;
  bool long_log = false;
#endif
  int debug_mode = 0;
  double zoom = 1.0;
  bool decorated = false;
  bool sidebar = true;
  bool background = false;
  ggadget::Gadget::DebugConsoleConfig debug_console =
      ggadget::Gadget::DEBUG_CONSOLE_DISABLED;

  // Set global main loop
  ggadget::SetGlobalMainLoop(&g_main_loop);

  std::string profile_dir =
      ggadget::BuildFilePath(ggadget::GetHomeDirectory().c_str(),
                             ggadget::kDefaultProfileDirectory, NULL);
  ggadget::EnsureDirectories(profile_dir.c_str());

  ggadget::RunOnce run_once(
      ggadget::BuildFilePath(profile_dir.c_str(),
                             kRunOnceSocketName,
                             NULL).c_str());
  run_once.ConnectOnMessage(ggadget::NewSlot(OnClientMessage));

  // Parse command line.
  std::vector<std::string> gadget_paths;
  for (int i = 1; i < argc; i++) {
    if (strcmp("-h", argv[i]) == 0 || strcmp("--help", argv[i]) == 0) {
      printf("%s", g_help_string);
      return 0;
    } else if (strcmp("-b", argv[i]) == 0 ||
               strcmp("--border", argv[i]) == 0) {
      decorated = true;
    } else if (strcmp("-bg", argv[i]) == 0 ||
               strcmp("--background", argv[i]) == 0) {
      background = true;
    } else if (strcmp("-ns", argv[i]) == 0 ||
               strcmp("--no-sidebar", argv[i]) == 0) {
      sidebar = false;
#ifdef _DEBUG
    } else if (strcmp("-d", argv[i]) == 0 || strcmp("--debug", argv[i]) == 0) {
      if (++i < argc) {
        debug_mode = atoi(argv[i]);
      } else {
        debug_mode = 1;
      }
#endif
    } else if (strcmp("-z", argv[i]) == 0 || strcmp("--zoom", argv[i]) == 0) {
      if (++i < argc) {
        zoom = strtod(argv[i], NULL);
        if (zoom <= 0)
          zoom = 1.0;
        DLOG("Use zoom factor %lf", zoom);
      }
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
    gdk_notify_startup_complete();
    DLOG("Another instance already exists.");
    exit(0);
  }

  // set locale according to env vars
  setlocale(LC_ALL, "");

  ggadget::SetupLogger(log_level, long_log);

  // Puth the process into background in the early stage to prevent from
  // printing any log messages.
  if (background)
    ggadget::Daemonize();

  // Set global file manager.
  ggadget::SetupGlobalFileManager(profile_dir.c_str());

  // Load global extensions.
  ggadget::ExtensionManager *ext_manager =
      ggadget::ExtensionManager::CreateExtensionManager();
  ggadget::ExtensionManager::SetGlobalExtensionManager(ext_manager);

  // Ignore errors when loading extensions.
  for (size_t i = 0; kGlobalExtensions[i]; ++i)
    ext_manager->LoadExtension(kGlobalExtensions[i], false);

  // Register JavaScript runtime.
  ggadget::ScriptRuntimeManager *script_runtime_manager =
      ggadget::ScriptRuntimeManager::get();
  ggadget::ScriptRuntimeExtensionRegister script_runtime_register(
      script_runtime_manager);
  ext_manager->RegisterLoadedExtensions(&script_runtime_register);

  std::string error;
  if (!ggadget::CheckRequiredExtensions(&error)) {
    // Don't use _GM here because localized messages may be unavailable.
    ggadget::gtk::ShowAlertDialog("Google Gadgets", error.c_str());
    return 1;
  }

  // Make the global extension manager readonly to avoid the potential
  // danger that a bad gadget register local extensions into the global
  // extension manager.
  ext_manager->SetReadonly();
  ggadget::InitXHRUserAgent(GGL_APP_NAME);
  // Initialize the gadget manager before creating the host.
  ggadget::GadgetManagerInterface *gadget_manager = ggadget::GetGadgetManager();
  gadget_manager->Init();

  ggadget::HostInterface *host;
  ggadget::OptionsInterface *options = ggadget::CreateOptions(kOptionsName);

  if (sidebar) {
    host = new hosts::gtk::SideBarGtkHost(options, decorated,
                                          debug_mode, debug_console);
  } else {
    host = new hosts::gtk::SimpleGtkHost(options, zoom, decorated,
                                         debug_mode, debug_console);
  }

  // Load gadget files.
  if (gadget_paths.size()) {
    for (size_t i = 0; i < gadget_paths.size(); ++i) {
      gadget_manager->NewGadgetInstanceFromFile(gadget_paths[i].c_str());
    }
  }

  // Hook popular signals to exit gracefully.
  signal(SIGHUP, DefaultSignalHandler);
  signal(SIGINT, DefaultSignalHandler);
  signal(SIGTERM, DefaultSignalHandler);
  signal(SIGUSR1, DefaultSignalHandler);
  signal(SIGUSR2, DefaultSignalHandler);

  gdk_notify_startup_complete();
  gtk_main();

  delete host;
  delete options;

  return 0;
}
