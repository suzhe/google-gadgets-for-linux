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
#include <ggadget/file_system_interface.h>
#include <ggadget/audioclip_interface.h>

namespace ggadget {

class LinuxFramework : public FrameworkInterface {
 public:
  LinuxFramework();
  virtual ~LinuxFramework();

 public:
  virtual framework::MachineInterface *GetMachine();
  virtual framework::MemoryInterface *GetMemory();
  virtual framework::NetworkInterface *GetNetwork();
  virtual framework::PerfmonInterface *GetPerfmon();
  virtual framework::PowerInterface *GetPower();
  virtual framework::ProcessInterface *GetProcess();
  virtual framework::WirelessInterface *GetWireless();
  virtual framework::FileSystemInterface *GetFileSystem();
  virtual framework::AudioclipInterface *CreateAudioclip(const char *src);

 private:
  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_LINUX_FRAMEWORK_H__
