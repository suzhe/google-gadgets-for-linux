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

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string>
#include <fontconfig/fontconfig.h>
#include <ggadget/common.h>
#include <ggadget/logger.h>
#include <ggadget/gadget.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/string_utils.h>
#include <ggadget/file_manager_interface.h>
#include <ggadget/view_interface.h>
#include "utilities.h"

namespace ggadget {
namespace qt {


void ShowGadgetAboutDialog(Gadget *gadget) {
}

struct CursorTypeMapping {
  int type;
  Qt::CursorShape qt_type;
};

static const CursorTypeMapping kCursorTypeMappings[] = {
  { ViewInterface::CURSOR_ARROW, Qt::ArrowCursor }, // FIXME
  { ViewInterface::CURSOR_IBEAM, Qt::IBeamCursor },
  { ViewInterface::CURSOR_WAIT, Qt::WaitCursor },
  { ViewInterface::CURSOR_CROSS, Qt::CrossCursor },
  { ViewInterface::CURSOR_UPARROW, Qt::UpArrowCursor },
  { ViewInterface::CURSOR_SIZE, Qt::SizeAllCursor },
  { ViewInterface::CURSOR_SIZENWSE, Qt::SizeFDiagCursor },
  { ViewInterface::CURSOR_SIZENESW, Qt::SizeBDiagCursor },
  { ViewInterface::CURSOR_SIZEWE, Qt::SizeHorCursor },
  { ViewInterface::CURSOR_SIZENS, Qt::SizeVerCursor },
  { ViewInterface::CURSOR_SIZEALL, Qt::SizeAllCursor },
  { ViewInterface::CURSOR_NO, Qt::ForbiddenCursor },
  { ViewInterface::CURSOR_HAND, Qt::OpenHandCursor },
  { ViewInterface::CURSOR_BUSY, Qt::BusyCursor }, // FIXME
  { ViewInterface::CURSOR_HELP, Qt::WhatsThisCursor }
};

Qt::CursorShape GetQtCursorShape(int type) {
  for (size_t i = 0; i < arraysize(kCursorTypeMappings); ++i) {
    if (kCursorTypeMappings[i].type == type)
      return kCursorTypeMappings[i].qt_type;
  }
  return Qt::ArrowCursor;
}
/**
 * Taken from GDLinux.
 * May move this function elsewhere if other classes use it too.
 */
#ifdef GGL_HOST_LINUX
static std::string GetFullPathOfSysCommand(const std::string &command) {
  const char *env_path_value = getenv("PATH");
  if (env_path_value == NULL)
    return "";

  std::string all_path = std::string(env_path_value);
  size_t cur_colon_pos = 0;
  size_t next_colon_pos = 0;
  // Iterator through all the parts in env value.
  while ((next_colon_pos = all_path.find(":", cur_colon_pos)) !=
         std::string::npos) {
    std::string path =
        all_path.substr(cur_colon_pos, next_colon_pos - cur_colon_pos);
    path += "/";
    path += command;
    if (access(path.c_str(), X_OK) == 0) {
      return path;
    }
    cur_colon_pos = next_colon_pos + 1;
  }
  return "";
}
#endif

bool OpenURL(const char *url) {
#ifdef GGL_HOST_LINUX
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

    execl(xdg_open.c_str(), xdg_open.c_str(), url, NULL);

    DLOG("Failed to exec command: %s", xdg_open.c_str());
    _exit(-1);
  }

  int status = 0;
  waitpid(pid, &status, 0);

  // Assume xdg-open will always succeed.
  return true;
#else
  LOG("Don't know how to open an url.");
  return false;
#endif
}

} // namespace qt
} // namespace ggadget
