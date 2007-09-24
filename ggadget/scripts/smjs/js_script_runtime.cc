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

#include "js_script_runtime.h"
#include "js_script_context.h"

namespace ggadget {

const uint32 kDefaultContextSize = 64 * 1024 * 1024;
const uint32 kDefaultStackTrunkSize = 4096;

JSScriptRuntime::JSScriptRuntime()
    : runtime_(JS_NewRuntime(kDefaultContextSize)) {
  // TODO: deal with errors in release build.
  ASSERT(runtime_);
  JS_SetRuntimePrivate(runtime_, this);
}

JSScriptRuntime::~JSScriptRuntime() {
  ASSERT(runtime_);
  JS_DestroyRuntime(runtime_);
}

ScriptContextInterface *JSScriptRuntime::CreateContext() {
  ASSERT(runtime_);
  JSContext *context = JS_NewContext(runtime_, kDefaultStackTrunkSize);
  ASSERT(context);
  if (!context)
    return NULL;
  JS_SetErrorReporter(context, ReportError);
  return new JSScriptContext(context);
}

Connection *JSScriptRuntime::ConnectErrorReporter(ErrorReporter *reporter) {
  return error_reporter_signal_.Connect(reporter);
}

void JSScriptRuntime::ReportError(JSContext *cx, const char *message,
                                  JSErrorReport *report) {
  JSRuntime *js_runtime = JS_GetRuntime(cx);
  ASSERT(js_runtime);
  JSScriptRuntime *runtime = reinterpret_cast<JSScriptRuntime *>
      (JS_GetRuntimePrivate(js_runtime));
  ASSERT(runtime);

  char lineno_buf[16];
  snprintf(lineno_buf, sizeof(lineno_buf), "%d", report->lineno);
  std::string error_report;
  if (report->filename)
    error_report = report->filename;
  error_report += ':';
  error_report += lineno_buf;
  error_report += ": ";
  error_report += message;
  if (!runtime->error_reporter_signal_.HasActiveConnections())
    LOG("No error reporter: %s", error_report.c_str());
  runtime->error_reporter_signal_(error_report.c_str());
}

} // namespace ggadget
