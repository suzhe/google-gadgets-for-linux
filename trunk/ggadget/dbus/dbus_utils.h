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

#ifndef GGADGET_DBUS_UTILS_H__
#define GGADGET_DBUS_UTILS_H__

#include <string>

class DBusConnection;

namespace ggadget {

enum BusType {
  /** system bus */
  BUS_TYPE_SYSTEM,
  /** session bus */
  BUS_TYPE_SESSION,
  BUS_TYPE_INVALID
};

enum MessageType {
  MESSAGE_TYPE_INVALID,
  MESSAGE_TYPE_BYTE,
  MESSAGE_TYPE_BOOLEAN,
  MESSAGE_TYPE_INT16,
  MESSAGE_TYPE_UINT16,
  MESSAGE_TYPE_INT32,
  MESSAGE_TYPE_UINT32,
  MESSAGE_TYPE_INT64,
  MESSAGE_TYPE_UINT64,
  MESSAGE_TYPE_DOUBLE,
  MESSAGE_TYPE_STRING
};

/**
 * DBuse proxy, something like glib DBus proxy
 */
class DBusProxy {
 public:
  class DBusProxyImp;
 public:
  /**
   * C-tor.
   * @param type the bus type the proxy want to connect
   * @param name any name on the message bus
   * @param path name of the object instance to call methods on
   * @param interface name of the interface to call methods on
   */
  DBusProxy(BusType type,
            const char* name,
            const char* path,
            const char* interface);
  /**
   * C-tor.
   * @param connection the connection to the remote bus
   * @param name any name on the message bus
   * @param path name of the object instance to call methods on
   * @param interface name of the interface to call methods on
   */
  DBusProxy(DBusConnection *connection,
            const char* name,
            const char* path,
            const char* interface);
  ~DBusProxy();

  /**
   * Function for synchronously calling a method and receiving reply values.
   * All of the input arguments are specified first,
   * followed by @c MESSAGE_TYPE_INVALID.
   * Note that this means the @c MESSAGE_TYPE_INVALID
   * must always spcified twice. And if no output arguments are specified,
   * this function will not keep block to read the reply.
   *
   * @param method method to call
   * @param timeout timeout in milisecond that the caller would cancel waiting reply, when @c timeout is set to -1, a sane default time out value will be set by dbus
   * @param first_arg_type type of first input argument
   * @return @c false if an error set @c true otherwise
   */
  bool SyncCall(const char* method,
                int timeout,
                MessageType first_arg_type,
                ...);
 private:
  DBusProxyImp *impl_;
};

}  // namespace ggadget

#endif  // GGADGET_DBUS_UTILS_H__
