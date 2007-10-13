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

#include "ggadget/scriptable_helper.h"
#include "ggadget/scriptable_interface.h"
#include "ggadget/scripts/smjs/js_script_context.h"

using namespace ggadget;

class GlobalObject : public ScriptableInterface {
 public:
  DEFINE_CLASS_ID(0x7067c76cc0d84d11, ScriptableInterface);
  GlobalObject() {
  }
  DEFAULT_OWNERSHIP_POLICY
  DELEGATE_SCRIPTABLE_INTERFACE(scriptable_helper_)
  virtual bool IsStrict() const { return false; }
  ScriptableHelper scriptable_helper_;
};

static GlobalObject *global;

// Called by the initialization code in js_shell.cc.
// Used to compile a standalone js_shell.
JSBool InitCustomObjects(JSScriptContext *context) {
  global = new GlobalObject();
  context->SetGlobalObject(global);
  return JS_TRUE;
}

void DestroyCustomObjects(JSScriptContext *context) {
  delete global;
}
