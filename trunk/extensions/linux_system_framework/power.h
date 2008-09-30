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

#ifndef EXTENSIONS_LINUX_SYSTEM_FRAMEWORK_POWER_H__
#define EXTENSIONS_LINUX_SYSTEM_FRAMEWORK_POWER_H__

#include <ggadget/framework_interface.h>
#include <ggadget/dbus/dbus_proxy.h>

namespace ggadget {
namespace framework {
namespace linux_system {

using namespace ggadget::dbus;

class Power : public PowerInterface {
 public:
  Power();
  ~Power();

  virtual bool IsCharging();
  virtual bool IsPluggedIn();
  virtual int GetPercentRemaining();
  virtual int GetTimeRemaining();
  virtual int GetTimeTotal();

 private:
  DBusProxy *battery_;
  DBusProxy *ac_adapter_;
};

} // namespace linux_system
} // namespace framework
} // namespace ggadget

#endif // EXTENSIONS_LINUX_SYSTEM_FRAMEWORK_POWER_H__
