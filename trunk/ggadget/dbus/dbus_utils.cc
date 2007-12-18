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

#include <set>
#include <map>

#include <dbus/dbus.h>

#include "dbus_utils.h"
#include <ggadget/common.h>
#include <ggadget/string_utils.h>

namespace ggadget {

namespace {

typedef std::set<DBusProxy::DBusProxyImp*> DBusProxySet;
typedef std::map<std::string, DBusProxySet*> DBusProxiesMap;

const struct {
  MessageType type;
  int dbus_type;
} typies[] = {
  { MESSAGE_TYPE_INVALID, DBUS_TYPE_INVALID },
  { MESSAGE_TYPE_BYTE,    DBUS_TYPE_BYTE    },
  { MESSAGE_TYPE_BOOLEAN, DBUS_TYPE_BOOLEAN },
  { MESSAGE_TYPE_INT16,   DBUS_TYPE_INT16   },
  { MESSAGE_TYPE_UINT16,  DBUS_TYPE_UINT16  },
  { MESSAGE_TYPE_INT32,   DBUS_TYPE_INT32   },
  { MESSAGE_TYPE_UINT32,  DBUS_TYPE_UINT32  },
  { MESSAGE_TYPE_INT64,   DBUS_TYPE_INT64   },
  { MESSAGE_TYPE_UINT64,  DBUS_TYPE_UINT64  },
  { MESSAGE_TYPE_DOUBLE,  DBUS_TYPE_DOUBLE  },
  { MESSAGE_TYPE_STRING,  DBUS_TYPE_STRING  }
};
const int TypeMapSize = sizeof(typies) / sizeof(typies[0]);

int MessageTypeToDBusType(MessageType type) {
  /** TODO: more effient way is needed. */
  for (int i = 0; i < TypeMapSize; ++i)
    if (typies[i].type == type)
      return typies[i].dbus_type;
  return DBUS_TYPE_INVALID;
}

bool EncodeMessage(DBusMessage* message,
                   MessageType first_arg_type,
                   va_list *args) {
  DBusMessageIter iter;
  dbus_message_iter_init_append(message, &iter);
  MessageType type = first_arg_type;
  while (type != MESSAGE_TYPE_INVALID) {
    switch (type) {
      case MESSAGE_TYPE_BYTE:
        {
          unsigned char v = va_arg(*args, int);
          dbus_message_iter_append_basic(&iter, DBUS_TYPE_BYTE, &v);
          break;
        }
      case MESSAGE_TYPE_BOOLEAN:
        {
          dbus_bool_t v = va_arg(*args, dbus_bool_t);
          dbus_message_iter_append_basic(&iter, DBUS_TYPE_BOOLEAN, &v);
          break;
        }
      case MESSAGE_TYPE_INT16:
        {
          dbus_int16_t v = va_arg(*args, int);
          dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT16, &v);
          break;
        }
      case MESSAGE_TYPE_UINT16:
        {
          dbus_uint16_t v = va_arg(*args, int);
          dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT16, &v);
          break;
        }
      case MESSAGE_TYPE_INT32:
        {
          dbus_int32_t v = va_arg(*args, dbus_int32_t);
          dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &v);
          break;
        }
      case MESSAGE_TYPE_UINT32:
        {
          dbus_uint32_t v = va_arg(*args, dbus_uint32_t);
          dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT32, &v);
          break;
        }
      case MESSAGE_TYPE_INT64:
        {
          dbus_int64_t v = va_arg(*args, dbus_int64_t);
          dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT64, &v);
          break;
        }
      case MESSAGE_TYPE_UINT64:
        {
          dbus_uint64_t v = va_arg(*args, dbus_uint64_t);
          dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT64, &v);
          break;
        }
      case MESSAGE_TYPE_DOUBLE:
        {
          double v = va_arg(*args, double);
          dbus_message_iter_append_basic(&iter, DBUS_TYPE_DOUBLE, &v);
          break;
        }
      case MESSAGE_TYPE_STRING:
        {
          const char* v = va_arg(*args, const char*);
          dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &v);
          break;
        }
      default:
        LOG("other types has not been supported yet!");
        ASSERT(false);
        return false;
    }
    type = static_cast<MessageType>(va_arg(*args, int));
  }
  return true;
}

