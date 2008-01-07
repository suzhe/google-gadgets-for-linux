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

#ifndef GGADGET_DBUS_DBUS_PROXY_H__
#define GGADGET_DBUS_DBUS_PROXY_H__

#include <map>
#include <string>
#include <vector>
#include <ggadget/variant.h>

class DBusConnection;

namespace ggadget {

template <class R> class Slot0;
template <class R, class P1> class Slot1;
template <class R, class P1, class P2> class Slot2;
class MainLoopInterface;

namespace dbus {

class DBusProxy;

typedef std::vector<Variant> VariantList;

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
  MESSAGE_TYPE_STRING,
  MESSAGE_TYPE_ARRAY,
  MESSAGE_TYPE_STRUCT,
  MESSAGE_TYPE_VARIANT,
  MESSAGE_TYPE_DICT
};

/**
 * Factory class of DBusProxy.
 */
class DBusProxyFactory {
 public:
  /**
   * Constructor of DBusProxyFactory.
   * @param main_loop the main loop used in the process. Use @c NULL if user
   *        isn't interested in the asynchronous features offered by the
   *        DBusProxy.
   */
  DBusProxyFactory(MainLoopInterface *main_loop);
  ~DBusProxyFactory();

  /**
   * Generate a proxy using system bus to transfer messages.
   * @param name destination name on the message bus
   * @param path name of the object instance to call methods on
   * @param interface name of the interface to call methods on
   * @param only_talk_to_current_owner use @true if the proxy only want to talk
   *        with current owner of the @c name. If the owner shutdown the
   *        connection no matter why, the proxy will not work any more.
   */
  DBusProxy* NewSystemProxy(const char *name,
                            const char *path,
                            const char *interface,
                            bool only_talk_to_current_owner);
  /**
   * Generate a proxy using session bus to transfer messages.
   * @param name destination name on the message bus
   * @param path name of the object instance to call methods on
   * @param interface name of the interface to call methods on
   * @param only_talk_to_current_owner use @true if the proxy only want to talk
   *        with current owner of the @c name. If the owner shutdown the
   *        connection no matter why, the proxy will not work any more.
   */
  DBusProxy* NewSessionProxy(const char *name,
                             const char *path,
                             const char *interface,
                             bool only_talk_to_current_owner);
 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(DBusProxyFactory);
};

/**
 * DBuse proxy.
 * User should not directly new the proxy. Use DBusProxyFactory instead.
 *
 * All methods have two style: @c va_list parameters and @c Variant vector
 * parameters. The @c va_list style method is prepared for C++ user. For
 * complicated value returned, the caller has responsibility to
 * detach the Variant. For example:
 * <code>
 * DBusProxyFactory factory(mainloop);
 * DBusProxy *proxy = factory.NewSystemProxy("org.freedesktop.DBus",
 *                                           "/org/freedesktop/DBus",
 *                                           "org.freedesktop.DBus",
 *                                           false);
 * Variant array;
 * proxy->SyncCall("ListNames", -1, MESSAGE_TYPE_INVALID,
 *                 MESSAGE_TYPE_ARRAY, &array, MESSAGE_TYPE_INVALID);
 * // doing something...
 * DeallocateContainerVariant(array);
 * </code>
 *
 * The methods that is Variant vector parameter style should not be called by
 * C++ code directly. They are used by JS code.
 */
class DBusProxy {
 public:
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
   * this function will send the request and return instantly without waiting for
   * the back of the reply.
   *
   * @param method method to call
   * @param timeout timeout in milisecond that the caller would
   *        cancel waiting reply, when @c timeout is set to -1,
   *        a sane default time out value will be set by dbus
   * @param first_arg_type type of first input argument
   * @return @c false if an error happen @c true otherwise
   */
  bool SyncCall(const char* method,
                int timeout,
                MessageType first_arg_type,
                ...);
  /**
   * Function for synchronously calling a method and receiving reply values.
   *
   * @param method method to call
   * @param timeout timeout in milisecond that the caller would
   *        cancel waiting reply, when @c timeout is set to -1,
   *        a sane default time out value will be set by dbus
   * @param not_wait_for_reply set to @true if the caller not want to wait for
   *        the reply message and return instantly
   * @param in_arguments arguments array handed in
   * @param count size of @c in_arguments
   * @param out_arguments arguments received from dbus server.
   * @return @c false if an error happen @c true otherwise
   */
  bool SyncCall(const char* method, int timeout,
                bool not_wait_for_reply,
                const Variant *in_arguments, size_t count,
                VariantList *out_arguments);

