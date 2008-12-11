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
#include <ggadget/usage_collector_interface.h>
#include <ggadget/xml_http_request_interface.h>
#include "sidebar_gtk_host.h"
#include "simple_gtk_host.h"
#include "standalone_gtk_host.h"

using ggadget::Variant;

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
  "analytics-usage-collector",
  "google-gadget-manager",
  NULL
};

static const char kHelpString[] =
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
  "  -b, --border\n"
  "      Draw window border for Main View.\n"
  "  -nt, --no-transparent\n"
  "      Don't use transparent window.\n"
  "  -nd, --no-decorator\n"
  "      Don't use main view decorator (Only for standalone gadgets).\n"
  "  -ns, --no-sidebar\n"
  "      Use dashboard mode instead of sidebar mode.\n"
  "  -mb, --matchbox\n"
  "      Enable matchbox workaround.\n"
  "  -bg, --background\n"
  "      Run in background.\n"
  "  -sa, --standalone\n"
  "      Run specified Gadgets in standalone mode.\n"
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
  "  -gp, --grant-permissions\n"
  "      Grant all permissions required by gadgets silently.\n"
  "  -h, --help\n"
  "      Print this message and exit.\n"
  "\n"
  "Gadgets:\n"
  "  Can specify one or more Desktop Gadget paths.\n"
  "  If any gadgets are specified, they will be installed by using\n"
  "  GadgetManager, or run as standalone windows if option -sa is specified.\n";

enum ArgumentID {
  ARG_DEBUG = 1,
  ARG_BORDER,
  ARG_NO_TRANSPARENT,
  ARG_NO_DECORATOR,
  ARG_NO_SIDEBAR,
  ARG_MATCHBOX,
  ARG_BACKGROUND,
  ARG_STANDALONE,
  ARG_LOG_LEVEL,
  ARG_LONG_LOG,
  ARG_DEBUG_CONSOLE,
  ARG_NO_COLLECTOR,
  ARG_GRANT_PERMISSIONS,
  ARG_HELP
};

static const ggadget::HostArgumentInfo kArgumentsInfo[] = {
#ifdef _DEBUG
  { ARG_DEBUG,             Variant::TYPE_INT64, "-d",  "--debug" },
#endif
  { ARG_BORDER,            Variant::TYPE_BOOL,  "-b",  "--border" },
  { ARG_NO_TRANSPARENT,    Variant::TYPE_BOOL,  "-nt", "--no-transparent" },
  { ARG_NO_DECORATOR,      Variant::TYPE_BOOL,  "-nd", "--no-decorator" },
  { ARG_NO_SIDEBAR,        Variant::TYPE_BOOL,  "-ns", "--no-sidebar" },
  { ARG_MATCHBOX,          Variant::TYPE_BOOL,  "-mb", "--matchbox" },
  { ARG_BACKGROUND,        Variant::TYPE_BOOL,  "-bg", "--background" },
  { ARG_STANDALONE,        Variant::TYPE_BOOL,  "-sa", "--standalone" },
  { ARG_LOG_LEVEL,         Variant::TYPE_INT64, "-l",  "--log-level" },
  { ARG_LONG_LOG,          Variant::TYPE_BOOL,  "-ll", "--long-log" },
  { ARG_DEBUG_CONSOLE,     Variant::TYPE_INT64, "-dc", "--debug-console" },
  { ARG_NO_COLLECTOR,      Variant::TYPE_BOOL,  "-nc", "--no-collector" },
  { ARG_GRANT_PERMISSIONS, Variant::TYPE_BOOL, "-gp", "--grant-permissions" },
  { ARG_HELP,              Variant::TYPE_BOOL,  "-h",  "--help" },
  { -1,                    Variant::TYPE_VOID, NULL, NULL } // End of list
};

struct Arguments {
  Arguments()
    : debug_mode(0),
      wm_border(false),
      no_sidebar(false),
      no_transparent(false),
      no_decorator(false),
      matchbox(false),
      background(false),
      standalone(false),
#ifdef _DEBUG
      log_level(ggadget::LOG_TRACE),
      long_log(true),
#else
      log_level(ggadget::LOG_WARNING),
      long_log(false),
#endif
      debug_console(ggadget::Gadget::DEBUG_CONSOLE_DISABLED),
      no_collector(false),
      grant_permissions(false) {
  }

