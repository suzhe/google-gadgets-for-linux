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
#include <ggadget/main_loop_interface.h>
#include <ggadget/element_factory.h>
#include <ggadget/script_context_interface.h>
#include <ggadget/dbus/dbus_proxy.h>
#include <ggadget/variant.h>
#include <ggadget/slot.h>
#include "scriptable_dbus_object.h"

#define Initialize dbus_script_class_LTX_Initialize
#define Finalize dbus_script_class_LTX_Finalize
#define RegisterExtension dbus_script_class_LTX_RegisterExtension

using ggadget::dbus::ScriptableDBusObject;
using ggadget::dbus::DBusProxyFactory;
using ggadget::dbus::DBusProxy;
using ggadget::Variant;
using ggadget::NewSlot;
using ggadget::NewSlotWithDefaultArgs;

static const char *kDBusSystemObjectName = "DBusSystemObject";
static const char *kDBusSessionObjectName = "DBusSessionObject";

static const Variant kDefaultArgs[] = {
  Variant(), // name
  Variant(), // path
  Variant(), // interface
  Variant(false) // only bind to current owner
};

static DBusProxyFactory *ggl_dbus_factory = NULL;

static ScriptableDBusObject* NewSystemObject(const char *name,
                                             const char *path,
                                             const char *interface,
                                             bool bind_object) {
  DBusProxy *proxy =
      ggl_dbus_factory->NewSystemProxy(name, path, interface, bind_object);
  return new ScriptableDBusObject(proxy);
}

static ScriptableDBusObject* NewSessionObject(const char *name,
                                              const char *path,
                                              const char *interface,
                                              bool bind_object) {
  DBusProxy *proxy =
      ggl_dbus_factory->NewSessionProxy(name, path, interface, bind_object);
  return new ScriptableDBusObject(proxy);
}

extern "C" {
  bool Initialize(ggadget::MainLoopInterface *main_loop) {
    LOG("Initialize dbus_script_class extension.");
    if (!ggl_dbus_factory)
      ggl_dbus_factory = new DBusProxyFactory(main_loop);
    return true;
  }

  void Finalize() {
    LOG("Finalize dbus_script_class extension.");
    delete ggl_dbus_factory;
    ggl_dbus_factory = NULL;
  }

  bool RegisterExtension(ggadget::ElementFactory *factory,
                         ggadget::ScriptContextInterface *context) {
    LOG("Register dbus_script_class extension.");
    if (context) {
      if (!context->RegisterClass(
          kDBusSystemObjectName, NewSlotWithDefaultArgs(
              NewSlot(NewSystemObject), kDefaultArgs))) {
        LOG("Failed to register %s class.", kDBusSystemObjectName);
      }
      if (!context->RegisterClass(
          kDBusSessionObjectName, NewSlotWithDefaultArgs(
              NewSlot(NewSessionObject), kDefaultArgs))) {
        LOG("Failed to register %s class.", kDBusSessionObjectName);
      }
    }
    return true;
  }
}
