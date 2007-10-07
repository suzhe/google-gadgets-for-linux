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

#include "wireless_access_point.h"

#include "ggadget/slot.h"
#include "ggadget/variant.h"

namespace ggadget {

namespace framework {

namespace system {

namespace network {

void WirelessAccessPoint::Destroy() {
  delete this;
}

const char *WirelessAccessPoint::GetName() const {
  return "Dummy Wireless Access Point";
}

WirelessAccessPointInterface::APType WirelessAccessPoint::GetType() const {
  return WirelessAccessPointInterface::APTYPE_WIRELESS_TYPE_ANY;
}

int WirelessAccessPoint::GetSignalStrength() const {
  return 87;
}

void WirelessAccessPoint::Connect(ggadget::Slot1<void, bool> *callback) {
  if (callback) {
    ggadget::Variant result(true);
    callback->Call(1, &result);
    delete callback;
  }
}

void WirelessAccessPoint::Disconnect(ggadget::Slot1<void, bool> *callback) {
  if (callback) {
    ggadget::Variant result(true);
    callback->Call(1, &result);
    delete callback;
  }
}

} // namespace network

} // namespace system

} // namespace framework

} // namespace ggadget
