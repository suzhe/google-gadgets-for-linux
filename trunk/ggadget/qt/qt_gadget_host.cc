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

#include <ggadget/script_runtime_manager.h>
#include <ggadget/element_factory.h>
#include <ggadget/file_manager.h>
#include <ggadget/file_manager_wrapper.h>
#include <ggadget/framework_interface.h>
#include <ggadget/gadget.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/logger.h>
#include <ggadget/script_runtime_interface.h>
#include <ggadget/xml_parser.h>

#include "qt_gadget_host.h"
#include "global_file_manager.h"
#include "qt_view_host.h"
#include "options.h"

namespace ggadget {
namespace qt {

static const char kResourceZipName[] = "ggl-resources.bin";

QtGadgetHost::QtGadgetHost(bool composited, bool useshapemask,
                           double zoom, int debug_mode)
    : resource_file_manager_(new FileManager(GetXMLParser())),
      global_file_manager_(new GlobalFileManager()),
      file_manager_(NULL),
      options_(new Options()),
      gadget_(NULL),
      plugin_flags_(0),
      composited_(composited),
      useshapemask_(useshapemask),
      zoom_(zoom),
      debug_mode_(debug_mode) {
  FileManagerWrapper *wrapper = new FileManagerWrapper(GetXMLParser());
  file_manager_ = wrapper;

  resource_file_manager_->Init(kResourceZipName);
  wrapper->RegisterFileManager(ggadget::kGlobalResourcePrefix,
                               resource_file_manager_);

  global_file_manager_->Init(ggadget::kDirSeparatorStr);
  wrapper->RegisterFileManager(ggadget::kDirSeparatorStr,
                               global_file_manager_);

  ScriptRuntimeManager::get()->ConnectErrorReporter(
      NewSlot(this, &QtGadgetHost::ReportScriptError));

  FcInit(); // Just in case this hasn't been done.
}

QtGadgetHost::~QtGadgetHost() {
  delete gadget_;
  gadget_ = NULL;
  delete options_;
  options_ = NULL;
  delete file_manager_;
  file_manager_ = NULL;
  delete resource_file_manager_;
  resource_file_manager_ = NULL;
  delete global_file_manager_;
  global_file_manager_ = NULL;
}

FileManagerInterface *QtGadgetHost::GetFileManager() {
  return file_manager_;
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
  std::string fontfile;
  if (!file_manager_->ExtractFile(filename, &fontfile)) {
    return false;
  }

  loaded_fonts_[filename] = fontfile;

  FcConfig *config = FcConfigGetCurrent();
  bool success = FcConfigAppFontAddFile(config,
                   reinterpret_cast<const FcChar8 *>(fontfile.c_str()));
  DLOG("LoadFont: %s %s", filename, fontfile.c_str());
  return success;
}

bool QtGadgetHost::UnloadFont(const char *filename) {
  // FontConfig doesn't actually allow dynamic removal of App Fonts, so
  // just remove the file.
  std::map<std::string, std::string>::iterator i = loaded_fonts_.find(filename);
  if (i == loaded_fonts_.end()) {
    return false;
  }

  unlink((i->second).c_str()); // ignore return
  loaded_fonts_.erase(i);

  return true;
}

bool QtGadgetHost::LoadGadget(const char *base_path) {
  gadget_ = new Gadget(this, debug_mode_);
  if (!file_manager_->Init(base_path) || !gadget_->Init()) {
    return false;
  }

  return true;
}

/*bool QtGadgetHost::BrowseForFiles(const char *filter, bool multiple,
                                  std::vector<std::string> *result) {
  ASSERT(result);
  result->clear();

  QStringList filters;
  QFileDialog dialog;
  if (multiple) dialog.setFileMode(QFileDialog::ExistingFiles);
  if (filter && *filter) {
    size_t len = strlen(filter);
    char *copy = static_cast<char*>(malloc(len + 2));
    memcpy(copy, filter, len + 1);
    copy[len] = '|';
    copy[len + 1] = '\0';
    char *str = copy;
    int i = 0;
    int t = 0;
    while (str[i] != '\0') {
      if (str[i] == '|') {
        t++;
        if (t == 1) str[i] = '(';
        if (t == 2) {
          str[i] = ')';
          char bak = str[i + 1];
          str[i + 1] = '\0';
          filters << str;
          str[i + 1] = bak;
          str = &str[i+1];
          i = 0;
          t = 0;
          continue;
        }
      } else if (str[i] == ';' && t == 1) {
        str[i] = ' ';
      }
      i++;
    }
    free(copy);
    dialog.setFilters(filters);
  }
  if (dialog.exec()) {
    QStringList fnames = dialog.selectedFiles();
    for (int i = 0; i < fnames.size(); ++i)
      result->push_back(fnames.at(i).toStdString());
    return true;
  }
  return false;
}
*/

} // namespace qt
} // namespace ggadget
