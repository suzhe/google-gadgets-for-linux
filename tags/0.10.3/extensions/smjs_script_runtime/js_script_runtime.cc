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

#include "js_script_runtime.h"

#include <ggadget/logger.h>
#include <ggadget/signals.h>
#include "js_script_context.h"

namespace ggadget {
namespace smjs {

static const uint32 kDefaultContextSize = 32 * 1024 * 1024;
static const uint32 kDefaultStackTrunkSize = 4096;

JSScriptRuntime::JSScriptRuntime()
    : runtime_(JS_NewRuntime(kDefaultContextSize)) {
  JS_SetRuntimePrivate(runtime_, this);
  ASSERT(runtime_);
  // Use the similar policy as Mozilla Gecko that unconstrains the runtime's
  // threshold on nominal heap size, to avoid triggering GC too often.
  JS_SetGCParameter(runtime_, JSGC_MAX_BYTES, 0xffffffff);
}

JSScriptRuntime::~JSScriptRuntime() {
  JS_DestroyRuntime(runtime_);
}

ScriptContextInterface *JSScriptRuntime::CreateContext() {
  JSContext *context = JS_NewContext(runtime_, kDefaultStackTrunkSize);
  ASSERT(context);
  if (!context)
    return NULL;
  JSScriptContext *result = new JSScriptContext(this, context);
  return result;
}

void JSScriptRuntime::DestroyContext(JSScriptContext *context) {
  delete context;
}

} // namespace smjs
} // namespace ggadget
