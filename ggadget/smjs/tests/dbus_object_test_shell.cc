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
#include <ggadget/scriptable_helper.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/smjs/js_script_context.h>
#include <ggadget/extension_manager.h>
#include <ggadget/native_main_loop.h>

using namespace ggadget;
using namespace ggadget::smjs;

class GlobalObject : public ScriptableHelper<ScriptableInterface> {
 public:
  DEFINE_CLASS_ID(0x7067c76cc0d84d11, ScriptableInterface);
  GlobalObject() {
  }
  virtual bool IsStrict() const { return false; }
};

static GlobalObject *global;
static ExtensionManager *ext_manager;
static NativeMainLoop main_loop;

// Called by the initialization code in js_shell.cc.
// Used to compile a standalone js_shell.
JSBool InitCustomObjects(JSScriptContext *context) {
  global = new GlobalObject();
  context->SetGlobalObject(global);
  ext_manager = ExtensionManager::CreateExtensionManager(&main_loop);

  if (!ext_manager->LoadExtension("dbus_script_class", false)) {
    LOG("Failed to load dbus_script_class extension.");
    return JS_FALSE;
  }

  ext_manager->RegisterLoadedExtensions(NULL, context);
  return JS_TRUE;
}

void DestroyCustomObjects(JSScriptContext *context) {
  delete global;
  ext_manager->Destroy();
}
