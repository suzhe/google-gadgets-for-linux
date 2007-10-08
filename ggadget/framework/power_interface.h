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

#ifndef GGADGET_FRAMEWORK_SYSTEM_POWER_INTERFACE_H__
#define GGADGET_FRAMEWORK_SYSTEM_POWER_INTERFACE_H__

namespace ggadget {

namespace framework {

namespace system {

/** Interface for retrieving the information of the power and battery
 * status. */
class PowerInterface {
 protected:
  virtual ~PowerInterface() {}

 public:
  /** Gets whether the battery is charning.  */
  virtual bool IsCharging() const = 0;
  /** Gets whether the computer is plugged in. */
  virtual bool IsPluggedIn() const = 0;
  /** Gets the remaining battery power in percentage. */
  virtual int GetPercentRemaining() const = 0;
  /**
   * Gets the estimated time, in seconds, left before the battery will need to
   * be charged.
   * */
  virtual int GetTimeRemaining() const = 0;
  /**
   * Gets the estimated time, in seconds, the bettery will work when fully
   * charged.
   */
  virtual int GetTimeTotal() const = 0;
};

} // namespace system

} // namespace framework

} // namespace ggadget

#endif // GGADGET_FRAMEWORK_SYSTEM_POWER_INTERFACE_H__