bool DecodeMessage(DBusMessage *message,
                   MessageType first_arg_type,
                   va_list* args) {
  DBusMessageIter iter;
  dbus_message_iter_init(message, &iter);
  MessageType type = first_arg_type;
  while (type != MESSAGE_TYPE_INVALID) {
    int arg_type = dbus_message_iter_get_arg_type(&iter);
    if (arg_type != MessageTypeToDBusType(type)) {
      LOG("type dismatch! the type in message is %d, "
          " but in this function it is %d", arg_type, type);
      return false;
    }
    if (arg_type == DBUS_TYPE_INVALID) {
      LOG("Too few arguments in reply.");
      return false;
    }
    void * return_storage = va_arg(*args, void*);
    if (return_storage) {
      switch (type) {
        case MESSAGE_TYPE_BOOLEAN:
          {
            dbus_bool_t v;
            dbus_message_iter_get_basic(&iter, &v);
            memcpy(return_storage, &v, sizeof(dbus_bool_t));
            break;
          }
        case MESSAGE_TYPE_BYTE:
          {
            unsigned char v;
            dbus_message_iter_get_basic(&iter, &v);
            memcpy(return_storage, &v, sizeof(unsigned char));
            break;
          }
        case MESSAGE_TYPE_INT32:
          {
            dbus_int32_t v;
            dbus_message_iter_get_basic(&iter, &v);
            memcpy(return_storage, &v, sizeof(dbus_int32_t));
            break;
          }
        case MESSAGE_TYPE_UINT32:
          {
            dbus_uint32_t v;
            dbus_message_iter_get_basic(&iter, &v);
            memcpy(return_storage, &v, sizeof(dbus_uint32_t));
            break;
          }
        case MESSAGE_TYPE_INT64:
          {
            dbus_int64_t v;
            dbus_message_iter_get_basic(&iter, &v);
            memcpy(return_storage, &v, sizeof(dbus_int64_t));
            break;
          }
        case MESSAGE_TYPE_UINT64:
          {
            dbus_uint64_t v;
            dbus_message_iter_get_basic(&iter, &v);
            memcpy(return_storage, &v, sizeof(dbus_uint64_t));
            break;
          }
        case MESSAGE_TYPE_DOUBLE:
          {
            double v;
            dbus_message_iter_get_basic(&iter, &v);
            memcpy(return_storage, &v, sizeof(double));
            break;
          }
        case MESSAGE_TYPE_INT16:
          {
            dbus_int16_t v;
            dbus_message_iter_get_basic(&iter, &v);
            memcpy(return_storage, &v, sizeof(dbus_int16_t));
            break;
          }
        case MESSAGE_TYPE_UINT16:
          {
            dbus_uint16_t v;
            dbus_message_iter_get_basic(&iter, &v);
            memcpy(return_storage, &v, sizeof(dbus_uint16_t));
            break;
          }
        case MESSAGE_TYPE_STRING:
          {
            const char* s;
            dbus_message_iter_get_basic(&iter, &s);
            memcpy(return_storage, &s, sizeof(const char*));
            break;
          }
        default:
          LOG("the type %d is not supported yet!", type);
          return false;
      }
    }
    dbus_message_iter_next(&iter);
    type = static_cast<MessageType>(va_arg(*args, int));
  }
  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_INVALID) {
    LOG("Too many arguments in reply.");
    return false;
  }
  return true;
}

}  // anonymous namespace

/**
 * @c DBusProxyManager's is used to route signals to the proxies
 * whose signals are emitted on. It also track the owners of the names
 * that proxies are bound to.
 * Note, this class is not thread-safe.
 */
class DBusProxyManager {
 public:
  explicit DBusProxyManager(DBusConnection* connection) :
      connection_(connection) {
    DLOG("create proxy manager for connection: %p", connection);
    dbus_connection_ref(connection_);
    AddFilter();
  }
  ~DBusProxyManager() {
    dbus_connection_unref(connection_);
  }

  void RegisterProxy(DBusProxy::DBusProxyImp* proxy);
  void UnRegisterProxy(DBusProxy::DBusProxyImp* proxy);

 private:
  static DBusHandlerResult ManagerFilter(DBusConnection *connection,
                                         DBusMessage *message,
                                         void *user_data);
  void AddFilter();
 private:
  /** The message bus with which the manager attaches. */
  DBusConnection *connection_;
  /** Proxy lists used to route incoming signals. */
  DBusProxiesMap proxies_map_;
};

typedef void (*DBusProxyCallBack)(DBusProxy::DBusProxyImp* proxy,
                                  uint32_t call_id,
                                  void* user_data);
struct DBusProxyPendingCall {
  DBusProxy::DBusProxyImp* proxy;
  uint32_t call_id;
  DBusProxyCallBack call_back;
  DBusPendingCall *pending_call;
  void *user_data;

  /** Default C-tor */
  DBusProxyPendingCall(DBusProxy::DBusProxyImp* p,
                       uint32_t id,
                       DBusProxyCallBack cb,
                       DBusPendingCall *pending,
                       void* data) :
    proxy(p), call_id(id), call_back(cb),
    pending_call(pending), user_data(data) {}
};
typedef std::map<uint32_t, DBusProxyPendingCall*> DBusPendingCallsMap;

class DBusProxy::DBusProxyImp {
 public:
  DBusProxyImp(DBusConnection* connection,
               const char* name,
               const char* path,
               const char* interface);
  ~DBusProxyImp();