  int debug_mode;
  bool wm_border;
  bool no_sidebar;
  bool no_transparent;
  bool no_decorator;
  bool matchbox;
  bool background;
  bool standalone;
  int log_level;
  bool long_log;
  ggadget::Gadget::DebugConsoleConfig debug_console;
  bool no_collector;
  bool grant_permissions;
};

static ggadget::HostArgumentParser g_argument_parser(kArgumentsInfo);
static Arguments g_arguments;

static ggadget::HostInterface *g_managed_host = NULL;
static int g_live_host_count = 0;
static bool g_gadget_manager_initialized = false;
static ggadget::Signal0<void> g_exit_all_hosts_signal;

static void ExtractArgumentsValue() {
  // Resets arguments value.
  g_arguments = Arguments();

  // no_transparent must be initialized dynamically.
  g_arguments.no_transparent = !ggadget::gtk::SupportsComposite(NULL);

  ggadget::Variant arg_value;
  if (g_argument_parser.GetArgumentValue(ARG_DEBUG, &arg_value))
    g_arguments.debug_mode = ggadget::VariantValue<int>()(arg_value);
  if (g_argument_parser.GetArgumentValue(ARG_BORDER, &arg_value))
    g_arguments.wm_border = ggadget::VariantValue<bool>()(arg_value);
  if (g_argument_parser.GetArgumentValue(ARG_NO_SIDEBAR, &arg_value))
    g_arguments.no_sidebar = ggadget::VariantValue<bool>()(arg_value);
  if (g_argument_parser.GetArgumentValue(ARG_NO_TRANSPARENT, &arg_value))
    g_arguments.no_transparent = ggadget::VariantValue<bool>()(arg_value);
  if (g_argument_parser.GetArgumentValue(ARG_NO_DECORATOR, &arg_value))
    g_arguments.no_decorator = ggadget::VariantValue<bool>()(arg_value);
  if (g_argument_parser.GetArgumentValue(ARG_MATCHBOX, &arg_value))
    g_arguments.matchbox = ggadget::VariantValue<bool>()(arg_value);
  if (g_argument_parser.GetArgumentValue(ARG_BACKGROUND, &arg_value))
    g_arguments.background = ggadget::VariantValue<bool>()(arg_value);
  if (g_argument_parser.GetArgumentValue(ARG_STANDALONE, &arg_value))
    g_arguments.standalone = ggadget::VariantValue<bool>()(arg_value);
  if (g_argument_parser.GetArgumentValue(ARG_LOG_LEVEL, &arg_value))
    g_arguments.log_level = ggadget::VariantValue<int>()(arg_value);
  if (g_argument_parser.GetArgumentValue(ARG_LONG_LOG, &arg_value))
    g_arguments.long_log = ggadget::VariantValue<bool>()(arg_value);
  if (g_argument_parser.GetArgumentValue(ARG_DEBUG_CONSOLE, &arg_value))
    g_arguments.debug_console =
        static_cast<ggadget::Gadget::DebugConsoleConfig>(
            ggadget::VariantValue<int>()(arg_value));
  if (g_argument_parser.GetArgumentValue(ARG_NO_COLLECTOR, &arg_value))
    g_arguments.no_collector = ggadget::VariantValue<bool>()(arg_value);
  if (g_argument_parser.GetArgumentValue(ARG_GRANT_PERMISSIONS, &arg_value))
    g_arguments.grant_permissions = ggadget::VariantValue<bool>()(arg_value);
}

static void OnHostExit(ggadget::HostInterface *host) {
  if (host == g_managed_host)
    g_managed_host = NULL;

  delete host;

  --g_live_host_count;
  if (g_live_host_count <= 0 && gtk_main_level() > 0) {
    DLOG("No host is running, exit.");
    gtk_main_quit();
  }
}

