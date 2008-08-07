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

#include "open_url.h"
#include <string>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ggadget/common.h>
#include <ggadget/logger.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/string_utils.h>
#include <ggadget/gadget.h>
#include <ggadget/permissions.h>

namespace ggadget {
namespace framework {
namespace linux_system {

// Characters need escaping for file path.
static const char kFilePathSpecialCharacters[] = {
  '|', '&', ';', '(', ')', '<', '>', '*',
  '?', '$', '{', '}', ',', '`', '\'', '\"',
  '\\', '#', ' ', '!', '\t',
  '\0'  // This should always be the last char
};

static bool IsFilePathSpecialChar(const char c) {
  for (const char *p = kFilePathSpecialCharacters; *p != '\0'; p++)
    if (*p == c)
      return true;

  return false;
}

static std::string EncodeFilePath(const char *filename) {
  std::string result;
  for (const char *p = filename; p && *p; ++p) {
    if (IsFilePathSpecialChar(*p))
      result.push_back('\\');
    result.push_back(*p);
  }
  return result;
}

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

enum WmType {
  WM_UNKNOWN,
  WM_KDE,
  WM_GNOME,
  WM_XFCE4
};

// Determines current WM is KDE or GNOME according to the env variables.
// Copied from xdg-open, a standard script provided by freedesktop.org.
static WmType DetermineWindowManager() {
  char *value;
  value = getenv("KDE_FULL_SESSION");

  if (value && strcmp (value, "true") == 0)
    return WM_KDE;

  value = getenv("GNOME_DESKTOP_SESSION_ID");

  if (value && *value)
    return WM_GNOME;

  int ret;

  ret = system("xprop -root _DT_SAVE_MODE | grep ' = \"xfce4\"$' > /dev/null 2>&1");
  if (WIFEXITED(ret) && WEXITSTATUS(ret) == 0)
    return WM_XFCE4;

  return WM_UNKNOWN;
}

static bool OpenURLWithSystemCommand(const char *url) {
  char *argv[4] = { NULL, NULL, NULL, NULL };

  // xdg-open is our first choice, if it's not available, then use window
  // manager specific commands.
  std::string open_command = GetFullPathOfSysCommand("xdg-open");
  if (open_command.length()) {
    argv[0] = strdup(open_command.c_str());
    argv[1] = strdup(url);
  } else {
    WmType wm_type = DetermineWindowManager();
    if (wm_type == WM_GNOME) {
      open_command = GetFullPathOfSysCommand("gnome-open");
      if (open_command.length()) {
        argv[0] = strdup(open_command.c_str());
        argv[1] = strdup(url);
      }
    } else if (wm_type == WM_KDE) {
      open_command = GetFullPathOfSysCommand("kfmclient");
      if (open_command.length()) {
        argv[0] = strdup(open_command.c_str());
        argv[1] = strdup("exec");
        argv[2] = strdup(url);
      }
    } else if (wm_type == WM_XFCE4) {
      open_command = GetFullPathOfSysCommand("exo-open");
      if (open_command.length()) {
        argv[0] = strdup(open_command.c_str());
        argv[1] = strdup(url);
      }
    }
  }

  if (argv[0] == NULL) {
    LOG("Can't find a suitable command to open the url.\n"
        "You probably need to install xdg-utils package.");
    return false;
  }

  pid_t pid;
  // fork and run the command.
  if ((pid = fork()) == 0) {
    if (fork() != 0)
      _exit(0);

    execv(argv[0], argv);

    DLOG("Failed to exec command: %s", argv[0]);
    _exit(-1);
  }

  int status = 0;
  waitpid(pid, &status, 0);

  for (size_t i = 0; argv[i]; ++i)
    free(argv[i]);

  // Assume open command will always succeed.
  return true;
}

bool OpenURL(const char *url, const Gadget *gadget) {
  ASSERT(gadget);
  if (!url || !*url) {
    LOG("Invalid URL!");
    return false;
  }

  if (!gadget->IsInUserInteraction()) {
    LOG("framework.openUrl() can only be invoked by user interaction.");
    return false;
  }

  const Permissions *permissions = gadget->GetPermissions();
  if (strncasecmp(url, kHttpUrlPrefix, arraysize(kHttpUrlPrefix) - 1) == 0 ||
      strncasecmp(url, kHttpsUrlPrefix, arraysize(kHttpsUrlPrefix) - 1) == 0) {
    if (!permissions->IsRequiredAndGranted(Permissions::NETWORK)) {
      LOG("No permission to open a remote url.");
      return false;
    }
    // For remote url, only supports http:// and https://
    std::string new_url = EncodeURL(url);
    if (IsValidURL(new_url.c_str()))
      return OpenURLWithSystemCommand(new_url.c_str());
    LOG("Malformed URL: %s", new_url.c_str());
  } else if (strncasecmp(url, kFileUrlPrefix,
                         arraysize(kFileUrlPrefix) - 1) == 0) {
    if (!permissions->IsRequiredAndGranted(Permissions::ALL_ACCESS)) {
      LOG("No permission to open a local file.");
      return false;
    }
    // TODO: Handle desktop file and executable file.
    std::string new_url = EncodeFilePath(url);
    return OpenURLWithSystemCommand(new_url.c_str());
  } else if (strstr(url, "://") == NULL) {
    // URI without prefix, will be treated as http://
    std::string new_url(kHttpUrlPrefix);
    new_url.append(url);
    return OpenURL(new_url.c_str(), gadget);
  } else {
    LOG("Unsupported URL format: %s", url);
  }
  return false;
}

} // namespace linux_system
} // namespace framework
} // ggadget
