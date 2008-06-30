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

#include "wireless_access_point.h"

#include <time.h>

#include <ggadget/dbus/dbus_proxy.h>
#include <ggadget/dbus/dbus_result_receiver.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/slot.h>
#include <ggadget/variant.h>
#include "hal_strings.h"

// defined in <linux/wireless.h>, but we don't want to introduce such
// dependency.
#define IW_MODE_INFRA 2
#define IW_MODE_ADHOC 1

using ggadget::dbus::DBusProxy;
using ggadget::dbus::DBusProxyFactory;
using ggadget::dbus::DBusBooleanReceiver;
using ggadget::dbus::MESSAGE_TYPE_BOOLEAN;
using ggadget::dbus::MESSAGE_TYPE_INVALID;
using ggadget::dbus::MESSAGE_TYPE_STRING;

namespace ggadget {
namespace framework {
namespace linux_system {

class WirelessAccessPoint::Impl {
 public:
  Impl(const std::string &name)
      : factory_(NULL),
        path_(name),
        type_(WirelessAccessPointInterface::WIRELESS_TYPE_ANY),
        strength_(0),
        last_check_time_in_sec_(0) {
    proxy_ = factory_.NewSystemProxy(kNetworkManagerDBusName,
                                     path_.c_str(),
                                     kNetworkManagerDeviceInterface,
                                     false);
    connect_proxy_ = factory_.NewSystemProxy(kNetworkManagerDBusName,
                                             kNetworkManagerObjectPath,
                                             kNetworkManagerInterface,
                                             false);
  }
  ~Impl() {
    delete proxy_;
    delete connect_proxy_;
  }
  std::string GetName() {
    Refresh();
    return name_;
  }
  WirelessAccessPointInterface::Type GetType() {
    Refresh();
    return type_;
  }
  int GetSignalStrength() {
    Refresh();
    return strength_;
  }
  void Connect(Slot1<void, bool> *callback) {
    DBusBooleanReceiver receiver;
    connect_proxy_->Call(kNetworkManagerMethodGetWireless, true, -1,
                         receiver.NewSlot(), MESSAGE_TYPE_INVALID);
    bool result = false;
    if (!receiver.GetValue()) {
      result = connect_proxy_->Call(kNetworkManagerMethodSetWireless,
                                    true, -1, NULL,
                                    MESSAGE_TYPE_BOOLEAN, true,
                                    MESSAGE_TYPE_INVALID);
    }
    result = connect_proxy_->Call(kNetworkManagerMethodSetActive,
                                  true, -1, NULL,
                                  MESSAGE_TYPE_STRING, path_.c_str(),
                                  MESSAGE_TYPE_STRING, name_.c_str(),
                                  MESSAGE_TYPE_INVALID);
    if (callback) {
      (*callback)(result);
      delete callback;
    }
  }
  void Disconnect(Slot1<void, bool> *callback) {
    bool result = connect_proxy_->Call(kNetworkManagerMethodSetWireless,
                                       true, -1, NULL,
                                       MESSAGE_TYPE_BOOLEAN, false,
                                       MESSAGE_TYPE_INVALID);
    DLOG("Disconnect result: %s", result ? "true" : "false");
    if (callback) {
      (*callback)(result);
      delete callback;
    }
  }
 private:
  void Refresh() {
    time_t now = time(NULL);
    if (now - last_check_time_in_sec_ < kCheckInterval) return;
    proxy_->Call(kNetworkManagerMethodGetProperties, true, -1,
                 NewSlot(this, &Impl::GetInterestedProperties),
                 MESSAGE_TYPE_INVALID);
  }
  bool GetInterestedProperties(int id, const Variant &value) {
    switch (id) {
      case 1:
        if (!value.ConvertToString(&name_)) return false;
        break;
      case 3:
        if (!value.ConvertToInt(&strength_)) return false;
        break;
      case 6:
        {
          int mode = -1;
          if (!value.ConvertToInt(&mode)) return false;
          if (mode == IW_MODE_INFRA)
            type_ = WirelessAccessPointInterface::WIRELESS_TYPE_INFRASTRUCTURE;
          else if (mode == IW_MODE_ADHOC)
            type_ = WirelessAccessPointInterface::WIRELESS_TYPE_INDEPENDENT;
          else
            type_ = WirelessAccessPointInterface::WIRELESS_TYPE_ANY;
          break;
        }
      default:
        return true;
    }
    return true;
  }
  DBusProxyFactory factory_;
  DBusProxy *proxy_;
  DBusProxy *connect_proxy_;
  std::string path_;

  std::string name_;
  WirelessAccessPointInterface::Type type_;
  int strength_;

  time_t last_check_time_in_sec_;
  static const time_t kCheckInterval = 5;
};

WirelessAccessPoint::WirelessAccessPoint(const std::string &name)
    : impl_(new Impl(name)) {
}

WirelessAccessPoint::~WirelessAccessPoint() {
  delete impl_;
  impl_ = NULL;
}

void WirelessAccessPoint::Destroy() {
  delete this;
}

std::string WirelessAccessPoint::GetName() const {
  return impl_->GetName();
}

WirelessAccessPointInterface::Type WirelessAccessPoint::GetType() const {
  return impl_->GetType();
}

int WirelessAccessPoint::GetSignalStrength() const {
  return impl_->GetSignalStrength();
}

void WirelessAccessPoint::Connect(Slot1<void, bool> *callback) {
  impl_->Connect(callback);
}

void WirelessAccessPoint::Disconnect(Slot1<void, bool> *callback) {
  impl_->Disconnect(callback);
}

} // namespace linux_system
} // namespace framework
} // namespace ggadget
