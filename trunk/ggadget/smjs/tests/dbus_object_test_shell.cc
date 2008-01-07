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

#include <ggadget/dbus/dbus_proxy.h>
#include <ggadget/dbus/scriptable_dbus_object.h>
#include <ggadget/logger.h>
#include <ggadget/scriptable_helper.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/smjs/js_script_context.h>

using namespace ggadget;
using namespace ggadget::smjs;
using namespace ggadget::dbus;

class GlobalObject : public ScriptableHelper<ScriptableInterface> {
 public:
  DEFINE_CLASS_ID(0x7067c76cc0d84d11, ScriptableInterface);
  GlobalObject() : factory_(NULL) {
  }
  virtual bool IsStrict() const { return false; }

  ScriptableInterface *NewSystemObject(const char *name,
                                       const char *path,
                                       const char *interface) {
    DBusProxy *proxy = factory_.NewSystemProxy(name, path, interface, false);
    return new ScriptableDBusObject(proxy);
  }

  ScriptableInterface *NewSessionObject(const char *name,
                                        const char *path,
                                        const char *interface) {
    DBusProxy *proxy = factory_.NewSessionProxy(name, path, interface, false);
    return new ScriptableDBusObject(proxy);
  }
  DBusProxyFactory factory_;
};

static GlobalObject *global;

// Called by the initialization code in js_shell.cc.
// Used to compile a standalone js_shell.
JSBool InitCustomObjects(JSScriptContext *context) {
  global = new GlobalObject();
  context->SetGlobalObject(global);
  if (!context->RegisterClass("DBusSystemObject",
                              NewSlot(global, &GlobalObject::NewSystemObject)))
    DLOG("Register DBusSystemObject failed.");
  if (!context->RegisterClass("DBusSessionObject",
                              NewSlot(global, &GlobalObject::NewSessionObject)))
    DLOG("Register DBusSessionObject failed.");
  return JS_TRUE;
}

void DestroyCustomObjects(JSScriptContext *context) {
  delete global;
}
