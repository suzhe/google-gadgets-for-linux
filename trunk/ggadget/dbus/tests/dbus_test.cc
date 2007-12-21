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

#include <stdio.h>
#include <stdlib.h>
#include <dbus/dbus.h>

#include "ggadget/dbus/dbus_utils.h"
#include "ggadget/logger.h"
#include "unittest/gunit.h"

using namespace ggadget;

namespace {

const char* kName        = "com.google.Gadget";
const char* kPath        = "/com/google/Gadget/Test";
const char* kInterface   = "com.google.Gadget.Test";
const char* kDisconnect  = "Disconnected";
const char* kSystemRule  = "type='signal',interface='"DBUS_INTERFACE_LOCAL "'";
const char* kSessionRule = "type='signal',interface='com.google.Gadget.Test'";

int feed = rand();

DBusHandlerResult FilterFunction(DBusConnection *connection,
                                 DBusMessage *message,
                                 void *user_data) {
  DLOG("Get message, type: %d, sender: %s, path: %s, interface: %s",
       dbus_message_get_type(message), dbus_message_get_sender(message),
       dbus_message_get_path(message), dbus_message_get_interface(message));
  if (dbus_message_is_signal(message, DBUS_INTERFACE_LOCAL, kDisconnect)) {
    DLOG("server: got system disconnect signal, exit.");
    exit(0);
    return DBUS_HANDLER_RESULT_HANDLED;
  } else {
    LOG("server: got other message.");
  }
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void path_unregistered_func(DBusConnection *connection,
                            void *user_data) {
  DLOG("server: connection was finalized");
}

DBusHandlerResult handle_echo(DBusConnection *connection,
                              DBusMessage *message) {
  DLOG("server: sending reply to Echo method");
  DBusError error;
  dbus_error_init (&error);
  DBusMessage *reply = dbus_message_new_method_return(message);

  DBusMessageIter out_iter, in_iter;
  dbus_message_iter_init(message, &out_iter);
  dbus_message_iter_init_append(reply, &in_iter);

  int arg_type = dbus_message_iter_get_arg_type(&out_iter);
  switch (arg_type) {
    case DBUS_TYPE_BYTE:
      {
        unsigned char v;
        dbus_message_iter_get_basic(&out_iter, &v);
        dbus_message_iter_append_basic(&in_iter, DBUS_TYPE_BYTE, &v);
        break;
      }
    case DBUS_TYPE_BOOLEAN:
      {
        dbus_bool_t v;
        dbus_message_iter_get_basic(&out_iter, &v);
        dbus_message_iter_append_basic(&in_iter, DBUS_TYPE_BOOLEAN, &v);
        break;
      }
    case DBUS_TYPE_INT16:
      {
        dbus_int16_t v;
        dbus_message_iter_get_basic(&out_iter, &v);
        dbus_message_iter_append_basic(&in_iter, DBUS_TYPE_INT16, &v);
        break;
      }
    case DBUS_TYPE_UINT16:
      {
        dbus_uint16_t v;
        dbus_message_iter_get_basic(&out_iter, &v);
        dbus_message_iter_append_basic(&in_iter, DBUS_TYPE_UINT16, &v);
        break;
      }
    case DBUS_TYPE_INT32:
      {
        dbus_int32_t v;
        dbus_message_iter_get_basic(&out_iter, &v);
        dbus_message_iter_append_basic(&in_iter, DBUS_TYPE_INT32, &v);
        break;
      }
    case DBUS_TYPE_UINT32:
      {
        dbus_uint32_t v;
        dbus_message_iter_get_basic(&out_iter, &v);
        dbus_message_iter_append_basic(&in_iter, DBUS_TYPE_UINT32, &v);
        break;
      }
    case DBUS_TYPE_INT64:
      {
        dbus_int64_t v;
        dbus_message_iter_get_basic(&out_iter, &v);
        dbus_message_iter_append_basic(&in_iter, DBUS_TYPE_INT64, &v);
        break;
      }
    case DBUS_TYPE_UINT64:
      {
        dbus_uint64_t v;
        dbus_message_iter_get_basic(&out_iter, &v);
        dbus_message_iter_append_basic(&in_iter, DBUS_TYPE_UINT64, &v);
        break;
      }
    case DBUS_TYPE_DOUBLE:
      {
        double v;
        dbus_message_iter_get_basic(&out_iter, &v);
        dbus_message_iter_append_basic(&in_iter, DBUS_TYPE_DOUBLE, &v);
        break;
      }
    case DBUS_TYPE_STRING:
      {
        const char* v;
        dbus_message_iter_get_basic(&out_iter, &v);
        dbus_message_iter_append_basic(&in_iter, DBUS_TYPE_STRING, &v);
        break;
      }
    default:
      DLOG("server: unsupported type met: %d", arg_type);
      ASSERT(false);
  }

  if (!dbus_connection_send(connection, reply, NULL))
    DLOG("server: send reply failed: No memory");
  dbus_message_unref(reply);
  return DBUS_HANDLER_RESULT_HANDLED;
}

DBusHandlerResult path_message_func(DBusConnection *connection,
                                    DBusMessage *message,
                                    void *user_data) {
  DLOG("server: handle message.");
  if (dbus_message_is_method_call(message,
                                  kInterface,
                                  "Echo")) {
    return handle_echo(connection, message);
  } else if (dbus_message_is_method_call(message,
                                         kInterface,
                                         kDisconnect)) {
    DLOG("server: received disconnected call from peer.");
    exit(0);
    return DBUS_HANDLER_RESULT_HANDLED;
  } else if (dbus_message_is_method_call(message,
                                         kInterface,
                                         "Hello")) {
    DLOG("server: received Hello message.");
    DBusMessage *reply = dbus_message_new_method_return(message);
    dbus_int32_t rand_feed = *(reinterpret_cast<dbus_int32_t*>(user_data));
    DLOG("server: feed: %d", rand_feed);
    dbus_message_append_args(reply,
                             DBUS_TYPE_INT32, &rand_feed,
                             DBUS_TYPE_INVALID);
    dbus_connection_send(connection, reply, NULL);
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
  }
  DLOG("server: the message was not handled.");
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

DBusObjectPathVTable echo_vtable = {
  path_unregistered_func,
  path_message_func,
  NULL,
};

void StartDBusServer(int feed) {
  DBusError error;
  dbus_error_init(&error);
  DBusConnection *bus;
  bus = dbus_bus_get(DBUS_BUS_SESSION, &error);
  if (!bus) {
    LOG("server: Failed to connect to the D-BUS daemon: %s", error.message);
    dbus_error_free(&error);
    return;
  }
  DLOG("server: name of the connection: %s", dbus_bus_get_unique_name(bus));

  if (!dbus_connection_add_filter(bus, FilterFunction, NULL, NULL))
    LOG("server: add filter failed.");

  dbus_error_init(&error);
  dbus_bus_request_name(bus, kName, 0, &error);
  if (dbus_error_is_set(&error)) {
    DLOG("server: %s: %s", error.name, error.message);
  }

  dbus_bus_add_match(bus, kSystemRule, &error);
  if (dbus_error_is_set(&error)) {
    DLOG("server: %s: %s", error.name, error.message);
  }

  dbus_bus_add_match(bus, kSessionRule, &error);
  if (dbus_error_is_set(&error)) {
    DLOG("server: %s: %s", error.name, error.message);
  }

  int f = feed;  /** to avoid user pass a number in */
  if (!dbus_connection_register_object_path(bus, kPath, &echo_vtable,
                                            (void*)&f))
    DLOG("server: register failed.");

  while (dbus_connection_read_write_dispatch(bus, -1))
    ;

  return;
}

std::string GetOwner(BusType type,
                     const char* name) {
  DBusConnection* connection;
  DBusError error;
  dbus_error_init(&error);

  if (type == BUS_TYPE_SYSTEM) {
    connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
  } else if (type == BUS_TYPE_SESSION) {
    connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
  } else {
    LOG("The type is not supported.");
    return "";
  }
  DBusMessage *message = dbus_message_new_method_call(DBUS_SERVICE_DBUS,
                                                      DBUS_PATH_DBUS,
                                                      DBUS_INTERFACE_DBUS,
                                                      "GetNameOwner");
  dbus_message_append_args(message,
                           DBUS_TYPE_STRING, &name,
                           DBUS_TYPE_INVALID);
  DBusMessage *reply = dbus_connection_send_with_reply_and_block(connection,
                                                                 message,
                                                                 2000,
                                                                 &error);
  const char* base_name;
  dbus_message_get_args(reply, &error, DBUS_TYPE_STRING, &base_name,
                        DBUS_TYPE_INVALID);
  return std::string(base_name);
}

}  // anonymous namespace

TEST(DBusProxy, SyncCall) {
  int id = fork();
  if (id == 0) {
    DLOG("server start");
    StartDBusServer(feed);
    exit(0);
  } else {
    sleep(1);  /** wait server start */
    DLOG("client start");
    DBusProxy *proxy = new DBusProxy(BUS_TYPE_SESSION, kName,
                                     kPath, kInterface);
    int read = 0;
    EXPECT_TRUE(proxy->SyncCall("Hello", -1, MESSAGE_TYPE_INVALID,
                                MESSAGE_TYPE_INT32, &read,
                                MESSAGE_TYPE_INVALID));
    DLOG("read feed: %d", read);
    EXPECT_EQ(feed, read);
    delete proxy;
  }
}

TEST(DBusProxy, SyncCall_ForOwner) {
  std::string name = GetOwner(BUS_TYPE_SESSION, kName);
  DLOG("client: Owner name of the server: %s", name.c_str());
  DBusProxy *proxy = new DBusProxy(BUS_TYPE_SESSION, name.c_str(),
                                   kPath, kInterface);
  int read = 0;
  proxy->SyncCall("Hello", -1, MESSAGE_TYPE_INVALID,
                  MESSAGE_TYPE_INT32, &read,
                  MESSAGE_TYPE_INVALID);
  DLOG("read feed: %d", read);
  EXPECT_EQ(feed, read);
  delete proxy;
}

TEST(DBusProxy, SendSignal) {
  DBusProxy *proxy = new DBusProxy(BUS_TYPE_SESSION, kName,
                                   kPath, kInterface);
  // close server
  DLOG("client: sent close signal.");
  EXPECT_TRUE(proxy->SyncCall(kDisconnect, -1,
                              MESSAGE_TYPE_INVALID, MESSAGE_TYPE_INVALID));
  int read = 0;
  EXPECT_FALSE(proxy->SyncCall("Hello", -1, MESSAGE_TYPE_INVALID,
                               MESSAGE_TYPE_INT32, &read,
                               MESSAGE_TYPE_INVALID));
  delete proxy;
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);
  int result = RUN_ALL_TESTS();
  return result;
}
