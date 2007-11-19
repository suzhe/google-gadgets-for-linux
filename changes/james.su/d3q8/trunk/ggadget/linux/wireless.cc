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

#include <cstddef>
#include "wireless.h"
#include "wireless_access_point.h"

namespace ggadget {
namespace framework {

bool Wireless::IsAvailable() const {
  return true;
}

bool Wireless::IsConnected() const {
  return true;
}

bool Wireless::EnumerationSupported() const {
  return true;
}

int Wireless::GetAPCount() const {
  return 1;
}

const WirelessAccessPointInterface *Wireless::GetWirelessAccessPoint(int index)
    const {
  if (index == 0)
    return new WirelessAccessPoint();
  return NULL;
}

WirelessAccessPointInterface *Wireless::GetWirelessAccessPoint(int index) {
  if (index == 0)
    return new WirelessAccessPoint();
  return NULL;
}

const char *Wireless::GetName() const {
  return "Dummy Wireless";
}

const char *Wireless::GetNetworkName() const {
  return "Dummy Wireless Network";
}

int Wireless::GetSignalStrength() const {
  return 87;
}

} // namespace framework
} // namespace ggadget
