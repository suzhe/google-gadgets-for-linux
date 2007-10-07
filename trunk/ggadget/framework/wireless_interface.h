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

#ifndef GGADGET_FRAMEWORK_SYSTEM_WIRELESS_INTERFACE_H__
#define GGADGET_FRAMEWORK_SYSTEM_WIRELESS_INTERFACE_H__

namespace ggadget {

namespace framework {

namespace system {

namespace network {

class WirelessAccessPointInterface;

/** Interface for retrieving the information of the wireless network. */
class WirelessInterface {
 public:
  /** Gets whether the wireless is available. */
  virtual bool IsAvailable() const = 0;
  /** Gets whether the wireless is connected. */
  virtual bool IsConnected() const = 0;
  /** Get whether the enumeration of wireless access points is supported. */
  virtual bool EnumerationSupported() const = 0;

  /**
   * Get the count of the wireless access points.
   */
  virtual int GetAPCount() const = 0;

  /**
   * Get information of an access points.
   * @return the @c WirelessAccessPointInterface pointer,
   *     or @c NULL if the access points don't support enumeration
   *     or index out of range.
   *     The user must call @c Destroy of the returned pointer after use
   *     if the return value is not @c NULL.
   */
  virtual const WirelessAccessPointInterface *GetWirelessAccessPoint(
      int index) const = 0;

  /**
   * Get information of an access points.
   * @return the @c WirelessAccessPointInterface pointer,
   *     or @c NULL if the access points don't support enumeration
   *     or index out of range.
   *     The user must call @c Destroy of the returned pointer after use
   *     if the return value is not @c NULL.
   */
  virtual WirelessAccessPointInterface *GetWirelessAccessPoint(int index) = 0;

  /** Get the name of the wireless adapter. */
  virtual const char *GetName() const = 0;

  /** Get the name of the network. */
  virtual const char *GetNetworkName() const = 0;

  /** Get the wireless connection's signal strength, in percentage. */
  virtual int GetSignalStrength() const = 0;
};

} // namespace network

} // namespace system

} // namespace framework

} // namespace ggadget

#endif // GGADGET_FRAMEWORK_SYSTEM_WIRELESS_INTERFACE_H__
