/*
  Copyright 2008 Google Inc.

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

#include "power.h"

#include "hal_strings.h"
#include <ggadget/logger.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/slot.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/dbus/dbus_proxy.h>
#include <ggadget/dbus/dbus_result_receiver.h>

namespace ggadget {
namespace framework {
namespace linux_system {

Power::Power()
  : factory_(GetGlobalMainLoop()),
    battery_(NULL),
    ac_adapter_(NULL) {
  std::vector<std::string> strlist;
  DBusStringArrayReceiver strlist_receiver(&strlist);

  DBusProxy *proxy = factory_.NewSystemProxy(kHalDBusName,
                                             kHalObjectManager,
                                             kHalInterfaceManager,
                                             false);
  ASSERT(proxy);

  // Initialize battery.
  if (proxy->Call(kHalMethodFindDeviceByCapability, true, -1,
                  strlist_receiver.NewSlot(),
                  MESSAGE_TYPE_STRING, kHalCapabilityBattery,
                  MESSAGE_TYPE_INVALID) && strlist.size()) {
    // Find out the primary battery.
    std::vector<DBusProxy *> batteries;
    batteries.resize(strlist.size(), NULL);
    for (size_t i = 0; i < strlist.size(); ++i) {
      batteries[i] = factory_.NewSystemProxy(kHalDBusName,
                                             strlist[i].c_str(),
                                             kHalInterfaceDevice,
                                             false);
      DLOG("Found battery %s", strlist[i].c_str());
    }
    for (size_t i = 0; i < batteries.size(); ++i) {
      DBusStringReceiver str_receiver;
      if (proxy->Call(kHalMethodGetProperty, true, -1,
                      str_receiver.NewSlot(),
                      MESSAGE_TYPE_STRING, kHalPropBatteryType,
                      MESSAGE_TYPE_INVALID) &&
          str_receiver.GetValue() == "primary") {
        battery_ = batteries[i];
        batteries[i] = NULL;
        DLOG("Primary battery is: %s", strlist[i].c_str());
        break;
      }
    }
    // If no primary battery then use the first one.
    if (!battery_) {
      battery_ = batteries[0];
      batteries[0] = NULL;
    }
    for (size_t i = 0; i < batteries.size(); ++i)
      delete batteries[i];
  }

  // Initialize ac_adapter.
  strlist.clear();
  if (proxy->Call(kHalMethodFindDeviceByCapability, true, -1,
                  strlist_receiver.NewSlot(),
                  MESSAGE_TYPE_STRING, kHalCapabilityACAdapter,
                  MESSAGE_TYPE_INVALID) && strlist.size()) {
    ac_adapter_ = factory_.NewSystemProxy(kHalDBusName,
                                          strlist[0].c_str(),
                                          kHalInterfaceDevice,
                                          false);
    DLOG("Found AC adapter %s", strlist[0].c_str());
  }

  if (!battery_)
    DLOG("No battery found.");
  if (!ac_adapter_)
    DLOG("No AC adapter found.");

  delete proxy;
}

Power::~Power() {
  delete battery_;
  delete ac_adapter_;
  battery_ = NULL;
  ac_adapter_ = NULL;
}

bool Power::IsCharging() {
  if (!battery_) return false;

  DBusBooleanReceiver result;
  battery_->Call(kHalMethodGetProperty, true, -1,
                 result.NewSlot(),
                 MESSAGE_TYPE_STRING, kHalPropBatteryRechargableIsCharging,
                 MESSAGE_TYPE_INVALID);
  return result.GetValue();
}

bool Power::IsPluggedIn() {
  if (!battery_) return true;
  if (!ac_adapter_) return false;

  DBusBooleanReceiver result;
  ac_adapter_->Call(kHalMethodGetProperty, true, -1,
                    result.NewSlot(),
                    MESSAGE_TYPE_STRING, kHalPropACAdapterPresent,
                    MESSAGE_TYPE_INVALID);
  return result.GetValue();
}

int Power::GetPercentRemaining() {
  if (!battery_) return 0;

  DBusIntReceiver percent;
  if (battery_->Call(kHalMethodGetProperty, true, -1,
                     percent.NewSlot(),
                     MESSAGE_TYPE_STRING, kHalPropBatteryChargeLevelPercentage,
                     MESSAGE_TYPE_INVALID)) {
    return percent.GetValue();
  }

  DLOG("battery.charge_level.percentage is missing.");

  // battery.charge_level.percentage is not available, calculate it manually.
  DBusIntReceiver design, current;
  if (battery_->Call(kHalMethodGetProperty, true, -1,
                     design.NewSlot(),
                     MESSAGE_TYPE_STRING, kHalPropBatteryChargeLevelDesign,
                     MESSAGE_TYPE_INVALID) &&
      battery_->Call(kHalMethodGetProperty, true, -1,
                     current.NewSlot(),
                     MESSAGE_TYPE_STRING, kHalPropBatteryChargeLevelCurrent,
                     MESSAGE_TYPE_INVALID) && design.GetValue() > 0) {
    return design.GetValue() <= 0 ?
           0 : current.GetValue() * 100 / design.GetValue();
  }

  DLOG("battery.charge_level.design/current is missing.");

  return 0;
}

int Power::GetTimeRemaining() {
  if (!battery_) return 0;

  DBusIntReceiver remaining;
  if (battery_->Call(kHalMethodGetPropertyInt, true, -1,
                     remaining.NewSlot(),
                     MESSAGE_TYPE_STRING, kHalPropBatteryRemainingTime,
                     MESSAGE_TYPE_INVALID)) {
    return remaining.GetValue();
  }

  DLOG("battery.remaining_time is missing.");

  // battery.remaining_time is not available, calculate it manually.
  DBusIntReceiver design, current, rate;
  if (battery_->Call(kHalMethodGetProperty, true, -1,
                     design.NewSlot(),
                     MESSAGE_TYPE_STRING, kHalPropBatteryChargeLevelDesign,
                     MESSAGE_TYPE_INVALID) &&
      battery_->Call(kHalMethodGetProperty, true, -1,
                     current.NewSlot(),
                     MESSAGE_TYPE_STRING, kHalPropBatteryChargeLevelCurrent,
                     MESSAGE_TYPE_INVALID) &&
      battery_->Call(kHalMethodGetProperty, true, -1,
                     rate.NewSlot(),
                     MESSAGE_TYPE_STRING, kHalPropBatteryChargeLevelRate,
                     MESSAGE_TYPE_INVALID) && rate.GetValue() > 0) {
    // If the battery is charging then return the remaining time to full.
    // else return the remaining time to empty.
    if (IsCharging()) {
      return (design.GetValue() - current.GetValue()) / rate.GetValue();
    } else {
      return current.GetValue() / rate.GetValue();
    }
  }

  DLOG("Failed to calculate remaining time.");

  return 0;
}

int Power::GetTimeTotal() {
  if (!battery_) return 0;

  DBusIntReceiver design, rate;
  if (battery_->Call(kHalMethodGetProperty, true, -1,
                     design.NewSlot(),
                     MESSAGE_TYPE_STRING, kHalPropBatteryChargeLevelDesign,
                     MESSAGE_TYPE_INVALID) &&
      battery_->Call(kHalMethodGetProperty, true, -1,
                     rate.NewSlot(),
                     MESSAGE_TYPE_STRING, kHalPropBatteryChargeLevelRate,
                     MESSAGE_TYPE_INVALID) && rate.GetValue() > 0) {
    return design.GetValue() / rate.GetValue();
  }

  DLOG("Failed to calculate total time.");

  return 0;
}

} // namespace linux_system
} // namespace framework
} // namespace ggadget
