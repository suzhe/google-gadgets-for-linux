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

#include <map>
#include <cstdlib>
#include <gtk/gtk.h>
#include <locale.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ggadget/script_runtime_interface.h>
#include <ggadget/gtk/single_view_host.h>
#include <ggadget/gtk/main_loop.h>
#include <ggadget/gtk/utilities.h>
#include <ggadget/directory_provider_interface.h>
#include <ggadget/extension_manager.h>
#include <ggadget/script_runtime_manager.h>
#include <ggadget/ggadget.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/file_manager_factory.h>
#include <ggadget/file_manager_wrapper.h>
#include <ggadget/localized_file_manager.h>
#include <ggadget/host_interface.h>
#include <ggadget/string_utils.h>
#include <ggadget/logger.h>
#include <ggadget/gadget_manager_interface.h>
#include "simple_gtk_host.h"

//static double g_zoom = 1.;
static int g_debug_mode = 0;
static ggadget::gtk::MainLoop g_main_loop;

static const char *kGlobalExtensions[] = {
// default framework must be loaded first, so that the default properties can
// be overrided.
  "default-framework",
  "libxml2-xml-parser",
  "default-options",
  "dbus-script-class",
  "gtk-edit-element",
  "gtkmoz-browser-element",
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

static const char *kGlobalResourcePaths[] = {
#ifdef GGL_RESOURCE_DIR
  GGL_RESOURCE_DIR "/ggl-resources.gg",
  GGL_RESOURCE_DIR "/ggl-resources",
#endif
  "ggl-resources.gg",
  "ggl-resources",
  NULL
};

class DirectoryProvider : public ggadget::DirectoryProviderInterface {
 public:
  virtual std::string GetProfileDirectory() { return ""; }
  virtual std::string GetResourceDirectory() { return ""; }
};

static DirectoryProvider g_directory_provider;

int main(int argc, char* argv[]) {
  gtk_init(&argc, &argv);

  // set locale according to env vars
  setlocale(LC_ALL, "");

  // Set global main loop
  ggadget::SetGlobalMainLoop(&g_main_loop);
  ggadget::SetDirectoryProvider(&g_directory_provider);

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

  // Make the global extension manager readonly to avoid the potential
  // danger that a bad gadget register local extensions into the global
  // extension manager.
  ext_manager->SetReadonly();

  hosts::gtk::SimpleGtkHost host(g_debug_mode);

  // Load gadget files.
  if (argc >= 2) {
    ggadget::GadgetManagerInterface *manager = ggadget::GetGadgetManager();

    for (int i = 1; i < argc; ++i)
      manager->NewGadgetInstanceFromFile(argv[i]);
  }

  host.Run();

  return 0;
}
