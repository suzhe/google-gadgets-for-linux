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


#include <sys/time.h>
#include <time.h>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <third_party/valgrind/valgrind.h>
#include "logger.h"
#include "string_utils.h"

#ifdef _DEBUG
#define LOG_WITH_TIMESTAMP
#define LOG_WITH_FILE_LINE
#endif

namespace ggadget {

LogHelper::LogHelper(const char *file, int line)
    : file_(file), line_(line) {
}

void LogHelper::operator()(const char *format, ...) {
  if (RUNNING_ON_VALGRIND) {
    va_list ap;
    va_start(ap, format);
    std::string message = StringVPrintf(format, ap);
    va_end(ap);
    VALGRIND_PRINTF_BACKTRACE("%s:%d: %s", file_, line_, message.c_str());
  } else {
    va_list ap;
    va_start(ap, format);
#ifdef LOG_WITH_TIMESTAMP
    struct timeval tv;
    gettimeofday(&tv, NULL);
    printf("%02d:%02d.%03d: ",
           static_cast<int>(tv.tv_sec / 60 % 60),
           static_cast<int>(tv.tv_sec % 60),
           static_cast<int>(tv.tv_usec / 1000));
#endif
#ifdef LOG_WITH_FILE_LINE
    static const char *short_log = getenv(("GGL_SHORT_LOG"));
    if (short_log) {
      const char *name = strrchr(file_, '/');
      printf("%s:%d: ", name ? name+1 : file_, line_);
    } else {
      printf("%s:%d: ", file_, line_);
    }
#endif
    vprintf(format, ap);
    va_end(ap);
    putchar('\n');
    fflush(stdout);
  }
}

} // namespace ggadget
