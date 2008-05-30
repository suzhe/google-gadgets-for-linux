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

#include "runtime.h"
#include <ggadget/sysdeps.h>
#include <ggadget/logger.h>
#include <ggadget/dbus/dbus_result_receiver.h>
#include "hal_strings.h"

namespace ggadget {
namespace framework {
namespace linux_system {

Runtime::Runtime() {
  DBusProxyFactory factory(NULL);
  DBusProxy *proxy = factory.NewSystemProxy(kHalDBusName,
                                            kHalObjectComputer,
                                            kHalInterfaceDevice,
                                            false);
  ASSERT(proxy);
  DBusStringReceiver name_receiver;
  if (!proxy->Call(kHalMethodGetProperty, true, -1,
                   name_receiver.NewSlot(),
                   MESSAGE_TYPE_STRING, kHalPropSystemKernelName,
                   MESSAGE_TYPE_INVALID)) {
    DLOG("Failed to get kernel name.");
    os_name_ = GGL_PLATFORM;
  } else {
    os_name_ = name_receiver.GetValue();
  }
  DBusStringReceiver version_receiver;
  if (!proxy->Call(kHalMethodGetProperty, true, -1,
                   version_receiver.NewSlot(),
                   MESSAGE_TYPE_STRING, kHalPropSystemKernelVersion,
                   MESSAGE_TYPE_INVALID)) {
    DLOG("Failed to get kernel version.");
  } else {
    os_version_ = version_receiver.GetValue();
  }
  delete proxy;
}

Runtime::~Runtime() {
}

} // namespace linux_system
} // namespace framework
} // namespace ggadget
