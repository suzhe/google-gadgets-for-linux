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

/**
 * Constant strings used to represent Hal properties.
 */

const char* const kHalDBusName       = "org.freedesktop.Hal";

const char* const kHalComputerPath   = "/org/freedesktop/Hal/devices/computer";
const char* const kHalManagerPath    = "/org/freedesktop/Hal/Manager";

const char* const kHalComputerInterface  = "org.freedesktop.Hal.Device";
const char* const kHalManagerInterface   = "org.freedesktop.Hal.Manager";

const char* const kHalPropertyMethod        = "GetProperty";
const char* const kHalFindDeviceMethod      = "FindDeviceByCapability";
const char* const kHalQueryCapabilityMethod = "QueryCapability";

const char* const kHalNet80203Property       = "net.80203";
const char* const kHalNet80211Property       = "net.80211";
const char* const kHalNetBlueToothProperty   = "net.bluetooth";
const char* const kHalNetIrdaProperty        = "net.irda";
const char* const kHalNetInterfaceOnProperty = "net.interface_up";
const char* const kHalNetInterfaceProperty   = "net.interface";
const char* const kHalNetMediaProperty       = "net.media";
const char* const kHalNetOriginalProperty    = "net.originating_device";

const char* const kNewUUIDProperty           = "system.hardware.uuid";
const char* const kOldUUIDProperty           = "smbios.system.uuid";
const char* const kNewVendorProperty         = "system.hardware.vendor";
const char* const kOldVendorProperty         = "system.vendor";
const char* const kMachineModelProperty      = "system.product";

const char* const kHalCapabilitiesProperty   = "info.capabilities";
const char* const kHalCategoryProperty       = "info.category";

const char* const kHalChargingProperty     = "battery.is_charging";
const char* const kHalPercentageProperty   = "battery.charge_level.percentage";
const char* const kHalRemainingProperty    = "battery.remaining_time";
const char* const kHalTotalTimeProperty    = "battery.charge_level.design";

const char* const kNMDBusName       = "org.freedesktop.NetworkManager";
const char* const kNMDBusPath       = "/org/freedesktop/NetworkManager";
const char* const kNMDBusInterface  = kNMDBusName;

const char* const kNMInfoDBusName      = "org.freedesktop.NetworkManagerInfo";
const char* const kNMInfoDBusPath      = "/org/freedesktop/NetworkManagerInfo";
const char* const kNMInfoDBusInterface = kNMInfoDBusName;

const char* const kNMGetDevicesMethod     = "getDevices";
const char* const kNMGetPropertiesMethod  = "getProperties";
