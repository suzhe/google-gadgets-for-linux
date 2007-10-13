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

#ifndef GGADGET_FRAMEWORK_SYSTEM_WIRELESS_ACCESS_POINT_H__
#define GGADGET_FRAMEWORK_SYSTEM_WIRELESS_ACCESS_POINT_H__

#include "wireless_access_point_interface.h"

namespace ggadget {

namespace framework {

namespace system {

namespace network {

class WirelessAccessPoint : public WirelessAccessPointInterface {
 public:
  virtual void Destroy();

 public:
  virtual const char *GetName() const;
  virtual
      WirelessAccessPointInterface::APType GetType() const;
  virtual int GetSignalStrength() const;
  virtual void Connect(Slot1<void, bool> *callback);
  virtual void Disconnect(Slot1<void, bool> *callback);
};

} // namespace network

} // namespace system

} // namespace framework

} // namespace ggadget

#endif // GGADGET_FRAMEWORK_SYSTEM_WIRELESS_ACCESS_POINT_H__
