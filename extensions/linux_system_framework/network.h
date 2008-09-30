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

#ifndef EXTENSIONS_LINUX_SYSTEM_FRAMEWORK_NETWORK_H__
#define EXTENSIONS_LINUX_SYSTEM_FRAMEWORK_NETWORK_H__

#include <vector>
#include <string>
#include <ggadget/framework_interface.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/logger.h>
#include <ggadget/slot.h>
#include <ggadget/string_utils.h>
#include <ggadget/dbus/dbus_proxy.h>
#include <ggadget/dbus/dbus_result_receiver.h>
#include "wireless.h"

namespace ggadget {
namespace framework {
namespace linux_system {

using namespace ggadget::dbus;

class Network : public NetworkInterface {
 public:
  Network();
  ~Network();

  virtual bool IsOnline() ;
  virtual NetworkInterface::ConnectionType GetConnectionType() ;
  virtual NetworkInterface::PhysicalMediaType GetPhysicalMediaType() ;
  virtual WirelessInterface *GetWireless();

 private:
  DBusProxy *GetInterfaceProxy(int i);
  int GetActiveInterface();
  std::string GetInterfacePropertyString(int i, const char *property);
  bool IsInterfaceUp(int i);

  int last_active_interface_;
  std::vector<std::string> interfaces_;
  std::vector<DBusProxy *> proxies_;
  Wireless wireless_;
};

} // namespace linux_system
} // namespace framework
} // namespace ggadget

#endif // EXTENSIONS_LINUX_SYSTEM_FRAMEWORK_NETWORK_H__
