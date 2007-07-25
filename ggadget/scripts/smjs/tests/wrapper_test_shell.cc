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

#include "ggadget/scripts/smjs/native_js_wrapper.h"

#define INIT_CUSTOM_OBJECTS InitCustomObjects
#include "js_shell.cc"

#include "ggadget/tests/scriptables.cc"

using namespace ggadget;

// Called by the initialization code in js.c.
JSBool InitCustomObjects(JSContext *cx, JSObject *obj) {
  TestScriptable1 *test_scriptable1 = new TestScriptable1();
  JSObject *scriptable_obj = NativeJSWrapper::Wrap(cx, test_scriptable1);
  ASSERT(scriptable_obj);
  jsval val = OBJECT_TO_JSVAL(scriptable_obj);
  JS_SetProperty(cx, obj, "scriptable", &val);
  return JS_TRUE;
}
