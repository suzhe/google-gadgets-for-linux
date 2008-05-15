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

#ifndef GGADGET_DBUS_DBUS_PROXY_H__
#define GGADGET_DBUS_DBUS_PROXY_H__

#include <map>
#include <string>
#include <vector>
#include <ggadget/variant.h>

class DBusConnection;

namespace ggadget {

template <typename R> class Slot0;
template <typename R, typename P1, typename P2> class Slot2;
class MainLoopInterface;

namespace dbus {

class DBusProxy;

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
 * parameters. The @c va_list style method is prepared for C++ user.
 * Usage exsample:
 * <code>
 * class IntValue {
 *  public:
 *   IntValue() : value_(0) {
 *   }
 *   ~IntValue() {}
 *   bool Callback(int id, const Variant &value) {
 *     DLOG("expect receiving a int type.");
 *     ASSERT(value.type() == Variant::TYPE_INT64);
 *     int64_t v = VariantValue<int64_t>()(value);
 *     value_ = static_cast<int>(v);
 *     return true;
 *   }
 *   int value() const { return value_; }
 *  private:
 *   int value_;
 * };
 * DBusProxyFactory factory(mainloop);
 * DBusProxy *proxy = factory.NewSystemProxy("org.freedesktop.DBus",
 *                                           "/org/freedesktop/DBus",
 *                                           "org.freedesktop.DBus",
 *                                           false);
 * IntValue obj;
 * proxy->Call("DummyMethod", true, -1,
 *             NewSlot(&obj, &IntValue::Callback),
 *             MESSAGE_TYPE_INVALID);
 * cout << "returned int value: " << obj.value();
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
   * @param mainloop the main loop to which the proxy attach
   * @param name any name on the message bus
   * @param path name of the object instance to call methods on
   * @param interface name of the interface to call methods on
   */
  DBusProxy(DBusConnection *connection,
            MainLoopInterface *mainloop,
            const char* name,
            const char* path,
            const char* interface);
  ~DBusProxy();

  /**
   * Callback slot to receive values from the DBus server. The callback will
   * return a @c bool: @true if it want to keep receiving next argument and
   * @false otherwise. The first parameter of the callback indecate the index of
   * current argument, and the second parameter is the value of current argument.
   */
  typedef Slot2<bool, int, const Variant&> ResultCallback;

  /**
   * Function for calling a method and receiving reply values.
   * All of the input arguments are specified first,
   * followed by @c MESSAGE_TYPE_INVALID.
   *
   * @param method method name to call
   * @param sync @c true if the caller want to block this method and wait for
   *        reply and @c false otherwise.
   * @param timeout timeout in milisecond that the caller would
   *        cancel waiting reply. When @c timeout is set to -1,
   *        for sync case, sane default time out value will be set by dbus,
   *        and for async case, callback will always there until a reply back
   * @param callback callback to receive the arguments returned from DBus server
   *        Note that the @c Call will own the callback and delete it after
   *        execute. If it is set to @c NULL, the method will not wait for the
   *        reply.
   * @param first_arg_type type of first input argument
   * @return @c false if an error happen @c true otherwise
   */
  bool Call(const char* method,
            bool sync,
            int timeout,
            ResultCallback *callback,
            MessageType first_arg_type,
            ...);

  /**
   * Function for calling a method and receiving reply values.
   *
   * @param method method name to call
   * @param sync @c true if the caller want to block this method and wait for
   *        reply and @c false otherwise.
   * @param timeout timeout in milisecond that the caller would
   *        cancel waiting reply. When @c timeout is set to -1,
   *        for sync case, sane default time out value will be set by dbus,
   *        and for async case, callback will always there until a reply back
   * @param in_arguments arguments array handed in
   * @param count size of @c in_arguments
   * @param callback callback to receive the arguments returned from DBus server
   *        Note that the @c Call will own the callback and delete it after
   *        execute. If it is set to @c NULL, the method will not wait for the
   *        reply.
   * @return @c false if an error happen @c true otherwise
   */
  bool Call(const char* method,
            bool sync,
            int timeout,
            const Variant *in_arguments, size_t count,
            ResultCallback *callback);

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

}  // namespace dbus
}  // namespace ggadget

#endif  // GGADGET_DBUS_DBUS_PROXY_H__
