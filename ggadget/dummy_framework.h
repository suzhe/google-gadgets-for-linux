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

#ifndef GGADGET_DUMMY_FRAMEWORK_H__
#define GGADGET_DUMMY_FRAMEWORK_H__

#include <string>
#include <stdint.h>
#include <ggadget/framework_interface.h>

namespace ggadget {

class framework::MachineInterface;
class framework::MemoryInterface;
class framework::NetworkInterface;
class framework::PerfmonInterface;
class framework::PowerInterface;
class framework::ProcessInterface;
class framework::WirelessInterface;
class framework::FileSystemInterface;
class framework::AudioclipInterface;

/**
 * A dummy implementation of FrameworkInterface for unsupported systems;
 */
class DummyFramework : public FrameworkInterface {
 public:
  DummyFramework();
  virtual ~DummyFramework();

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

#endif // GGADGET_FRAMEWORK_INTERFACE_H__