 public:
  bool SyncCall(const char* method, int timeout,
                MessageType first_arg_type, va_list *args);
  uint32_t PendingCall(const char* method,
                       int timeout,
                       DBusProxyCallBack call_back,
                       void* user_data,
                       MessageType first_arg_type,
                       va_list *args);
  bool CollectPendingCallResult(uint32_t call_id,
                                MessageType first_arg_type,
                                va_list *args);

 public:
  std::string signature() const { return signature_; }
  std::string MatchRule() const;

 private:
  DBusProxyManager* GetManager();
  static void InvokeCallBack(DBusPendingCall *call, void *user_data);
  static void RemoveCall(void *user_data);
 private:
  std::string name_;
  std::string path_;
  std::string interface_;
  /** Used to identify the proxy. */
  std::string signature_;

  uint32_t caller_counter_;
  DBusPendingCallsMap pending_calls_;
  DBusConnection *connection_;

  static dbus_int32_t manager_slot_;
};
dbus_int32_t DBusProxy::DBusProxyImp::manager_slot_ = -1;

void DBusProxyManager::RegisterProxy(DBusProxy::DBusProxyImp* proxy) {
  DBusProxiesMap::iterator it = proxies_map_.find(proxy->signature());
  if (it == proxies_map_.end()) {
    DBusProxySet *list = new DBusProxySet;
    proxies_map_[proxy->signature()] = list;
    dbus_bus_add_match(connection_, proxy->MatchRule().c_str(), NULL);
  }
  proxies_map_[proxy->signature()]->insert(proxy);
}

void DBusProxyManager::UnRegisterProxy(DBusProxy::DBusProxyImp* proxy) {
  DBusProxiesMap::iterator map_iter = proxies_map_.find(proxy->signature());
  if (map_iter == proxies_map_.end()) {
    DLOG("this kind of proxies was not registered before.");
    return;
  }

  DBusProxySet::iterator set_iter = (*map_iter).second->find(proxy);
  if (set_iter == (*map_iter).second->end()) {
    DLOG("this proxy was not registered before.");
    return;
  }
  (*map_iter).second->erase(set_iter);
}

void DBusProxyManager::AddFilter() {
  dbus_connection_add_filter(connection_, ManagerFilter, this, NULL);
}

DBusHandlerResult DBusProxyManager::ManagerFilter(DBusConnection *connection,
                                                  DBusMessage *message,
                                                  void *user_data) {
  DBusProxyManager *this_p = reinterpret_cast<DBusProxyManager*>(user_data);

  if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_SIGNAL)
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

  if (dbus_message_is_signal(message, DBUS_INTERFACE_LOCAL, "Disconnected")) {
    delete this_p;
  }

  /** This signal is globaly useful, do not return other value
   * to stop other client listening this signal.
   */
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

DBusProxy::DBusProxyImp::DBusProxyImp(DBusConnection* connection,
                                      const char* name,
                                      const char* path,
                                      const char* interface) :
    caller_counter_(0), connection_(connection) {
  if (name) name_ = name;
  if (path) path_ = path;
  if (interface) interface_ = interface;
  signature_ = name;
  signature_ += "\n";
  signature_ += path;
  signature_ += "\n";
  signature_ += interface;
  DBusProxyManager* manager = GetManager();
  manager->RegisterProxy(this);
}

DBusProxy::DBusProxyImp::~DBusProxyImp() {
  DBusProxyManager* manager = GetManager();
  manager->UnRegisterProxy(this);
}

bool DBusProxy::DBusProxyImp::SyncCall(const char* method,
                                       int timeout,
                                       MessageType first_arg_type,
                                       va_list *args) {
  ASSERT(method);
  uint32_t call_id = PendingCall(method, timeout, NULL, NULL,
                                 first_arg_type, args);
  first_arg_type = static_cast<MessageType>(va_arg(*args, int));
  if (first_arg_type == MESSAGE_TYPE_INVALID) {
    DLOG("no output argument, do not collect pending result.");
    return true;
  }
  return CollectPendingCallResult(call_id, first_arg_type, args);
}

uint32_t DBusProxy::DBusProxyImp::PendingCall(const char* method,
                                              int timeout,
                                              DBusProxyCallBack call_back,
                                              void* user_data,
                                              MessageType first_arg_type,
                                              va_list *args) {
  DBusMessage *message = dbus_message_new_method_call(name_.c_str(),
                                                      path_.c_str(),
                                                      interface_.c_str(),
                                                      method);
  if (!message) {
    LOG("create message failed.");
    return 0;
  }

  if (!EncodeMessage(message, first_arg_type, args)) {
    LOG("failed to encode message.");
    dbus_message_unref(message);
    return 0;
  }

  DBusPendingCall *pending;
  dbus_connection_send_with_reply(connection_,
                                  message,
                                  &pending,
                                  timeout);
  dbus_message_unref(message);
  ASSERT(pending);
  uint32_t id = ++caller_counter_;
  DBusProxyPendingCall *call = new DBusProxyPendingCall(this, id, call_back,
                                                        pending, user_data);
  pending_calls_[id] = call;
  dbus_pending_call_set_notify(pending, InvokeCallBack, call, RemoveCall);
  return id;
}