static int GetHostFlagsFromArguments() {
  int flags = hosts::gtk::GtkHostBase::NONE;
  if (g_arguments.wm_border)
    flags |= hosts::gtk::GtkHostBase::WINDOW_MANAGER_BORDER;
  if (g_arguments.no_decorator)
    flags |= hosts::gtk::GtkHostBase::NO_MAIN_VIEW_DECORATOR;
  if (g_arguments.no_transparent)
    flags |= hosts::gtk::GtkHostBase::NO_TRANSPARENT;
  if (g_arguments.matchbox)
    flags |= hosts::gtk::GtkHostBase::MATCHBOX_WORKAROUND;
  if (g_arguments.grant_permissions)
    flags |= hosts::gtk::GtkHostBase::GRANT_PERMISSIONS;

  return flags;
}

static ggadget::HostInterface *GetManagedHost() {
  if (!g_managed_host) {
    // Init gadget manager before creating managed host.
    if (!g_gadget_manager_initialized) {
      ggadget::GetGadgetManager()->Init();
      g_gadget_manager_initialized = true;
    }
    hosts::gtk::GtkHostBase *host;
    if (g_arguments.no_sidebar) {
      host = new hosts::gtk::SimpleGtkHost(
          kOptionsName, GetHostFlagsFromArguments(),
          g_arguments.debug_mode, g_arguments.debug_console);
    } else {
      host = new hosts::gtk::SideBarGtkHost(
          kOptionsName, GetHostFlagsFromArguments(),
          g_arguments.debug_mode, g_arguments.debug_console);
    }
    g_managed_host = host;

    // Make sure that the managed host will be remove when exits.
    ggadget::Connection *connection = g_exit_all_hosts_signal.Connect(
        ggadget::NewSlot(host, &hosts::gtk::GtkHostBase::Exit));

    // Make sure that the connection to g_exit_all_hosts_signal will be
    // disconnected when the host exits.
    host->ConnectOnExit(
        ggadget::NewSlot(connection, &ggadget::Connection::Disconnect));

    // Make sure that the host will be deleted when exits.
    host->ConnectOnExit(ggadget::NewSlot(OnHostExit, g_managed_host));
    ++g_live_host_count;
  }
  return g_managed_host;
}

static ggadget::Gadget *LoadManagedGadget(const char *path,
                                          const char *options_name,
                                          int instance_id,
                                          bool show_debug_console) {
  return GetManagedHost()->LoadGadget(path, options_name, instance_id,
                                      show_debug_console);
}

static bool LoadLocalGadget(const std::string &gadget) {
  std::string path = ggadget::GetAbsolutePath(gadget.c_str());
  if (g_arguments.standalone) {
    hosts::gtk::StandaloneGtkHost *host = new hosts::gtk::StandaloneGtkHost(
        GetHostFlagsFromArguments(), g_arguments.debug_mode,
        g_arguments.debug_console);

    // Make sure that the managed host will be remove when exits.
    ggadget::Connection *connection = g_exit_all_hosts_signal.Connect(
        ggadget::NewSlot(static_cast<hosts::gtk::GtkHostBase *>(host),
                         &hosts::gtk::GtkHostBase::Exit));

    // Make sure that the connection to g_exit_all_hosts_signal will be
    // disconnected when the host exits.
    host->ConnectOnExit(
        ggadget::NewSlot(connection, &ggadget::Connection::Disconnect));

    // Make sure that the host will be deleted when exits.
    host->ConnectOnExit(ggadget::NewSlot(
        OnHostExit, static_cast<ggadget::HostInterface*>(host)));

    // Standalone host can't load more than one gadgets, so the LoadGadget task
    // must be handled by managed host.
    host->ConnectOnLoadGadget(ggadget::NewSlot(LoadManagedGadget));
    ++g_live_host_count;

    // Don't care the return value. OnHostExit will be called if it's failed.
    host->Init(path);
  } else {
    ggadget::GetGadgetManager()->NewGadgetInstanceFromFile(path.c_str());
  }
  return true;
}

