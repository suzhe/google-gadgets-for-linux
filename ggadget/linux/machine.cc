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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/utsname.h>
#include <vector>

#include "machine.h"
#include "hal_strings.h"
#include <ggadget/common.h>
#include <ggadget/slot.h>
#include <ggadget/string_utils.h>

#include <ggadget/dbus/dbus_proxy.h>

namespace ggadget {
namespace framework {
namespace linux_os {

using ggadget::dbus::DBusProxy;
using ggadget::dbus::DBusProxyFactory;
using ggadget::dbus::MESSAGE_TYPE_INVALID;
using ggadget::dbus::MESSAGE_TYPE_STRING;

namespace {

const char* kKeysInMachineInfo[] = {
  "cpu family", "model", "stepping",
  "vendor_id", "model name", "cpu MHz"
};

// Represents the file names for reading CPU info.
const char* kCPUInfoFile = "/proc/cpuinfo";

class StringValue {
 public:
  StringValue() {
  }
  ~StringValue() {
  }
  std::string string() const { return str_; }
  void Reset() { str_.clear(); }
  bool Callback(int id, const Variant &value) {
    value.ConvertToString(&str_);
    return true;
  }
 private:
  std::string str_;
};

}  // namespace anonymous

Machine::Machine() {
  InitArchInfo();
  InitProcInfo();
  DBusProxyFactory factory(NULL);
  DBusProxy *proxy = factory.NewSystemProxy(kHalDBusName,
                                            kHalComputerPath,
                                            kHalComputerInterface,
                                            false);
  StringValue obj;
  if (!proxy->Call(kHalPropertyMethod, true, -1,
                   NewSlot(&obj, &StringValue::Callback),
                   MESSAGE_TYPE_STRING, kNewUUIDProperty,
                   MESSAGE_TYPE_INVALID)) {
    /** The Hal specification changed one time. */
    proxy->Call(kHalPropertyMethod, true, -1,
                NewSlot(&obj, &StringValue::Callback),
                MESSAGE_TYPE_STRING, kOldUUIDProperty,
                MESSAGE_TYPE_INVALID);
  }
  serial_number_ = obj.string();
  obj.Reset();

  /** get machine vendor */
  if (!proxy->Call(kHalPropertyMethod, true, -1,
                   NewSlot(&obj, &StringValue::Callback),
                   MESSAGE_TYPE_STRING, kNewVendorProperty,
                   MESSAGE_TYPE_INVALID)) {
    /** The Hal specification changed one time. */
    proxy->Call(kHalPropertyMethod, true, -1,
                NewSlot(&obj, &StringValue::Callback),
                MESSAGE_TYPE_STRING, kOldVendorProperty,
                MESSAGE_TYPE_INVALID);
  }
  machine_vendor_ = obj.string();
  obj.Reset();

  /** get machine model */
  proxy->Call(kHalPropertyMethod, true, -1,
              NewSlot(&obj, &StringValue::Callback),
              MESSAGE_TYPE_STRING, kMachineModelProperty,
              MESSAGE_TYPE_INVALID);
  machine_model_ = obj.string();
  delete proxy;
}

Machine::~Machine() {
}

std::string Machine::GetBiosSerialNumber() const {
  return serial_number_;
}

std::string Machine::GetMachineManufacturer() const {
  return machine_vendor_;
}

std::string Machine::GetMachineModel() const {
  return machine_model_;
}

std::string Machine::GetProcessorArchitecture() const {
  return sysinfo_[CPU_ARCH];
}

int Machine::GetProcessorCount() const {
  return cpu_count_;
}

int Machine::GetProcessorFamily() const {
  return strtol(sysinfo_[CPU_FAMILY].c_str(), NULL, 10);
}

int Machine::GetProcessorModel() const {
  return strtol(sysinfo_[CPU_MODEL].c_str(), NULL, 10);
}

std::string Machine::GetProcessorName() const {
  return sysinfo_[CPU_NAME];
}

int Machine::GetProcessorSpeed() const {
  return strtol(sysinfo_[CPU_SPEED].c_str(), NULL, 10);
}

int Machine::GetProcessorStepping() const {
  return strtol(sysinfo_[CPU_STEPPING].c_str(), NULL, 10);
}

std::string Machine::GetProcessorVendor() const {
  return sysinfo_[CPU_VENDOR];
}

void Machine::InitArchInfo() {
  utsname name;
  if (uname(&name) == -1) { // indicates error when -1 is returned.
    sysinfo_[CPU_ARCH] = "";
    return;
  }

  sysinfo_[CPU_ARCH] = std::string(name.machine);
}

void Machine::InitProcInfo() {
  FILE* fp = fopen(kCPUInfoFile, "r");
  if (fp == NULL)
    return;

  char line[1001] = { 0 };
  cpu_count_ = 0;
  std::string key, value;

  // get the processor count
  while (fgets(line, sizeof(line) - 1, fp)) {
    if (!SplitString(line, ":", &key, &value))
      continue;

    key = TrimString(key);
    value = TrimString(value);

    if (key == "processor") {
      cpu_count_ ++;
      continue;
    }

    if (cpu_count_ > 1) continue;

    for (size_t i = 0; i < arraysize(kKeysInMachineInfo); ++i) {
      if (key == kKeysInMachineInfo[i]) {
        sysinfo_[i] = value;
        break;
      }
    }
  }

  fclose(fp);
}

} // namespace linux_os
} // namespace framework
} // namespace ggadget
