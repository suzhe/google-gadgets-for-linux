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

#ifndef GGADGET_LINUX_FRAMEWORK_H__
#define GGADGET_LINUX_FRAMEWORK_H__

#include <ggadget/framework_interface.h>
#include <ggadget/linux/machine.h>
#include <ggadget/linux/memory.h>
#include <ggadget/linux/network.h>
#include <ggadget/linux/perfmon.h>
#include <ggadget/linux/power.h>
#include <ggadget/linux/process.h>
#include <ggadget/linux/wireless.h>

namespace ggadget {

class Framework : public FrameworkInterface {
 public:
  virtual framework::MachineInterface *GetMachineInterface() {
    return &machine_;
  }
  virtual framework::MemoryInterface *GetMemoryInterface() {
    return &memory_;
  }
  virtual framework::NetworkInterface *GetNetworkInterface() {
    return &network_;
  }
  virtual framework::PerfmonInterface *GetPerfmonInterface() {
    return &perfmon_;
  }
  virtual framework::PowerInterface *GetPowerInterface() {
    return &power_;
  }
  virtual framework::ProcessInterface *GetProcessInterface() {
    return &process_;
  }
  virtual framework::WirelessInterface *GetWirelessInterface() {
    return &wireless_;
  }

 private:
  framework::Machine machine_;
  framework::Memory memory_;
  framework::Network network_;
  framework::Perfmon perfmon_;
  framework::Power power_;
  framework::Process process_;
  framework::Wireless wireless_;
};

} // namespace ggadget

#endif // GGADGET_LINUX_MACHINE_H__