static void OnClientMessage(const std::string &data) {
  if (data == ggadget::HostArgumentParser::kStartSignature) {
    g_argument_parser.Start();
  } else if (data == ggadget::HostArgumentParser::kFinishSignature) {
    if (g_argument_parser.Finish()) {
      ExtractArgumentsValue();
      if (!g_arguments.standalone) {
        GetManagedHost();
      }
      g_argument_parser.EnumerateRemainedArgs(
          ggadget::NewSlot(LoadLocalGadget));
    }
  } else if (data.length()) {
    g_argument_parser.AppendArgument(data.c_str());
  }
}

static void DefaultSignalHandler(int sig) {
  DLOG("Signal caught: %d, exit.", sig);
  g_exit_all_hosts_signal();
}

static bool SendArgumentCallback(const std::string &arg,
                                 ggadget::RunOnce *run_once) {
  run_once->SendMessage(arg);
  return true;
}

static bool SendPathCallback(const std::string &path,
                             ggadget::RunOnce *run_once) {
  std::string abs_path = ggadget::GetAbsolutePath(path.c_str());
  if (abs_path.length())
    run_once->SendMessage(abs_path);
  return true;
}

int main(int argc, char* argv[]) {
  gtk_init(&argc, &argv);

  // set locale according to env vars
  setlocale(LC_ALL, "");

  // Set global main loop. Not using a global variable to ensure the main loop
  // object lives longer than any other objects, including the static objects.
  ggadget::SetGlobalMainLoop(new ggadget::gtk::MainLoop());

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
  if (argc > 1) {
    g_argument_parser.Start();
    if (!g_argument_parser.AppendArguments(argc - 1, argv + 1) ||
        !g_argument_parser.Finish()) {
      printf("Invalid arguments.\n%s", kHelpString);
      return 1;
    }
  }

  // Check --help argument first.
  if (g_argument_parser.GetArgumentValue(ARG_HELP, NULL)) {
    printf("%s", kHelpString);
    return 0;
  }

  // If another instance is already running, then send all arguments to it.
  if (run_once.IsRunning()) {
    gdk_notify_startup_complete();
    DLOG("Another instance already exists.");
    run_once.SendMessage(ggadget::HostArgumentParser::kStartSignature);
    g_argument_parser.EnumerateRecognizedArgs(
        NewSlot(SendArgumentCallback, &run_once));
    g_argument_parser.EnumerateRemainedArgs(
        NewSlot(SendPathCallback, &run_once));
    run_once.SendMessage(ggadget::HostArgumentParser::kFinishSignature);
    exit(0);
  }

  ExtractArgumentsValue();

  ggadget::SetupLogger(g_arguments.log_level, g_arguments.long_log);

  // Puth the process into background in the early stage to prevent from
  // printing any log messages.
  if (g_arguments.background)
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

  if (!g_arguments.no_collector) {
    ggadget::UsageCollectorFactoryInterface *collector_factory =
        ggadget::GetUsageCollectorFactory();
    if (collector_factory) {
      collector_factory->SetApplicationInfo(GGL_APP_NAME, GGL_VERSION);
      // Only take the initial screen size.
      // We don't really want very accurate stats.
      GdkScreen *screen = NULL;
      gdk_display_get_pointer(gdk_display_get_default(), &screen,
                              NULL, NULL, NULL);
      std::string screen_size_param =
          ggadget::StringPrintf("%dx%d", gdk_screen_get_width(screen),
                                gdk_screen_get_height(screen));
      collector_factory->SetParameter(
          ggadget::UsageCollectorFactoryInterface::PARAM_SCREEN_SIZE,
          screen_size_param.c_str());
    }
  }

  // Only init managed host if it's not standalone mode.
  if (!g_arguments.standalone)
    GetManagedHost();

  // Load gadget files.
  g_argument_parser.EnumerateRemainedArgs(ggadget::NewSlot(LoadLocalGadget));

  // Hook popular signals to exit gracefully.
  signal(SIGHUP, DefaultSignalHandler);
  signal(SIGINT, DefaultSignalHandler);
  signal(SIGTERM, DefaultSignalHandler);
  signal(SIGUSR1, DefaultSignalHandler);
  signal(SIGUSR2, DefaultSignalHandler);

  gdk_notify_startup_complete();

  // Only starts main loop if there is at least one live host.
  if (g_live_host_count > 0)
    gtk_main();

  return 0;
}
