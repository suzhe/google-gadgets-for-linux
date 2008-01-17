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

#include <ggadget/logger.h>
#include <ggadget/signals.h>
#include "js_script_runtime.h"
#include "js_script_context.h"

namespace ggadget {
namespace smjs {

static const uint32 kDefaultContextSize = 64 * 1024 * 1024;
static const uint32 kDefaultStackTrunkSize = 4096;

class JSScriptRuntime::Impl {
 public:
  Impl()
      : runtime_(JS_NewRuntime(kDefaultContextSize)) {
    JS_SetRuntimePrivate(runtime_, this);
    ASSERT(runtime_);
  }

  ~Impl() {
    JS_DestroyRuntime(runtime_);
  }

  static void ReportError(JSContext *cx, const char *message,
                          JSErrorReport *report) {
    JSRuntime *js_runtime = JS_GetRuntime(cx);
    ASSERT(js_runtime);
    Impl *this_p = reinterpret_cast<Impl *>(JS_GetRuntimePrivate(js_runtime));
    ASSERT(this_p);

    char lineno_buf[16];
    snprintf(lineno_buf, sizeof(lineno_buf), "%d", report->lineno);
    std::string error_report;
    if (report->filename)
      error_report = report->filename;
    error_report += ':';
    error_report += lineno_buf;
    error_report += ": ";
    error_report += message;
    if (!this_p->error_reporter_signal_.HasActiveConnections())
      LOG("No error reporter: %s", error_report.c_str());
    this_p->error_reporter_signal_(error_report.c_str());
  }

  Signal1<void, const char *> error_reporter_signal_;
  JSRuntime *runtime_;
};

JSScriptRuntime::JSScriptRuntime()
    : impl_(new Impl) {
}

JSScriptRuntime::~JSScriptRuntime() {
  delete impl_;
}

ScriptContextInterface *JSScriptRuntime::CreateContext() {
  JSContext *context = JS_NewContext(impl_->runtime_, kDefaultStackTrunkSize);
  ASSERT(context);
  if (!context)
    return NULL;
  JS_SetErrorReporter(context, Impl::ReportError);
  JSScriptContext *result = new JSScriptContext(this, context);
  return result;
}

Connection *JSScriptRuntime::ConnectErrorReporter(ErrorReporter *reporter) {
  printf("ConnectErrorReporter: %p\n", reporter); fflush(stdout);
  return impl_->error_reporter_signal_.Connect(reporter);
}

void JSScriptRuntime::DestroyContext(JSScriptContext *context) {
  delete context;
}

} // namespace smjs
} // namespace ggadget
