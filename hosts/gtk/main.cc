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

#include <map>
#include <cstdlib>
#include <gtk/gtk.h>
#include <locale.h>
#include <signal.h>
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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
#include <ggadget/script_runtime_interface.h>
#include <ggadget/script_runtime_manager.h>
#include <ggadget/string_utils.h>
#include <ggadget/system_utils.h>
#include <ggadget/xml_http_request_interface.h>
#include <ggadget/xml_parser_interface.h>
#include "sidebar_gtk_host.h"
#include "simple_gtk_host.h"

static ggadget::gtk::MainLoop g_main_loop;

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


static const char *g_help_string =
  "Google Gadgets for Linux " GGL_VERSION "\n"
  "Usage: %s [Options] [Gadgets]\n"
  "Options:\n"
#ifdef _DEBUG
  "  -d mode    Specify debug modes for drawing View:\n"
  "             0 - No debug.\n"
  "             1 - Draw bounding boxes around container elements.\n"
  "             2 - Draw bounding boxes around all elements.\n"
  "             4 - Draw bounding boxes around clip region.\n"
#endif
  "  -z zoom    Specify initial zoom factor for View, no effect for sidebar.\n"
  "  -b         Draw window border for Main View.\n"
  "  -ns        Use dashboard mode instead of sidebar mode.\n"
  "  -bg        Run in background.\n"
  "  -h, --help Print this message and exit.\n"
  "\n"
  "Gadgets:\n"
  "  Can specify one or more Desktop Gadget paths.\n"
  "  If any gadgets are specified, they will be installed by using\n"
  "  GadgetManager.\n";

int main(int argc, char* argv[]) {
  gtk_init(&argc, &argv);

  int debug_mode = 0;
  double zoom = 1.0;
  bool decorated = false;
  bool sidebar = true;
  bool background = false;

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
    } else {
      gadget_paths.push_back(argv[i]);
    }
  }

  // set locale according to env vars
  setlocale(LC_ALL, "");

  // Puth the process into background in the early stage to prevent from
  // printing any log messages.
  if (background)
    ggadget::Daemonize();

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

  if (!CheckRequiredExtensions())
    return 1;

  // Make the global extension manager readonly to avoid the potential
  // danger that a bad gadget register local extensions into the global
  // extension manager.
  ext_manager->SetReadonly();

  ggadget::HostInterface *host;

  if (sidebar)
    host = new hosts::gtk::SideBarGtkHost(decorated, debug_mode);
  else
    host = new hosts::gtk::SimpleGtkHost(zoom, decorated, debug_mode);

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

  return 0;
}
