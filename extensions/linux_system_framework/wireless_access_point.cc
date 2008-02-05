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

#include <ggadget/slot.h>
#include <ggadget/variant.h>

#include "wireless_access_point.h"

namespace ggadget {
namespace framework {
namespace linux_system {

void WirelessAccessPoint::Destroy() {
  delete this;
}

std::string WirelessAccessPoint::GetName() const {
  // TODO::
  return "Dummy Wireless Access Point";
}

WirelessAccessPointInterface::Type WirelessAccessPoint::GetType() const {
  // TODO::
  return WirelessAccessPointInterface::WIRELESS_TYPE_ANY;
}

int WirelessAccessPoint::GetSignalStrength() const {
  // TODO::
  return 87;
}

void WirelessAccessPoint::Connect(Slot1<void, bool> *callback) {
  // TODO::
  if (callback) {
    (*callback)(true);
    delete callback;
  }
}

void WirelessAccessPoint::Disconnect(Slot1<void, bool> *callback) {
  // TODO::
  if (callback) {
    (*callback)(true);
    delete callback;
  }
}

} // namespace linux_system
} // namespace framework
} // namespace ggadget
