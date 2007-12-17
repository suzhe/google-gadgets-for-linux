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

#include "framework.h"
#include <ggadget/linux/machine.h>
#include <ggadget/linux/memory.h>
#include <ggadget/linux/network.h>
#include <ggadget/linux/perfmon.h>
#include <ggadget/linux/power.h>
#include <ggadget/linux/process.h>
#include <ggadget/linux/wireless.h>
#include <ggadget/linux/wireless_access_point.h>
#include <ggadget/linux/file_system.h>

namespace ggadget {

struct LinuxFramework::Impl {
  framework::linux_os::Machine machine_;
  framework::linux_os::Memory memory_;
  framework::linux_os::Network network_;
  framework::linux_os::Perfmon perfmon_;
  framework::linux_os::Power power_;
  framework::linux_os::Process process_;
  framework::linux_os::Wireless wireless_;
  framework::linux_os::FileSystem filesystem_;
};

LinuxFramework::LinuxFramework()
  : impl_(new Impl()) {
}

LinuxFramework::~LinuxFramework() {
  delete impl_;
}

framework::MachineInterface *LinuxFramework::GetMachine() {
  return &impl_->machine_;
}
framework::MemoryInterface *LinuxFramework::GetMemory() {
  return &impl_->memory_;
}
framework::NetworkInterface *LinuxFramework::GetNetwork() {
  return &impl_->network_;
}
framework::PerfmonInterface *LinuxFramework::GetPerfmon() {
  return &impl_->perfmon_;
}
framework::PowerInterface *LinuxFramework::GetPower() {
  return &impl_->power_;
}
framework::ProcessInterface *LinuxFramework::GetProcess() {
  return &impl_->process_;
}
framework::WirelessInterface *LinuxFramework::GetWireless() {
  return &impl_->wireless_;
}
framework::FileSystemInterface *LinuxFramework::GetFileSystem() {
  return &impl_->filesystem_;
}
framework::AudioclipInterface *
LinuxFramework::CreateAudioclip(const char *src) {
  // TODO:
  return NULL;
}

} // namespace ggadget