bool
DBusProxy::DBusProxyImp::CollectPendingCallResult(uint32_t call_id,
                                                  MessageType first_arg_type,
                                                  va_list *args) {
  DBusPendingCallsMap::iterator it = pending_calls_.find(call_id);
  if (it == pending_calls_.end()) {
    LOG("the call with id %d was not in the pending list or has been removed.",
        call_id);
    return false;
  }
  dbus_pending_call_block((*it).second->pending_call);
  DBusMessage *reply =
      dbus_pending_call_steal_reply((*it).second->pending_call);
  delete (*it).second;
  pending_calls_.erase(it);

  bool result = false;
  switch (dbus_message_get_type(reply)) {
    case DBUS_MESSAGE_TYPE_METHOD_RETURN:
      result = DecodeMessage(reply, first_arg_type, args);
    case DBUS_MESSAGE_TYPE_ERROR:
    default:
      {
        DBusError error;
        dbus_error_init(&error);
        dbus_set_error_from_message(&error, reply);
        if (dbus_error_is_set(&error))
          LOG("error: %s: %s", error.name, error.message);
        dbus_error_free(&error);
      }
  }

  if (reply) dbus_message_unref(reply);
  return result;
}

inline std::string DBusProxy::DBusProxyImp::MatchRule() const {
  if (name_.empty()) {
    return StringPrintf("type='signal',path='%s',interface='%s'",
                        path_.c_str(), interface_.c_str());
  } else {
    return StringPrintf("type='signal',sneder='%s',path='%s',interface='%s'",
                        name_.c_str(), path_.c_str(), interface_.c_str());
  }
  return "";
}

DBusProxyManager*
DBusProxy::DBusProxyImp::GetManager() {
  dbus_connection_allocate_data_slot(&manager_slot_);
  if (manager_slot_ < 0) {
    LOG("out of memeory.");
    return NULL;
  }

  DBusProxyManager* manager = reinterpret_cast<DBusProxyManager*>(
      dbus_connection_get_data(connection_, manager_slot_));
  if (!manager) {
    manager = new DBusProxyManager(connection_);
    dbus_connection_set_data(connection_, manager_slot_, manager, NULL);
  } else {
    dbus_connection_free_data_slot(&manager_slot_);
  }
  return manager;
}

void DBusProxy::DBusProxyImp::InvokeCallBack(DBusPendingCall *call,
                                             void *user_data) {
  DBusProxyPendingCall *pending =
      reinterpret_cast<DBusProxyPendingCall*>(user_data);
  if (pending->call_back != NULL)
    (*pending->call_back)(pending->proxy, pending->call_id, user_data);
}

void DBusProxy::DBusProxyImp::RemoveCall(void *user_data) {
  DBusProxyPendingCall *call =
      reinterpret_cast<DBusProxyPendingCall*>(user_data);
  DBusPendingCallsMap *map = &call->proxy->pending_calls_;
  DBusPendingCallsMap::iterator it = map->find(call->call_id);
  ASSERT(it != call->proxy->pending_calls_.end());
  delete call;
  map->erase(it);
}

DBusProxy::DBusProxy(BusType type,
                     const char* name,
                     const char* path,
                     const char* interface) : impl_(NULL) {
  DBusConnection* connection;
  DBusError error;
  dbus_error_init(&error);

  if (type == BUS_TYPE_SYSTEM) {
    connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
  } else if (type == BUS_TYPE_SESSION) {
    connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
  } else {
    LOG("The type is not supported.");
    return;
  }
  if (!connection) {
    LOG("error: %s: %s", error.name, error.message);
  } else {
    impl_ = new DBusProxyImp(connection, name, path, interface);
  }
  dbus_error_free(&error);
}

DBusProxy::DBusProxy(DBusConnection* connection,
                     const char* name,
                     const char* path,
                     const char* interface) : impl_(NULL) {
  ASSERT(connection);
  impl_ = new DBusProxyImp(connection, name, path, interface);
}

DBusProxy::~DBusProxy() {
  delete impl_;
}

bool DBusProxy::SyncCall(const char* method,
                         int timeout,
                         MessageType first_arg_type,
                         ...) {
  if (!impl_) return false;

  va_list args;
  va_start(args, first_arg_type);
  bool ret = impl_->SyncCall(method, timeout, first_arg_type, &args);
  va_end(args);
  return ret;
}

}  // namespace ggadget
