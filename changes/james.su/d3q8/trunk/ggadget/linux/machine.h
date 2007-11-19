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

#ifndef GGADGET_LINUX_MACHINE_H__
#define GGADGET_LINUX_MACHINE_H__

#include <ggadget/framework_interface.h>

namespace ggadget {
namespace framework {

class Machine : public MachineInterface {
 public:
  virtual const char *GetBiosSerialNumber() const;
  virtual const char *GetMachineManufacturer() const;
  virtual const char *GetMachineModel() const;
  virtual const char *GetProcessorArchitecture() const;
  virtual int GetProcessorCount() const;
  virtual int GetProcessorFamily() const;
  virtual int GetProcessorModel() const;
  virtual const char *GetProcessorName() const;
  virtual int GetProcessorSpeed() const;
  virtual int GetProcessorStepping() const;
  virtual const char *GetProcessorVendor() const;
};

} // namespace framework
} // namespace ggadget

#endif // GGADGET_LINUX_MACHINE_H__
