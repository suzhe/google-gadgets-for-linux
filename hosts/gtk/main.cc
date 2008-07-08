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
#include <sys/time.h>
#include <ctime>
#include <map>
#include <cstdlib>

#include <ggadget/dir_file_manager.h>
#include <ggadget/extension_manager.h>
#include <ggadget/file_manager_factory.h>
#include <ggadget/file_manager_wrapper.h>
#include <ggadget/gadget.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/gadget_manager_interface.h>
#include <ggadget/gtk/main_loop.h>
#include <ggadget/gtk/single_view_host.h>
#include <ggadget/gtk/utilities.h>
#include <ggadget/host_interface.h>
#include <ggadget/localized_file_manager.h>
#include <ggadget/logger.h>
#include <ggadget/messages.h>
#include <ggadget/options_interface.h>
#include <ggadget/run_once.h>
#include <ggadget/script_runtime_interface.h>
#include <ggadget/script_runtime_manager.h>
#include <ggadget/slot.h>
#include <ggadget/string_utils.h>
#include <ggadget/system_utils.h>
#include <ggadget/xml_http_request_interface.h>
#include <ggadget/xml_parser_interface.h>
#include "sidebar_gtk_host.h"
#include "simple_gtk_host.h"

static ggadget::gtk::MainLoop g_main_loop;

static const char kOptionsName[] = "gtk-host-options";
static const char kRunOnceSocketName[] = "gtk-host-socket";

static const char *kGlobalExtensions[] = {
// default framework must be loaded first, so that the default properties can
// be overrode.
  "default-framework",
  "libxml2-xml-parser",
  "default-options",
  // Disable DBUS script class for now to ensure security.
  // "dbus-script-class",
  "gtk-edit-element",
  "gtkmoz-browser-element",
  "gst-mediaplayer-element",
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

static bool CheckRequiredExtensions() {
  if (!ggadget::GetGlobalFileManager()->FileExists(ggadget::kCommonJS, NULL)) {
    // We can't use localized message here because resource failed to load.
    ggadget::gtk::ShowAlertDialog(
        "Google Gadgets",
        "Program can't start because it failed to load resources");
    return false;
  }

  if (!ggadget::GetXMLParser()) {
    // We can't use localized message here because XML parser is not available.
    ggadget::gtk::ShowAlertDialog(
        "Google Gadgets", "Program can't start because it failed to load the "
        "libxml2-xml-parser module.");
    return false;
  }

  std::string message;
  if (!ggadget::ScriptRuntimeManager::get()->GetScriptRuntime("js"))
    message += "smjs-script-runtime\n";
  if (!ggadget::GetXMLHttpRequestFactory())
    message += "curl-xml-http-request\n";
  if (!ggadget::GetGadgetManager())
    message += "google-gadget-manager\n";

  if (!message.empty()) {
    message = GMS_("LOAD_EXTENSIONS_FAIL") + "\n\n" + message;
    ggadget::gtk::ShowAlertDialog(GM_("GOOGLE_GADGETS"), message.c_str());
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

std::string LogListener(ggadget::LogLevel level, const char *filename, int line,
                        const std::string &message) {
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

static void Handler(const std::string &data) {
  ggadget::GetGadgetManager()->NewGadgetInstanceFromFile(data.c_str());
}

int main(int argc, char* argv[]) {
  gtk_init(&argc, &argv);

  int debug_mode = 0;
  double zoom = 1.0;
  bool decorated = false;
  bool sidebar = true;
  bool background = false;
  int debug_console = -1;

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
  run_once.ConnectOnMessage(ggadget::NewSlot(Handler));

  // Parse command line.
  std::vector<std::string> gadget_paths;
  for (int i = 1; i < argc; i++) {
    if (strcmp("-h", argv[i]) == 0 || strcmp("--help", argv[i]) == 0) {
      printf(g_help_string, argv[0]);
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

  // set locale according to env vars
  setlocale(LC_ALL, "");

  ggadget::ConnectGlobalLogListener(ggadget::NewSlot(LogListener));

  // Puth the process into background in the early stage to prevent from
  // printing any log messages.
  if (background)
    ggadget::Daemonize();

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

  if ((fm = ggadget::CreateFileManager(ggadget::kDirSeparatorStr)) != NULL) {
    fm_wrapper->RegisterFileManager(ggadget::kDirSeparatorStr, fm);
  }

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

  // Register JavaScript runtime.
  ggadget::ScriptRuntimeManager *manager = ggadget::ScriptRuntimeManager::get();
  ggadget::ScriptRuntimeExtensionRegister script_runtime_register(manager);
  ext_manager->RegisterLoadedExtensions(&script_runtime_register);

  if (!CheckRequiredExtensions())
    return 1;

  // Make the global extension manager readonly to avoid the potential
  // danger that a bad gadget register local extensions into the global
  // extension manager.
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

  ggadget::HostInterface *host;
  ggadget::OptionsInterface *options = ggadget::CreateOptions(kOptionsName);

  if (sidebar) {
    host = new hosts::gtk::SideBarGtkHost(options, decorated,
                                          debug_mode, debug_console);
  } else {
    host = new hosts::gtk::SimpleGtkHost(options, zoom, decorated,
                                         debug_mode, debug_console);
  }

#ifdef _DEBUG
  std::vector<ggadget::Gadget *> temp_gadgets;
#endif

  // Load gadget files.
  if (gadget_paths.size()) {
    ggadget::GadgetManagerInterface *manager = ggadget::GetGadgetManager();
    for (size_t i = 0; i < gadget_paths.size(); ++i) {
      manager->NewGadgetInstanceFromFile(gadget_paths[i].c_str());
    }
  }

  host->Run();

#ifdef _DEBUG
  for (size_t i = 0; i < temp_gadgets.size(); ++i)
    delete temp_gadgets[i];
#endif

  delete host;
  delete options;

  return 0;
}
