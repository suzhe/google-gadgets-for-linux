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

#include "wireless.h"

#include <cstddef>
#include <time.h>

#include <ggadget/dbus/dbus_proxy.h>
#include <ggadget/dbus/dbus_result_receiver.h>
#include <ggadget/scriptable_interface.h>
#include "hal_strings.h"
#include "wireless_access_point.h"

using ggadget::dbus::DBusProxy;
using ggadget::dbus::kDefaultDBusTimeout;
using ggadget::dbus::DBusStringArrayReceiver;
using ggadget::dbus::MESSAGE_TYPE_INVALID;

namespace ggadget {
namespace framework {
namespace linux_system {

const int kNMDeviceType80211Wireless = 2;

class Wireless::Impl {
 public:
  Impl() : network_manager_client_(NULL),
           is_active_(false),
           strength_(0),
           last_check_time_in_sec_(0) {
    GetWirelessDevices();
  }
  ~Impl() {
    for (std::size_t i = 0; i < wireless_proxies_.size(); ++i)
      delete wireless_proxies_[i];
  }
  bool IsAvailable() {
    RefreshWireless();
    return network_manager_client_;
  }
  bool IsConnected() {
    return IsAvailable() && is_active_;
  }
  bool EnumerationSupported() {
    return IsAvailable();
  }
  int GetAPCount() {
    RefreshWireless();
    return static_cast<int>(ap_list_.size());
  }
  WirelessAccessPointInterface* GetWirelessAccessPoint(int index) {
    RefreshWireless();
    if (index < 0 || index >= static_cast<int>(ap_list_.size())) return NULL;
    return new WirelessAccessPoint(ap_list_[index]);
  }
  std::string GetName() {
    RefreshWireless();
    return name_;
  }
  std::string GetNetworkName() {
    RefreshWireless();
    return network_name_;
  }
  int GetSignalStrength() {
    RefreshWireless();
    return strength_;
  }
 private:
  void GetWirelessDevices() {
    DBusProxy *proxy = DBusProxy::NewSystemProxy(kNetworkManagerDBusName,
                                               kNetworkManagerObjectPath,
                                               kNetworkManagerInterface);
    if (!proxy) return;

    std::vector<std::string> strlist;
    DBusStringArrayReceiver receiver(&strlist);
    if (!proxy->CallMethod(kNetworkManagerMethodGetDevices, true,
                           kDefaultDBusTimeout, receiver.NewSlot(),
                           MESSAGE_TYPE_INVALID)) {
      DLOG("Get wireless Devices failed.");
      delete proxy;
      return;
    }
    delete proxy;
    for (std::vector<std::string>::iterator it = strlist.begin();
        it != strlist.end(); ++it) {
      proxy = DBusProxy::NewSystemProxy(kNetworkManagerDBusName,
                                      it->c_str(),
                                      kNetworkManagerInterface);
      find_wireless_device_ = false;
      is_active_ = false;
      proxy->CallMethod(kNetworkManagerMethodGetProperties, true,
                        kDefaultDBusTimeout,
                        NewSlot(this, &Impl::GetDeviceProperties),
                        MESSAGE_TYPE_INVALID);
      if (!find_wireless_device_) {
        delete proxy;
      } else {
        wireless_proxies_.push_back(proxy);
        if (is_active_ && !network_manager_client_)
          network_manager_client_ = proxy;
      }
    }
  }
  void RefreshWireless() {
    time_t now = time(NULL);
    if (now - last_check_time_in_sec_ < kCheckInterval) return;
    // find first active wireless interface now
    network_manager_client_ = NULL;
    for (ProxyVector::iterator it = wireless_proxies_.begin();
         it != wireless_proxies_.end(); ++it) {
      is_active_ = false;
      (*it)->CallMethod(kNetworkManagerMethodGetProperties, true,
                        kDefaultDBusTimeout,
                        NewSlot(this, &Impl::GetDeviceProperties),
                        MESSAGE_TYPE_INVALID);
      if (is_active_) {
        network_manager_client_ = *it;
        break;
      }
    }
  }
  bool GetDeviceProperties(int id, const Variant &value) {
    switch (id) {
      case 1:
        if (!value.ConvertToString(&name_)) return false;
        break;
      case 2:
        {
          int type;
          if (!value.ConvertToInt(&type)) return false;
          if (type == kNMDeviceType80211Wireless)
            find_wireless_device_ = true;
          break;
        }
      case 4:
        if (!value.ConvertToBool(&is_active_)) return false;
        break;
      case 14:
        if (!value.ConvertToInt(&strength_)) return false;
        break;
      case 19:
        if (!value.ConvertToString(&network_name_)) return false;
        DecodeNetworkName(&network_name_);
        break;
      case 20:
        {
          ScriptableInterface *scriptable =
              VariantValue<ScriptableInterface*>()(value);
          ap_list_.clear();
          scriptable->EnumerateElements(NewSlot(this, &Impl::GetAllNetworks));
          break;
        }
      default:
        // ignore other returned properties.
        break;
    }
    return true;
  }
  // decode special charactors in the string, for exsample: _2d_ -> -
  void DecodeNetworkName(std::string *name) {
    std::string::size_type pos = name->rfind("/");
    if (pos != name->npos)
      name->erase(0, pos + 1);
    while ((pos = name->find("_")) != name->npos) {
      if (pos > name->size() - 4 ||
          !isxdigit(name->at(pos + 1)) ||
          !isxdigit(name->at(pos + 2)) ||
          name->at(pos + 3) != '_')
        continue;
      unsigned int ch = 0;
      sscanf(name->c_str() + pos, "_%02x_", &ch);
      if (ch) name->replace(pos, 4, std::string(1, static_cast<char>(ch)));
    }
  }
  bool GetAllNetworks(int id, const Variant &value) {
    std::string str;
    if (!value.ConvertToString(&str)) return false;
    ap_list_.push_back(str);
    return true;
  }
 private:
  // dbus proxies
  typedef std::vector<DBusProxy*> ProxyVector;
  ProxyVector wireless_proxies_;
  DBusProxy *network_manager_client_;

  // properties of current wireless interface
  std::string name_;
  std::string network_name_;
  bool is_active_;
  int strength_;
  std::vector<std::string> ap_list_;

  time_t last_check_time_in_sec_;
  bool find_wireless_device_;
  static const time_t kCheckInterval = 10;
};

Wireless::Wireless() : impl_(new Impl) {
}

Wireless::~Wireless() {
  delete impl_;
  impl_ = NULL;
}

bool Wireless::IsAvailable() const {
  return impl_->IsAvailable();
}

bool Wireless::IsConnected() const {
  return impl_->IsConnected();
}

bool Wireless::EnumerationSupported() const {
  return impl_->GetAPCount() != 0;
}

int Wireless::GetAPCount() const {
  return static_cast<int>(impl_->GetAPCount());
}

const WirelessAccessPointInterface *
Wireless::GetWirelessAccessPoint(int index) const {
  return impl_->GetWirelessAccessPoint(index);
}

WirelessAccessPointInterface *Wireless::GetWirelessAccessPoint(int index) {
  return impl_->GetWirelessAccessPoint(index);
}

std::string Wireless::GetName() const {
  return impl_->GetName();
}

std::string Wireless::GetNetworkName() const {
  return impl_->GetNetworkName();
}

int Wireless::GetSignalStrength() const {
  return impl_->GetSignalStrength();
}

} // namespace linux_system
} // namespace framework
} // namespace ggadget
