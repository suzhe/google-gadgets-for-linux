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

#include <ggadget/logger.h>
#include <ggadget/dbus/dbus_proxy.h>
#include "scriptable_dbus_object.h"

namespace ggadget {
namespace dbus {

class ScriptableDBusObject::Impl {
 public:
  Impl(ScriptableDBusObject *owner, DBusProxy* proxy)
      : register_(owner), proxy_(proxy) {
    proxy->EnumerateMethods(NewSlot(&register_, &Register::Callback));
  }
  ~Impl() {
    delete proxy_;
  }
 private:
  class Register {
   public:
    Register(ScriptableDBusObject *owner) : owner_(owner) {}
    ~Register() {}
    bool Callback(const char *name, Slot *slot) {
      method_names_.push_back(name);
      DLOG("register method call: %s", name);
      owner_->RegisterMethod((method_names_.end() - 1)->c_str(), slot);
      return true;
    }
   private:
    std::vector<std::string> method_names_;
    ScriptableDBusObject *owner_;
  } register_;
  DBusProxy *proxy_;
};

ScriptableDBusObject::ScriptableDBusObject(DBusProxy *proxy)
  : impl_(new Impl(this, proxy)) {
}

ScriptableDBusObject::~ScriptableDBusObject() {
  delete impl_;
  impl_ = NULL;
}

}  // namespace dbus
}  // namespace ggadget
