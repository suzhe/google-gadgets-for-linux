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
#include <ggadget/script_runtime_manager.h>
#include "js_script_runtime.h"

#define Initialize smjs_script_runtime_LTX_Initialize
#define Finalize smjs_script_runtime_LTX_Finalize
#define RegisterScriptRuntimeExtension \
    smjs_script_runtime_LTX_RegisterScriptRuntimeExtension

using ggadget::ScriptRuntimeManager;
using ggadget::smjs::JSScriptRuntime;

static JSScriptRuntime *g_smjs_script_runtime_ = NULL;

extern "C" {
  bool Initialize() {
    LOG("Initialize smjs_script_runtime extension.");
    return true;
  }

  void Finalize() {
    LOG("Finalize smjs_script_runtime extension.");
    delete g_smjs_script_runtime_;
  }

  bool RegisterScriptRuntimeExtension(ScriptRuntimeManager *manager) {
    LOG("Register smjs_script_runtime extension.");
    if (manager) {
      if (!g_smjs_script_runtime_)
        g_smjs_script_runtime_ = new JSScriptRuntime();

      manager->RegisterScriptRuntime("js", g_smjs_script_runtime_);
      return true;
    }
    return false;
  }
}
