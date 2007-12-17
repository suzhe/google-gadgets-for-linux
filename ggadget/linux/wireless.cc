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
namespace linux {

bool Wireless::IsAvailable() const {
  // TODO:
  return true;
}

bool Wireless::IsConnected() const {
  // TODO:
  return true;
}

bool Wireless::EnumerationSupported() const {
  // TODO:
  return true;
}

int Wireless::GetAPCount() const {
  // TODO:
  return 1;
}

const WirelessAccessPointInterface *
Wireless::GetWirelessAccessPoint(int index) const {
  // TODO:
  if (index == 0)
    return new WirelessAccessPoint();
  return NULL;
}

WirelessAccessPointInterface *Wireless::GetWirelessAccessPoint(int index) {
  // TODO:
  if (index == 0)
    return new WirelessAccessPoint();
  return NULL;
}

std::string Wireless::GetName() const {
  // TODO:
  return "Dummy Wireless";
}

std::string Wireless::GetNetworkName() const {
  // TODO:
  return "Dummy Wireless Network";
}

int Wireless::GetSignalStrength() const {
  // TODO:
  return 87;
}

} // namespace linux
} // namespace framework
} // namespace ggadget
