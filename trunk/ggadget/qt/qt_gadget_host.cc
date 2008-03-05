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

#include <QFileDialog>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fontconfig/fontconfig.h>

#include <ggadget/element_factory.h>
#include <ggadget/gadget.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/logger.h>
#include <ggadget/options_interface.h>
#include <ggadget/script_runtime_manager.h>
#include <ggadget/xml_parser_interface.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/file_manager_interface.h>

#include "qt_gadget_host.h"
#include "qt_view_host.h"

namespace ggadget {
namespace qt {

static const char kRSSGadgetName[] = "rss_gadget.gg";
static const char kRSSURLOption[] = "RSS_URL";

QtGadgetHost::QtGadgetHost(bool composited, bool useshapemask,
                           double zoom, int debug_mode)
    : options_(CreateOptions("")),
      gadget_(NULL),
      plugin_flags_(0),
      composited_(composited),
      useshapemask_(useshapemask),
      zoom_(zoom),
      debug_mode_(debug_mode) {
  ScriptRuntimeManager::get()->ConnectErrorReporter(
      NewSlot(this, &QtGadgetHost::ReportScriptError));
  FcInit(); // Just in case this hasn't been done.
}

QtGadgetHost::~QtGadgetHost() {
  delete gadget_;
  gadget_ = NULL;
  delete options_;
  options_ = NULL;
}

OptionsInterface *QtGadgetHost::GetOptions() {
  return options_;
}

GadgetInterface *QtGadgetHost::GetGadget() {
  return gadget_;
}

ViewHostInterface *QtGadgetHost::NewViewHost(ViewType type,
                                              ViewInterface *view) {
  return new QtViewHost(this, type, view, composited_, useshapemask_, zoom_);
}

void QtGadgetHost::SetPluginFlags(int plugin_flags) {
}

void QtGadgetHost::RemoveMe(bool save_data) {
}

void QtGadgetHost::DebugOutput(DebugLevel level, const char *message) const {
  const char *str_level = "";
  switch (level) {
    case DEBUG_TRACE: str_level = "TRACE: "; break;
    case DEBUG_WARNING: str_level = "WARNING: "; break;
    case DEBUG_ERROR: str_level = "ERROR: "; break;
    default: break;
  }
  // TODO: actual debug console.
  printf("%s%s\n", str_level, message);
}

/**
 * Taken from GDLinux.
 * May move this function elsewhere if other classes use it too.
 */
std::string GetFullPathOfSysCommand(const std::string &command) {
  const char *env_path_value = getenv("PATH");
  if (env_path_value == NULL)
    return "";

  std::string all_path = std::string(env_path_value);
  size_t cur_colon_pos = 0;
  size_t next_colon_pos = 0;
  // Iterator through all the parts in env value.
  while ((next_colon_pos = all_path.find(":", cur_colon_pos)) != std::string::npos) {
    std::string path = all_path.substr(cur_colon_pos, next_colon_pos - cur_colon_pos);
    path += "/";
    path += command;
    if (access(path.c_str(), X_OK) == 0) {
      return path;
    }
    cur_colon_pos = next_colon_pos + 1;
  }
  return "";
}

bool QtGadgetHost::OpenURL(const char *url) const {
  std::string xdg_open = GetFullPathOfSysCommand("xdg-open");
  if (xdg_open.empty()) {
    xdg_open = GetFullPathOfSysCommand("gnome-open");
    if (xdg_open.empty()) {
      LOG("Couldn't find xdg-open or gnome-open.");
      return false;
    }
  }

  DLOG("Launching URL: %s", url);

  pid_t pid;
  // fork and run the command.
  if ((pid = fork()) == 0) {
    if (fork() != 0)
      _exit(0);

    // Restore original LD_LIBRARY_PATH, to prevent external applications
    // from loading our private libraries.
    //char *ld_path_old = getenv("LD_LIBRARY_PATH_GDL_BACKUP");

    //if (ld_path_old)
    //  setenv("LD_LIBRARY_PATH", ld_path_old, 1);
    //else
    //  unsetenv("LD_LIBRARY_PATH");

    execl(xdg_open.c_str(), xdg_open.c_str(), url, NULL);

    DLOG("Failed to exec command: %s", xdg_open.c_str());
    _exit(-1);
  }

  int status = 0;
  waitpid(pid, &status, 0);

  // Assume xdg-open will always succeed.
  return true;
}

void QtGadgetHost::ReportScriptError(const char *message) {
  DebugOutput(DEBUG_ERROR, (std::string("Script error: " ) + message).c_str());
}

bool QtGadgetHost::LoadFont(const char *filename) {
  FcConfig *config = FcConfigGetCurrent();
  bool success = FcConfigAppFontAddFile(config,
                   reinterpret_cast<const FcChar8 *>(filename));
  DLOG("LoadFont: %s %s", filename, success ? "success" : "fail");
  return success;
}

bool QtGadgetHost::LoadGadget(const char *base_path) {
  bool is_url = IsValidRSSURL(base_path);   
  if (is_url) {
    // Seed options with URL.
    std::string json_url = "\"";
    json_url += base_path;
    json_url += "\"";
    Variant url = Variant(JSONString(json_url)); // raw objects

    // Use putValue instead of putDefaultValue since gadget may set its own
    // default. Gadget can check if it has been initialized by host by checking
    // exists().
    options_->PutValue(kRSSURLOption, url); 
  }

  gadget_ = new Gadget(this, is_url ? kRSSGadgetName : base_path, debug_mode_);
  if (!gadget_->Init()) {
    return false;
  }

  return true;
}

} // namespace qt
} // namespace ggadget
