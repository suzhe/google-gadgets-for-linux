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

#include <ggadget/main_loop_interface.h>
#include "network.h"
#include "hal_strings.h"

namespace ggadget {
namespace framework {
namespace linux_system {

Network::Network()
  : last_active_interface_(-1) {
  DBusProxy *proxy = DBusProxy::NewSystemProxy(kHalDBusName,
                                               kHalObjectManager,
                                               kHalInterfaceManager);
  DBusStringArrayReceiver receiver(&interfaces_);
  if (!proxy->CallMethod(kHalMethodFindDeviceByCapability, true,
                         kDefaultDBusTimeout, receiver.NewSlot(),
                         MESSAGE_TYPE_STRING, kHalCapabilityNet,
                         MESSAGE_TYPE_INVALID)) {
    DLOG("Get devices failed.");
    interfaces_.clear();
    last_active_interface_ = -2;
  }
  delete proxy;

#ifdef _DEBUG
  DLOG("Network interfaces:");
  for (size_t i = 0; i < interfaces_.size(); ++i)
    DLOG("%s", interfaces_[i].c_str());
#endif

  // Lazy initialize proxies for interfaces.
  proxies_.resize(interfaces_.size(), NULL);
}

Network::~Network() {
  for(size_t i = 0; i < proxies_.size(); ++i)
    delete proxies_[i];
}

bool Network::IsOnline() {
  // Also returns true if dbus or service is not available.
  return GetActiveInterface() != -1;
}

NetworkInterface::ConnectionType Network::GetConnectionType() {
  int index = GetActiveInterface();
  if (index >= 0) {
    std::string category =
        GetInterfacePropertyString(index, kHalPropInfoCategory);
    DLOG("category: %s", category.c_str());
    if (category == kHalPropNet80203)
      return CONNECTION_TYPE_802_3;
    else if (category == kHalPropNet80211)
      return CONNECTION_TYPE_NATIVE_802_11;
    else if (category == kHalPropNetBlueTooth)
      return CONNECTION_TYPE_BLUETOOTH;
    else if (category == kHalPropNetIrda)
      return CONNECTION_TYPE_IRDA;
    else
      LOG("the net interface %s is a unknown type: %s",
          interfaces_[index].c_str(), category.c_str());
  }
  return CONNECTION_TYPE_UNKNOWN;
}

NetworkInterface::PhysicalMediaType Network::GetPhysicalMediaType() {
  NetworkInterface::ConnectionType type = GetConnectionType();
  switch (type) {
    case CONNECTION_TYPE_NATIVE_802_11:
      return PHYSICAL_MEDIA_TYPE_NATIVE_802_11;
    case CONNECTION_TYPE_BLUETOOTH:
      return PHYSICAL_MEDIA_TYPE_BLUETOOTH;
    default:
      return PHYSICAL_MEDIA_TYPE_UNSPECIFIED;
  }
  return PHYSICAL_MEDIA_TYPE_UNSPECIFIED;
}

WirelessInterface *Network::GetWireless() {
  return &wireless_;
}

DBusProxy *Network::GetInterfaceProxy(int i) {
  ASSERT(i >= 0);
  // Size of proxies may be zero, if now no network device is available.
  if (static_cast<size_t>(i) >= proxies_.size())
    return NULL;
  if (proxies_[i] == NULL) {
    proxies_[i] = DBusProxy::NewSystemProxy(kHalDBusName,
                                            interfaces_[i].c_str(),
                                            kHalInterfaceDevice);
  }
  return proxies_[i];
}

int Network::GetActiveInterface() {
  if (last_active_interface_ == -2) {
    // DBus or service is not available.
    return last_active_interface_;
  }
  if (last_active_interface_ >= 0) {
    if (IsInterfaceUp(last_active_interface_))
      return last_active_interface_;
    else
      last_active_interface_ = -1;
  }
  for (size_t i = 0; i < interfaces_.size(); ++i) {
    if (IsInterfaceUp(static_cast<int>(i))) {
      last_active_interface_ = static_cast<int>(i);
      break;
    }
  }
  return last_active_interface_;
}

std::string Network::GetInterfacePropertyString(int i, const char *property) {
  DBusProxy *proxy = GetInterfaceProxy(i);
  ASSERT(proxy);

  DBusStringReceiver receiver;
  proxy->CallMethod(kHalMethodGetProperty, true, kDefaultDBusTimeout,
                    receiver.NewSlot(),
                    MESSAGE_TYPE_STRING, property,
                    MESSAGE_TYPE_INVALID);
  return receiver.GetValue();
}

bool Network::IsInterfaceUp(int i) {
  DBusProxy *proxy = GetInterfaceProxy(i);
  ASSERT(proxy);

  DBusBooleanReceiver receiver;
  if (proxy->CallMethod(kHalMethodGetProperty, true, kDefaultDBusTimeout,
                        receiver.NewSlot(),
                        MESSAGE_TYPE_STRING, kHalPropNetInterfaceUp,
                        MESSAGE_TYPE_INVALID)) {
    return receiver.GetValue();
  }

  DLOG("net.interface_up property is missing.");

  std::string category =
      GetInterfacePropertyString(i, kHalPropInfoCategory);

  // always returns true for Ethernet interface.
  // FIXME: We should use NetworkManager to detect the correct value.
  if (category == kHalPropNet80203)
    return true;

  return false;
}

} // namespace linux_system
} // namespace framework
} // namespace ggadget
