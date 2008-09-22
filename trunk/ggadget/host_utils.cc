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

#include "host_utils.h"

#include <sys/time.h>
#include <ctime>

#include "dir_file_manager.h"
#include "file_manager_factory.h"
#include "file_manager_wrapper.h"
#include "gadget_consts.h"
#include "gadget_manager_interface.h"
#include "locales.h"
#include "localized_file_manager.h"
#include "logger.h"
#include "main_loop_interface.h"
#include "messages.h"
#include "options_interface.h"
#include "script_runtime_interface.h"
#include "script_runtime_manager.h"
#include "slot.h"
#include "xml_http_request_interface.h"
#include "xml_parser_interface.h"

namespace ggadget {

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

bool SetupGlobalFileManager(const char *profile_dir) {
  FileManagerWrapper *fm_wrapper = new FileManagerWrapper();
  FileManagerInterface *fm;

  for (size_t i = 0; kGlobalResourcePaths[i]; ++i) {
    fm = CreateFileManager(kGlobalResourcePaths[i]);
    if (fm) {
      fm_wrapper->RegisterFileManager(kGlobalResourcePrefix,
                                      new LocalizedFileManager(fm));
      break;
    }
  }
  if ((fm = CreateFileManager(kDirSeparatorStr)) != NULL) {
    fm_wrapper->RegisterFileManager(kDirSeparatorStr, fm);
  }
#ifdef _DEBUG
  std::string dot_slash(".");
  dot_slash += kDirSeparatorStr;
  if ((fm = CreateFileManager(dot_slash.c_str())) != NULL) {
    fm_wrapper->RegisterFileManager(dot_slash.c_str(), fm);
  }
#endif

  fm = DirFileManager::Create(profile_dir, true);
  if (fm != NULL) {
    fm_wrapper->RegisterFileManager(kProfilePrefix, fm);
  } else {
    LOG("Failed to initialize profile directory.");
  }

  SetGlobalFileManager(fm_wrapper);
  return true;
}

static int g_log_level;
static bool g_long_log;

static std::string DefaultLogListener(LogLevel level,
                               const char *filename,
                               int line,
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

void SetupLogger(int log_level, bool long_log) {
  g_log_level = log_level;
  g_long_log = long_log;
  ConnectGlobalLogListener(NewSlot(DefaultLogListener));
}

bool CheckRequiredExtensions(std::string *message) {
  if (!GetGlobalFileManager()->FileExists(kCommonJS, NULL)) {
    // We can't use localized message here because resource failed to load.
    *message = "Program can't start because it failed to load resources";
    return false;
  }

  if (!GetXMLParser()) {
    // We can't use localized message here because XML parser is not available.
    *message = "Program can't start because it failed to load the "
        "libxml2-xml-parser module.";
    return false;
  }

  *message = "";
  if (!ScriptRuntimeManager::get()->GetScriptRuntime("js"))
    *message += "js-script-runtime\n";
  if (!GetXMLHttpRequestFactory())
    *message += "xml-http-request\n";
  if (!GetGadgetManager())
    *message += "google-gadget-manager\n";

  if (!message->empty()) {
    *message = GMS_("LOAD_EXTENSIONS_FAIL") + "\n\n" + *message;
    return false;
  }
  return true;
}

void InitXHRUserAgent(const char *app_name) {
  XMLHttpRequestFactoryInterface *xhr_factory = GetXMLHttpRequestFactory();
  ASSERT(xhr_factory);
  std::string user_agent = StringPrintf(
      "%s/" GGL_VERSION " (%c%s; %s; ts:" GGL_VERSION_TIMESTAMP
      "; api:" GGL_API_VERSION
  #ifdef GGL_OEM_BRAND
      "; oem:" GGL_OEM_BRAND ")",
  #else
      ")",
  #endif
      app_name, toupper(GGL_PLATFORM[0]), GGL_PLATFORM + 1,
      GetSystemLocaleName().c_str());
  xhr_factory->SetDefaultUserAgent(user_agent.c_str());
}

static int BestPosition(int total, int pos, int size) {
  if (pos + size < total)
    return pos;
  else if (size > total)
    return 0;
  else
    return total - size;
}

void GetPopupPosition(int x, int y, int w, int h,
                      int w1, int h1, int sw, int sh,
                      int *x1, int *y1) {
  int left_gap = x - w1;
  int right_gap = sw - (x + w + w1);
  int top_gap = y - h1;
  int bottom_gap = sh - (y + h + h1);

  // We prefer to popup to right
  if (right_gap >= 0) {
    *x1 = x + w;
    *y1 = BestPosition(sh, y, h1);
  } else if (left_gap > top_gap && left_gap > bottom_gap) {
    *x1 = x - w1;
    *y1 = BestPosition(sh, y, h1);
  } else if (top_gap > bottom_gap) {
    *y1 = y - h1;
    *x1 = BestPosition(sw, x, w1);
  } else {
    *y1 = y + h;
    *x1 = BestPosition(sw, x, w1);
  }
}

} // namespace ggadget
