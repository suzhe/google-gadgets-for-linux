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

#include "machine.h"

namespace ggadget {

namespace framework {

namespace system {

const char *Machine::GetBiosSerialNumber() const {
  return "007";
}

const char *Machine::GetMachineManufacturer() const {
  return "Google";
}

const char *Machine::GetMachineModel() const {
  return "Google007";
}

const char *Machine::GetProcessorArchitecture() const {
  return "GoogleX86";
}

int Machine::GetProcessorCount() const {
  return 1;
}

int Machine::GetProcessorFamily() const {
  return 7;
}

int Machine::GetProcessorModel() const {
  return 8;
}

const char *Machine::GetProcessorName() const {
  return "GoogleMachine";
}

int Machine::GetProcessorSpeed() const {
  return 9999;
}

int Machine::GetProcessorStepping() const {
  return 5;
}

const char *Machine::GetProcessorVender() const {
  return "Google";
}

} // namespace system

} // namespace framework

} // namespace ggadget
