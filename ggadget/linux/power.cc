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

#include "power.h"

#include "hal_strings.h"
#include <ggadget/dbus/dbus_proxy.h>
#include <ggadget/logger.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/slot.h>

using ggadget::dbus::DBusProxy;
using ggadget::dbus::DBusProxyFactory;
using ggadget::dbus::MESSAGE_TYPE_BOOLEAN;
using ggadget::dbus::MESSAGE_TYPE_STRING;
using ggadget::dbus::MESSAGE_TYPE_INVALID;

namespace ggadget {
namespace framework {
namespace linux_os {

class Power::Impl {
 public:
  Impl() : factory_(NULL), proxy_(NULL) {
    DBusProxy *proxy = factory_.NewSystemProxy(kHalDBusName,
                                               kHalManagerPath,
                                               kHalManagerInterface,
                                               false);
    if (!proxy->Call(kHalFindDeviceMethod, true, -1,
                     NewSlot(this, &Impl::ReceiveStringList),
                     MESSAGE_TYPE_STRING, "battery", MESSAGE_TYPE_INVALID)) {
      DLOG("Get battery devices failed.");
    }
    delete proxy;
    if (!strlist_result_.empty()) {
      proxy_ = factory_.NewSystemProxy(kHalDBusName,
                                       strlist_result_[0].c_str(),
                                       kHalManagerInterface,
                                       false);
    }
  }
  ~Impl() {
    delete proxy_;
  }
  bool IsCharging() {
    if (!proxy_) return false;
    proxy_->Call(kHalPropertyMethod, true, -1,
                 NewSlot(this, &Impl::ReceiveBoolValue),
                 MESSAGE_TYPE_STRING, kHalChargingProperty,
                 MESSAGE_TYPE_INVALID);
    return boolean_result_;
  }
  bool IsPluggedIn() {
    if (!proxy_) return true;
    return GetPercentRemaining() == 100 || IsCharging();
  }
  int GetPercentRemaining() {
    if (!proxy_) return 0;
    int_resutl_ = 0;
    proxy_->Call(kHalPropertyMethod, true, -1,
                 NewSlot(this, &Impl::ReceiveIntValue),
                 MESSAGE_TYPE_STRING, kHalPercentageProperty,
                 MESSAGE_TYPE_INVALID);
    return int_resutl_;
  }
  int GetTimeRemaining() {
    if (!proxy_) return 0;
    int_resutl_ = 0;
    proxy_->Call(kHalPropertyMethod, true, -1,
                 NewSlot(this, &Impl::ReceiveIntValue),
                 MESSAGE_TYPE_STRING, kHalRemainingProperty,
                 MESSAGE_TYPE_INVALID);
    return int_resutl_;
  }
  int GetTimeTotal() {
    if (!proxy_) return 0;
    int_resutl_ = 0;
    proxy_->Call(kHalPropertyMethod, true, -1,
                 NewSlot(this, &Impl::ReceiveIntValue),
                 MESSAGE_TYPE_STRING, kHalTotalTimeProperty,
                 MESSAGE_TYPE_INVALID);
    return int_resutl_;
  }
 private:
  bool ReceiveIntValue(int id, const Variant &value) {
    if (value.type() != Variant::TYPE_INT64) return false;
    int_resutl_ = VariantValue<int>()(value);
    return false;
  }
  bool ReceiveBoolValue(int id, const Variant &value) {
    boolean_result_ = false;
    if (value.type() != Variant::TYPE_BOOL) return false;
    boolean_result_ = VariantValue<bool>()(value);
    return false;
  }
  bool ReceiveStringList(int id, const Variant &value) {
    if (id > 0 || value.type() != Variant::TYPE_SCRIPTABLE) return false;
    strlist_result_.clear();
    ScriptableInterface *array = VariantValue<ScriptableInterface*>()(value);
    if (array)
      return array->EnumerateElements(NewSlot(this,
                                              &Impl::StringListEnumerator));
    return false;
  }
  bool StringListEnumerator(int id, const Variant &value) {
    std::string str;
    if (!value.ConvertToString(&str)) {
      DLOG("the element in the array is not a string, it is: %s",
           value.Print().c_str());
      return false;
    }
    strlist_result_.push_back(str);
    return true;
  }
  DBusProxyFactory factory_;
  DBusProxy *proxy_;

  std::vector<std::string> strlist_result_;
  bool boolean_result_;
  int int_resutl_;
};

Power::Power() : impl_(new Impl) {
}

Power::~Power() {
  delete impl_;
  impl_ = NULL;
}

bool Power::IsCharging() const {
  return impl_->IsCharging();
}

bool Power::IsPluggedIn() const {
  return impl_->IsPluggedIn();
}

int Power::GetPercentRemaining() const {
  return impl_->GetPercentRemaining();
}

int Power::GetTimeRemaining() const {
  return impl_->GetTimeRemaining();
}

int Power::GetTimeTotal() const {
  return impl_->GetTimeTotal();
}

} // namespace linux_os
} // namespace framework
} // namespace ggadget
