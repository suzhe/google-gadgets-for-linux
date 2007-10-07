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

#ifndef GGADGET_FRAMEWORK_SYSTEM_NETWORK_INTERFACE_H__
#define GGADGET_FRAMEWORK_SYSTEM_NETWORK_INTERFACE_H__

namespace ggadget {

namespace framework {

namespace system {

namespace network {

class WirelessInterface;

} // namespace network

/** Interface for retrieving the information about the network. */
class NetworkInterface {
 public:
  /** The network connection type. */
  enum ConnectionType {
    /** Ethernet 802.3. */
    CONNECTION_TYPE_802_3,
    /** Token Ring 802.5. */
    CONNECTION_TYPE_802_5,
    /** FDDI. */
    CONNECTION_TYPE_FDDI,
    /** WAN. */
    CONNECTION_TYPE_WAN,
    /** Local Talk. */
    CONNECTION_TYPE_LOCAL_TALK,
    /** DIX. */
    CONNECTION_TYPE_DIX,
    /** ARCNET (raw). */
    CONNECTION_TYPE_ARCNET_RAW,
    /** ARCNET 878.2. */
    CONNECTION_TYPE_ARCNET_878_2,
    /** ATM. */
    CONNECTION_TYPE_ATM,
    /** NDIS wireless. */
    CONNECTION_TYPE_WIRELESS_WAN,
    /** Infrared (IrDA). */
    CONNECTION_TYPE_IRDA,
    /** Broadcast PC. */
    CONNECTION_TYPE_BPC,
    /** Connection-oriented WAN. */
    CONNECTION_TYPE_CO_WAN,
    /** IEEE 1394 (firewire) bus. */
    CONNECTION_TYPE_1394,
    /** InfiniBand. */
    CONNECTION_TYPE_INFINI_BAND,
  };

  /** The network connection physical media type. */
  enum ConnectionMediaType {
    /** None of the following. */
    CONNECTION_MEDIA_TYPE_UNSPECIFIED,
    /**
     * A wireless LAN network through a miniport driver that conforms to the
     * 802.11 interface.
     */
    CONNECTION_MEDIA_TYPE_WIRELESS_LAN,
    /** A DOCSIS-based cable network. */
    CONNECTION_MEDIA_TYPE_CABLE_MODEM,
    /** Standard phone lines. */
    CONNECTION_MEDIA_TYPE_PHONE_LINE,
    /** Wiring that is connected to a power distribution system. */
    CONNECTION_MEDIA_TYPE_POWER_LINE,
    /** A Digital Subscriber line (DSL) network. */
    CONNECTION_MEDIA_TYPE_DSL,
    /** A Fibre Channel interconnect. */
    CONNECTION_MEDIA_TYPE_FIBRE_CHANNEL,
    /** An IEEE 1394 (firewire) bus. */
    CONNECTION_MEDIA_TYPE_1394,
    /** A Wireless WAN link. */
    CONNECTION_MEDIA_TYPE_WIRELESS_WAN,
  };

 public:
  /** Detects whether the network connection is on. */
  virtual bool IsOnline() const = 0;
  /** Gets the type of the connection. */
  virtual ConnectionType GetConnectionType() const = 0;
  /** Gets the type of the physical media. */
  virtual ConnectionMediaType GetPhysicalMediaType() const = 0;
};

} // namespace system

} // namespace framework

} // namespace ggadget

#endif // GGADGET_FRAMEWORK_SYSTEM_NETWORK_INTERFACE_H__