  /**
   * Asynchronously send out a method call request and return instantly.
   * All of the input arguments are specified first, followed by
   * @c MESSAGE_TYPE_INVALID.
   *
   * @param method method name to call
   * @param slot the call back slot. The proxy will own the slot and delete it.
   *        So caller should new the slot and never delete it.
   * @param first_arg_type type of first input argument
   * @return a call ID by which @c CollectResult could get the reply
   */
  uint32_t AsyncCall(const char* method,
                     Slot1<bool, uint32_t> *slot,
                     MessageType first_arg_type,
                     ...);
  /**
   * Asynchronously send out a method call request and return instantly.
   *
   * @param method method name to call
   * @param slot the call back slot. The proxy will own the slot and delete it.
   *        So caller should new the slot and never delete it.
   * @param in_arguments arguments array handed in
   * @param count size of @c in_arguments
   * @return a call ID by which @c CollectResult could get the reply
   */
  uint32_t AsyncCall(const char* method,
                     Slot1<bool, uint32_t> *slot,
                     const Variant *in_arguments, size_t count);

  /**
   * Method used by collect the reply information of a async call filed by
   * previous invoking of @c AsyncCall.
   * @param call_id the call ID the @c AsyncCall returned
   * @param first_arg_type type of first output argument
   * @return @c false if an error happen @c true otherwise
   */
  bool CollectResult(uint32_t call_id,
                     MessageType first_arg_type,
                     ...);
  /**
   * Method used by collect the reply information of a async call filed by
   * previous invoking of @c AsyncCall.
   * @param call_id the call ID the @c AsyncCall returned
   * @param out_arguments arguments handed out
   * @return @c false if an error happen @c true otherwise
   */
  bool CollectResult(uint32_t call_id,
                     VariantList *out_arguments);

  /**
   * Connect a slot to a signal name that the proxy listen to. When the proxy got
   * such signal, the slot will be invoked.
   * NOTE that the proxy will manage the ownership of the slot.
   * @param signal the signal name
   * @param dbus_signal_slot the slot which will be invoked when a signal come
   */
  void ConnectToSignal(const char *signal, Slot0<void>* dbus_signal_slot);

  /**
   * There is a protocol named Introspectable in DBus protocols. By this
   * mechanism, we could get all methods and signals one interface supported.
   * This function will enumerate all supported method calls.
   * @param slot the callback slot used to handle the enumerated methods. It
   *        will got two parameters, the first is the method call name, the
   *        second is a slot represent that method call, the @slot own the second
   *        parameter. This slot should return @c true if it want to keep
   *        receiving enumerated method
   * @return @c true if everything goes well and @false if the callback @slot
   *         return false or no method calls at all
   */
  bool EnumerateMethods(Slot2<bool, const char*, Slot*> *slot) const;
  /**
   * Similar with @GetMethods.
   * @return @c true if everything goes well and @false if the callback @slot
   *         return false or no method calls at all
   */
  bool EnumerateSignals(Slot2<bool, const char*, Slot*> *slot) const;
 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(DBusProxy);
};

/**
 * Util function.
 * We use @c Variant as a container to hold values transfer between DBus client
 * and server. For complicated values (array, structure and dict), the
 * @c DBusProxy will allocate memory to fill in the container. So the caller
 * need use this method to free them.
 */
void DeallocateContainerVariant(const Variant &container);

}  // namespace dbus
}  // namespace ggadget

#endif  // GGADGET_DBUS_DBUS_PROXY_H__
