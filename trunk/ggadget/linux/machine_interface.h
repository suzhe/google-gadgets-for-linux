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

#ifndef GGADGET_FRAMEWORK_SYSTEM_MACHINE_INTERFACE_H__
#define GGADGET_FRAMEWORK_SYSTEM_MACHINE_INTERFACE_H__

namespace ggadget {

namespace framework {

namespace system {

/** Interface for retrieving the information of the machine. */
class MachineInterface {
 protected:
  virtual ~MachineInterface() {}

 public:
  /** Retrieves the BIOS Serial number. */
  virtual const char *GetBiosSerialNumber() const = 0;
  /** Retrieves the machine's manuafacturere name. */
  virtual const char *GetMachineManufacturer() const = 0;
  /** Retrieves the machine's model. */
  virtual const char *GetMachineModel() const = 0;
  /** Retrieves the machine's architecture. */
  virtual const char *GetProcessorArchitecture() const = 0;
  /** Retrieves the number of processors running the gadget. */
  virtual int GetProcessorCount() const = 0;
  /** Retrieves the family name of the processor. */
  virtual int GetProcessorFamily() const = 0;
  /** Retrieves the model number of the processor. */
  virtual int GetProcessorModel() const = 0;
  /** Retrieves the processor's name. */
  virtual const char *GetProcessorName() const = 0;
  /** Gets the speed of the processor, in MHz. */
  virtual int GetProcessorSpeed() const = 0;
  /** Retrieves the step designation of the processor. */
  virtual int GetProcessorStepping() const = 0;
  /** Gets the processor's vender name. */
  virtual const char *GetProcessorVender() const = 0;
};

} // namespace system

} // namespace framework

} // namespace ggadget

#endif // GGADGET_FRAMEWORK_SYSTEM_MACHINE_INTERFACE_H__
