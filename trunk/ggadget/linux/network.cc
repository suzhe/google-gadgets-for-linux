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

#include "network.h"

namespace ggadget {
namespace framework {
namespace linux {

bool Network::IsOnline() const {
  // TODO:
  return true;
}

NetworkInterface::ConnectionType Network::GetConnectionType() const {
  // TODO:
  return NetworkInterface::CONNECTION_TYPE_802_3;
}

NetworkInterface::PhysicalMediaType Network::GetPhysicalMediaType() const {
  // TODO:
  return NetworkInterface::PHISICAL_MEDIA_TYPE_UNSPECIFIED;
}

} // namespace linux
} // namespace framework
} // namespace ggadget
