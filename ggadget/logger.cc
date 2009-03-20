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


#include "logger.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <vector>
#include "main_loop_interface.h"
#include "signals.h"
#include "string_utils.h"

namespace ggadget {

typedef Signal4<std::string, LogLevel, const char *, int,
                const std::string &> LogSignal;
typedef LightMap<void *, LogSignal *> ContextSignalMap;

static bool g_log_destroyed = false;
struct LogGlobalData {
  ~LogGlobalData() { g_log_destroyed = true; }
  LogSignal global_signal;
  ContextSignalMap context_signals;
  std::vector<void *> context_stack;
};
static LogGlobalData g_log;

LogHelper::LogHelper(LogLevel level, const char *file, int line)
    : level_(level), file_(file), line_(line) {
}

static void DoLog(LogLevel level, const char *file, int line,
                  const std::string &message) {
  static bool in_logger = false; // Prevents re-entrance.
  if (in_logger) return;

  in_logger = true;
  if (!g_log_destroyed) {
    std::string new_message;
    void *context = g_log.context_stack.empty() ?
                    NULL : g_log.context_stack.back();
    ContextSignalMap::const_iterator it = g_log.context_signals.find(context);
    if (it != g_log.context_signals.end())
      new_message = (*it->second)(level, file, line, message);
    else
      new_message = message;

    if (g_log.global_signal.HasActiveConnections()) {
      g_log.global_signal(level, file, line, new_message);
    } else {
      printf("%s:%d: %s\n", file, line, new_message.c_str());
    }
  } else {
    printf("%s:%d: %s\n", file, line, message.c_str());
  }
  // Prints out the log message immediately.
  fflush(stdout);
  in_logger = false;
}

// Run in the main thread if LoggerHelper is called in another thread.
class LogTask : public WatchCallbackInterface {
 public:
  LogTask(LogLevel level, const char *file, int line,
          const std::string &message)
      : level_(level), file_(file), line_(line), message_(message) {
  }
  virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
    DoLog(level_, VariantValue<const char *>()(file_), line_, message_);
    return false;
  }
  virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
    delete this;
  }

  LogLevel level_;
  // Variant can correctly handle NULL char *, and use decoupled memory space.
  Variant file_;
  int line_;
  std::string message_;
};

void LogHelper::operator()(const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  std::string message = StringVPrintf(format, ap);
  va_end(ap);

  MainLoopInterface *main_loop = GetGlobalMainLoop();
  if (g_log_destroyed || !main_loop || main_loop->IsMainThread()) {
    DoLog(level_, file_, line_, message);
  } else {
    main_loop->AddTimeoutWatch(0, new LogTask(level_, file_, line_, message));
  }
}

ScopedLogContext::ScopedLogContext(void *context) {
  if (!g_log_destroyed)
    g_log.context_stack.push_back(context);
}

ScopedLogContext::~ScopedLogContext() {
  if (!g_log_destroyed)
    g_log.context_stack.pop_back();
}

void PushLogContext(void *context) {
  if (!g_log_destroyed)
    g_log.context_stack.push_back(context);
}

void PopLogContext(void *log_context) {
  if (!g_log_destroyed) {
    ASSERT(log_context == g_log.context_stack.back());
    g_log.context_stack.pop_back();
  }
}

Connection *ConnectGlobalLogListener(LogListener *listener) {
  return g_log_destroyed ? NULL : g_log.global_signal.Connect(listener);
}

Connection *ConnectContextLogListener(void *context, LogListener *listener) {
  if (!g_log_destroyed) {
    ContextSignalMap::const_iterator it = g_log.context_signals.find(context);
    LogSignal *signal;
    if (it == g_log.context_signals.end()) {
      signal = new LogSignal();
      g_log.context_signals[context] = signal;
    } else {
      signal = it->second;
    }
    return signal->Connect(listener);
  }
  return NULL;
}

void RemoveLogContext(void *context) {
  if (!g_log_destroyed) {
    ContextSignalMap::iterator it = g_log.context_signals.find(context);
    if (it != g_log.context_signals.end()) {
      delete it->second;
      g_log.context_signals.erase(it);
    }
  }
}

} // namespace ggadget
