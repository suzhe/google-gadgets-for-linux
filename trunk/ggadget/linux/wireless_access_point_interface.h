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

#ifndef GGADGET_FRAMEWORK_SYSTEM_WIRELESS_ACCESS_POINT_INTERFACE_H__
#define GGADGET_FRAMEWORK_SYSTEM_WIRELESS_ACCESS_POINT_INTERFACE_H__

namespace ggadget {

template <typename R, typename P1> class Slot1;

namespace framework {

namespace system {

namespace network {

/** Interface for retrieving the information of the wireless access point. */
class WirelessAccessPointInterface {
 public:
  enum APType {
    APTYPE_INFRASTRUCTURE,
    APTYPE_INDEPENDENT,
    APTYPE_WIRELESS_TYPE_ANY
  };

 public:
  virtual void Destroy() = 0;

 public:
  /** Gets the name of the access point. */
  virtual const char *GetName() const = 0;
  /** Gets the type of the wireless service. */
  virtual APType GetType() const = 0;
  /**
   * Gets the signal strength of the access point, expressed as a
   * percentage.
   */
  virtual int GetSignalStrength() const = 0;
  /**
   * Connects to this access point and, if the @c callback is specified and
   * non-null, calls the method with a boolean status.
   */
  virtual void Connect(Slot1<void, bool> *callback) = 0;
  /**
   * Disconnects from this access point and, if the @c callback is specified
   * and non-null, calls the method with a boolean status.
   */
  virtual void Disconnect(Slot1<void, bool> *callback) = 0;
};

} // namespace network

} // namespace system

} // namespace framework

} // namespace ggadget

#endif // GGADGET_FRAMEWORK_SYSTEM_WIRELESS_ACCESS_POINT_INTERFACE_H__
