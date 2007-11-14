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

#ifndef GGADGET_FRAMEWORK_FRAMEWORK_INTERFACE_H__
#define GGADGET_FRAMEWORK_FRAMEWORK_INTERFACE_H__

#include <stdint.h>

namespace ggadget {

namespace framework {

class HostSystemInterface;

namespace system {

class MachineInterface;
class MemoryInterface;
class NetworkInterface;
class PerfmonInterface;
class PowerInterface;
class ProcessInterface;

namespace network {

class WirelessInterface;

}; // namespace network

} // namespace system

class FrameworkInterface {
 protected:
  virtual ~FrameworkInterface() {}

 public:
  virtual HostSystemInterface *GetHostSystemInterface() = 0;
  virtual system::MachineInterface *GetMachineInterface() = 0;
  virtual system::MemoryInterface *GetMemoryInterface() = 0;
  virtual system::NetworkInterface *GetNetworkInterface() = 0;
  virtual system::PerfmonInterface *GetPerfmonInterface() = 0;
  virtual system::PowerInterface *GetPowerInterface() = 0;
  virtual system::ProcessInterface *GetProcessInterface() = 0;
  virtual system::network::WirelessInterface *GetWirelessInterface() = 0;
};

} // namespace framework

} // namespace ggadget

#endif // GGADGET_FRAMEWORK_FRAMEWORK_INTERFACE_H__
