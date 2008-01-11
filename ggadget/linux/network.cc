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

#include "network.h"
#include "hal_strings.h"
#include <ggadget/dbus/dbus_proxy.h>
#include <ggadget/logger.h>
#include <ggadget/scoped_ptr.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/slot.h>
#include <ggadget/string_utils.h>

using ggadget::dbus::DBusProxy;
using ggadget::dbus::DBusProxyFactory;
using ggadget::dbus::MESSAGE_TYPE_BOOLEAN;
using ggadget::dbus::MESSAGE_TYPE_INVALID;
using ggadget::dbus::MESSAGE_TYPE_STRING;

namespace ggadget {
namespace framework {
namespace linux_os {

class Network::Impl {
 public:
  Impl() : proxy_factory_(NULL), last_active_interface_(-1) {
    GetInterfaces();
  }
  ~Impl() {
  }
  bool IsOnline() {
    int index = GetActiveInterface();
    return index >= 0;
  }
  NetworkInterface::ConnectionType GetConnectionType() {
    int index = GetActiveInterface();
    if (index >= 0) {
      std::string category =
          GetInterfacePropertyString(index, kHalCategoryProperty);
      DLOG("category: %s", category.c_str());
      if (category == kHalNet80203Property)
        return CONNECTION_TYPE_802_3;
      else if (category == kHalNet80211Property)
        return CONNECTION_TYPE_NATIVE_802_11;
      else if (category == kHalNetBlueToothProperty)
        return CONNECTION_TYPE_BLUETOOTH;
      else if (category == kHalNetIrdaProperty)
        return CONNECTION_TYPE_IRDA;
      else
        LOG("the net interface is a unknown type: %s", category.c_str());
    }
    return CONNECTION_TYPE_UNKNOWN;
  }
  NetworkInterface::PhysicalMediaType GetPhysicalMediaType() {
    NetworkInterface::ConnectionType type = GetConnectionType();
    switch (type) {
      case CONNECTION_TYPE_NATIVE_802_11:
        return PHISICAL_MEDIA_TYPE_NATIVE_802_11;
      case CONNECTION_TYPE_BLUETOOTH:
        return PHISICAL_MEDIA_TYPE_BLUETOOTH;
      default:
        return PHISICAL_MEDIA_TYPE_UNSPECIFIED;
    }
    return PHISICAL_MEDIA_TYPE_UNSPECIFIED;
  }
 private:
  int GetActiveInterface() {
    if (last_active_interface_ >= 0) {
      if (IsInterfaceUp(last_active_interface_))
        return last_active_interface_;
      else
        last_active_interface_ = -1;
    }
    for (std::size_t i = 0; i < interfaces_.size(); ++i) {
      if (IsInterfaceUp(i)) {
        last_active_interface_ = i;
        break;
      }
    }
    return last_active_interface_;
  }
  std::string GetInterfacePropertyString(int i, const char *property) {
    DBusProxy *proxy = proxy_factory_.NewSystemProxy(kHalDBusName,
                                                     interfaces_[i].c_str(),
                                                     kHalComputerInterface,
                                                     false);
    string_result_.clear();
    proxy->Call(kHalPropertyMethod, true, -1,
                NewSlot(this, &Impl::ReceiveString),
                MESSAGE_TYPE_STRING, property, MESSAGE_TYPE_INVALID);
    delete proxy;
    return string_result_;
  }
  bool IsInterfaceUp(int i) {
    DBusProxy *proxy = proxy_factory_.NewSystemProxy(kHalDBusName,
                                                     interfaces_[i].c_str(),
                                                     kHalComputerInterface,
                                                     false);
    boolean_result_ = false;
    proxy->Call(kHalPropertyMethod, true, -1,
                NewSlot(this, &Impl::ReceiveBoolean),
                MESSAGE_TYPE_STRING, kHalNetInterfaceOnProperty,
                MESSAGE_TYPE_INVALID);
    delete proxy;
    return boolean_result_;
  }
  void GetInterfaces() {
    DBusProxy *proxy = proxy_factory_.NewSystemProxy(kHalDBusName,
                                                     kHalManagerPath,
                                                     kHalManagerInterface,
                                                     false);
    strlist_result_.clear();
    if (!proxy->Call(kHalFindDeviceMethod, true, -1,
                     NewSlot(this, &Impl::ReceiveStringList),
                     MESSAGE_TYPE_STRING, "net", MESSAGE_TYPE_INVALID)) {
      DLOG("Get devices failed.");
    }
    interfaces_.swap(strlist_result_);
    delete proxy;
  }
  bool ReceiveStringList(int id, const Variant &value) {
    if (id > 0 || value.type() != Variant::TYPE_SCRIPTABLE) return false;
    ScriptableInterface *array = VariantValue<ScriptableInterface*>()(value);
    return array->EnumerateElements(NewSlot(this, &Impl::StringListEnumerator));
  }
  bool ReceiveBoolean(int id, const Variant &value) {
    if (id > 0 || value.type() != Variant::TYPE_BOOL) return false;
    boolean_result_ = VariantValue<bool>()(value);
    return true;
  }
  bool ReceiveString(int id, const Variant &value) {
    if (id > 0 || value.type() != Variant::TYPE_STRING) return false;
    value.ConvertToString(&string_result_);
    return true;
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
  DBusProxyFactory proxy_factory_;
  int last_active_interface_;
  std::vector<std::string> interfaces_;
  std::vector<std::string> strlist_result_;
  bool boolean_result_;
  std::string string_result_;
};

Network::Network() : impl_(new Impl) {
}

Network::~Network() {
  delete impl_;
}

bool Network::IsOnline() const {
  return impl_->IsOnline();
}

NetworkInterface::ConnectionType Network::GetConnectionType() const {
  return impl_->GetConnectionType();
}

NetworkInterface::PhysicalMediaType Network::GetPhysicalMediaType() const {
  return impl_->GetPhysicalMediaType();
}

} // namespace linux_os
} // namespace framework
} // namespace ggadget
