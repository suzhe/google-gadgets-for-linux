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

#ifndef GGADGET_SMJS_JS_SCRIPT_RUNTIME_H__
#define GGADGET_SMJS_JS_SCRIPT_RUNTIME_H__

#include <jsapi.h>
#include <ggadget/script_runtime_interface.h>
#include <ggadget/signals.h>

namespace ggadget {
namespace smjs {

/**
 * @c ScriptRuntime implementation for SpiderMonkey JavaScript engine.
 */
class JSScriptRuntime : public ScriptRuntimeInterface {
 public:
  JSScriptRuntime();
  virtual ~JSScriptRuntime();

  /** @see ScriptRuntimeInterface::CreateContext() */
  virtual ScriptContextInterface *CreateContext();
  /** @see ScriptRuntimeInterface::ConnectErrorReporter() */
  virtual Connection *ConnectErrorReporter(ErrorReporter *reporter);
 private:
  DISALLOW_EVIL_CONSTRUCTORS(JSScriptRuntime);

  static void ReportError(JSContext *cx, const char *message,
                          JSErrorReport *report);

  Signal1<void, const char *> error_reporter_signal_;
  JSRuntime *runtime_;
};

} // namespace smjs
} // namespace ggadget

#endif  // GGADGET_SMJS_JS_SCRIPT_RUNTIME_H__
