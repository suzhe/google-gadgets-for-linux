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

#include <map>
#include <dbus/dbus.h>

#include "dbus_proxy.h"
#include "dbus_utils.h"
#include <ggadget/common.h>
#include <ggadget/logger.h>
#include <ggadget/slot.h>
#include <ggadget/scriptable_array.h>
#include <ggadget/string_utils.h>
#include <ggadget/xml_dom_interface.h>
#include <ggadget/xml_parser_interface.h>
#include <ggadget/xml_parser.h>

namespace ggadget {
namespace dbus {

namespace {

const char *kIntrospectInterface = "org.freedesktop.DBus.Introspectable";
const char *kIntrospectMethod    = "Introspect";

void VariantListToArguments(const Variant *list_start, size_t count,
                            Arguments *args) {
  Arguments tmp;
  for (size_t i = 0; i < count; ++i)
    tmp.push_back(Argument(*list_start++));
  args->swap(tmp);
}

void ArgumentsToVariantList(const Arguments &args, VariantList *list) {
  VariantList tmp;
  for (Arguments::const_iterator it = args.begin(); it != args.end(); ++it)
    tmp.push_back(it->value);
  list->swap(tmp);
}

void DeallocateContainerArugments(const Arguments &args) {
  for (Arguments::const_iterator it = args.begin(); it != args.end(); ++it)
    DeallocateContainerVariant(it->value);
}

// Recursively deallocate the Variant container
class ContainerFreeMan {
 public:
  bool ArrayCallback(int id, const Variant &value) {
    DeallocateContainerVariant(value);
    return true;
  }
  bool PropertyCallback(int id, const char *name,
                        const Variant &value, bool is_method) {
    if (!is_method) DeallocateContainerVariant(value);
    return true;
  }
};

}  /** anonymous namespace */

class DBusProxyFactory::Impl {
 public:
  Impl(MainLoopInterface *main_loop) :
    main_loop_(main_loop),
    system_bus_(NULL), session_bus_(NULL),
    system_bus_closure_(NULL), session_bus_closure_(NULL) {
  }
  ~Impl() {
    if (system_bus_) {
      if (main_loop_) {
        delete system_bus_closure_;
        dbus_connection_close(system_bus_);
      }
      dbus_connection_unref(system_bus_);
    }
    if (session_bus_) {
      if (main_loop_) {
        delete session_bus_closure_;
        dbus_connection_close(session_bus_);
      }
      dbus_connection_unref(session_bus_);
    }
  }

  DBusProxy* NewSystemProxy(const char *name,
                            const char *path,
                            const char *interface,
                            bool by_owner) {
    if (!system_bus_) {
      system_bus_ = GetBus(true);
      if (main_loop_)
        system_bus_closure_ = new DBusMainLoopClosure(system_bus_, main_loop_);
    }
    if (by_owner)
      name = GetOwner(true, name).c_str();
    return new DBusProxy(system_bus_, name, path, interface);
  }

  DBusProxy* NewSessionProxy(const char *name,
                             const char *path,
                             const char *interface,
                             bool by_owner) {
    if (!session_bus_) {
      session_bus_ = GetBus(false);
      if (main_loop_)
        session_bus_closure_ =
            new DBusMainLoopClosure(session_bus_, main_loop_);
    }
    if (by_owner)
      name = GetOwner(false, name).c_str();
    return new DBusProxy(session_bus_, name, path, interface);
  }
 private:
  DBusConnection* GetBus(bool system_bus) {
    DBusError error;
    dbus_error_init(&error);
    DBusConnection *connection = NULL;
    if (system_bus) {
      connection = GetDBusBus(DBUS_BUS_SYSTEM, &error);
    } else {
      connection = GetDBusBus(DBUS_BUS_SESSION, &error);
    }
    if (dbus_error_is_set(&error))
      LOG("error: %s, %s", error.name, error.message);
    dbus_error_free(&error);
    return connection;
  }
  DBusConnection* GetDBusBus(DBusBusType type, DBusError *error) {
    /**
     * if the main_loop_ is set, we should use private bus so that any
     * main_loop-related configuration will not affect default bus.
     */
    if (main_loop_)
      return dbus_bus_get_private(type, error);
    return dbus_bus_get(type, error);
  }
  std::string GetOwner(bool system_bus,
                       const char* name) {
    DBusMessage *message = dbus_message_new_method_call(DBUS_SERVICE_DBUS,
                                                        DBUS_PATH_DBUS,
                                                        DBUS_INTERFACE_DBUS,
                                                        "GetNameOwner");
    dbus_message_append_args(message, DBUS_TYPE_STRING,
                             &name, DBUS_TYPE_INVALID);
    DBusConnection *bus = system_bus ? system_bus_ : session_bus_;
    DBusError error;
    dbus_error_init(&error);
    DBusMessage *reply = dbus_connection_send_with_reply_and_block(bus,
                                                                   message,
                                                                   -1,
                                                                   &error);
    const char* base_name;
    dbus_message_get_args(reply, &error, DBUS_TYPE_STRING, &base_name,
                          DBUS_TYPE_INVALID);
    dbus_message_unref(reply);
    return std::string(base_name);
  }
 private:
  MainLoopInterface *main_loop_;
  DBusConnection *system_bus_;
  DBusConnection *session_bus_;
  DBusMainLoopClosure *system_bus_closure_;
  DBusMainLoopClosure *session_bus_closure_;
};

DBusProxyFactory::DBusProxyFactory(MainLoopInterface *main_loop) :
  impl_(new Impl(main_loop)) {
}

DBusProxyFactory::~DBusProxyFactory() {
  delete impl_;
}

DBusProxy* DBusProxyFactory::NewSystemProxy(const char *name,
                                            const char *path,
                                            const char *interface,
                                            bool only_talk_to_current_owner) {
  return impl_->NewSystemProxy(name, path, interface,
                               only_talk_to_current_owner);
}

DBusProxy* DBusProxyFactory::NewSessionProxy(const char *name,
                                             const char *path,
                                             const char *interface,
                                             bool only_talk_to_current_owner) {
  return impl_->NewSessionProxy(name, path, interface,
                                only_talk_to_current_owner);
}

class DBusProxy::Impl {
 public:
  Impl(DBusProxy* owner, DBusConnection* connection,
       const char* name, const char* path, const char* interface)
    : owner_(owner), connection_(connection) {
    if (name) name_ = name;
    if (path) path_ = path;
    if (interface) interface_ = interface;
    GetRemoteMethodsAndSignals();
    AddFilter();
  }
  ~Impl() {
    RemoveFilter();
    for (SignalSlotMap::iterator it = signal_slots_.begin();
         it != signal_slots_.end(); ++it)
      delete it->second;
    for (MethodSlotMap::iterator it = method_slots_.begin();
         it != method_slots_.end(); ++it)
      delete it->second;
  }
 public:
  bool SyncCall(const char* method, int timeout,
                MessageType first_arg_type, va_list *args);
  bool SyncCall(const char* method, int timeout,
                bool not_wait_for_reply,
                Arguments *in_arguments,
                Arguments *out_arguments);
  uint32_t AsyncCall(const char* method,
                     Slot1<bool, uint32_t> *slot,
                     MessageType first_arg_type,
                     va_list *args);
  uint32_t AsyncCall(const char* method,
                     Slot1<bool, uint32_t> *slot,
                     Arguments *in_arguments);
  bool CollectResult(uint32_t call_id,
                     MessageType first_arg_type,
                     va_list *args);
  bool CollectResult(uint32_t call_id,
                     Arguments *out_arguments);
  void ConnectToSignal(const char *signal, Slot0<void>* dbus_signal_slot);
  bool EnumerateMethods(Slot2<bool, const char*, Slot*> *slot) const;
  bool EnumerateSignals(Slot2<bool, const char*, Slot*> *slot) const;
 private:
  class MethodSlot : public Slot {
   public:
    MethodSlot(DBusProxy *proxy, const Prototype &prototype) :
        proxy_(proxy), prototype_(prototype) {
      arg_types_ = new Variant::Type[prototype_.in_args.size()];
      std::size_t i = 0;
      for (; i < prototype_.in_args.size(); ++i)
        arg_types_[i] =
            DBusTypeToVariantType(prototype_.in_args[i].signature.c_str());
    }
    ~MethodSlot() {
      delete [] arg_types_;
    }
    virtual Variant Call(int argc, const Variant argv[]) const {
      VariantList out;
      bool ret = proxy_->SyncCall(prototype_.name.c_str(), -1, false,
                                  argv, argc, &out);
      if (!ret) return Variant();
      return MergeArguments(out);
    }
    virtual bool HasMetadata() const {
      return true;
    }
    virtual int GetArgCount() const {
      return prototype_.in_args.size();
    }
    virtual const Variant::Type* GetArgTypes() const {
      return arg_types_;
    }
    virtual bool operator==(const Slot &another) const {
      return down_cast<const MethodSlot*>(&another)->prototype_.name ==
          prototype_.name;
    }
   private:
    Variant MergeArguments(const VariantList &args) const {
      if (args.size() == 0) return Variant(true);
      if (args.size() == 1) return args[0];
      return Variant(ScriptableArray::Create(args.begin(), args.size()));
    }
    Variant::Type DBusTypeToVariantType(const char *s) const {
      switch (*s) {
        case 'y':
        case 'n':
        case 'q':
        case 'i':
        case 'u':
        case 'x':
        case 't':
          return Variant::TYPE_INT64;
        case 'b':
          return Variant::TYPE_BOOL;
        case 'd':
          return Variant::TYPE_DOUBLE;
        case 's':
          return Variant::TYPE_STRING;
        case 'a':
        case '(':
        case '{':
        case 'v':
          return Variant::TYPE_SCRIPTABLE;
        default:
          LOG("invalid type: %s", s);
      }
      return Variant::TYPE_VOID;
    }
    DBusProxy *proxy_;
    Prototype prototype_;
    Variant::Type *arg_types_;
  };
  static DBusHandlerResult MessageFilter(DBusConnection *connection,
                                         DBusMessage *message,
                                         void *user_data);
  std::string MatchRule() const;
  void AddFilter() {
    dbus_connection_add_filter(connection_, MessageFilter, this, NULL);
    dbus_bus_add_match(connection_, MatchRule().c_str(), NULL);
    DLOG("add rule: %s", MatchRule().c_str());
  }
  void RemoveFilter() {
    dbus_bus_remove_match(connection_, MatchRule().c_str(), NULL);
    dbus_connection_remove_filter(connection_, MessageFilter, this);
  }

  PrototypeVector::iterator FindMethod(const char *method_name);
  bool ConvertToArguments(const char *method,
                          Arguments *in, Arguments *out,
                          MessageType first_arg_type,
                          va_list *args);
  bool CheckMethodArgsValidity(const char *name,
                               Arguments *in_args,
                               Arguments *out_args,
                               PrototypeVector::iterator *iter,
                               bool *number_dismatch);

  bool GetRemoteMethodsAndSignals();
  bool ParseOneMethodNode(DOMElementInterface* node);
  bool ParseOneSignalNode(DOMElementInterface* node);
 private:
  DBusProxy *owner_;
  DBusConnection *connection_;

  std::string name_;
  std::string path_;
  std::string interface_;

  PrototypeVector method_calls_;
  PrototypeVector signals_;

  typedef std::map<std::string, Slot0<void>*> SignalSlotMap;
  SignalSlotMap signal_slots_;
  typedef std::map<uint32_t, Slot1<bool, uint32_t>*> MethodSlotMap;
  MethodSlotMap method_slots_;
  typedef std::map<uint32_t, DBusMessage*> ReplyMap;
  ReplyMap replies_;
};

PrototypeVector::iterator DBusProxy::Impl::FindMethod(const char *method_name) {
  if (!method_name) return method_calls_.end();
  for (PrototypeVector::iterator it = method_calls_.begin();
       it != method_calls_.end(); ++it)
    if (it->name == method_name) return it;
  return method_calls_.end();
}

bool DBusProxy::Impl::CheckMethodArgsValidity(const char *name,
                                              Arguments *in_args,
                                              Arguments *out_args,
                                              PrototypeVector::iterator *it,
                                              bool *number_dismatch) {
  *number_dismatch = false;
  *it = FindMethod(name);
  if (*it == method_calls_.end()) return false;
  bool ret = true;
  if (in_args && in_args->size() != (*it)->in_args.size()) {
    *number_dismatch = true;
    return false;
  }
  if (in_args) {
    for (std::size_t i = 0; i < in_args->size(); ++i)
      if (in_args->at(i) != (*it)->in_args[i]) {
        in_args->at(i).signature = (*it)->in_args[i].signature;
        ret = false;
      }
  }
  if (out_args)
    *out_args = (*it)->out_args;
  return ret;
}

bool DBusProxy::Impl::GetRemoteMethodsAndSignals() {
  XMLParserInterface *xml_parser = CreateXMLParser();
  DOMDocumentInterface *domdoc = xml_parser->CreateDOMDocument();
  domdoc->Attach();
  bool result = false;
  const char* str = NULL;
  DBusMessage *message = dbus_message_new_method_call(name_.c_str(),
                                                      path_.c_str(),
                                                      kIntrospectInterface,
                                                      kIntrospectMethod);
  DBusError error;
  dbus_error_init(&error);
  DBusMessage *reply = dbus_connection_send_with_reply_and_block(connection_,
                                                                 message,
                                                                 -1,
                                                                 &error);
  if (reply) {
    DBusMessageIter iter;
    dbus_message_iter_init(reply, &iter);
    dbus_message_iter_get_basic(&iter, &str);
    dbus_message_unref(reply);
    DLOG("xml:\n%s", str);
    if (xml_parser->ParseContentIntoDOM(str, "Introspect.xml", NULL, NULL,
                                        domdoc, NULL, NULL)) {
      DOMNodeInterface *root_node = domdoc->GetDocumentElement();
      if (!root_node || root_node->GetNodeName() != "node") {
        LOG("have no node named 'node', invalid XML returned.");
        goto exit;
      }
      for (DOMNodeInterface *interface_node = root_node->GetFirstChild();
           interface_node; interface_node = interface_node->GetNextSibling()) {
        if (interface_node->GetNodeType() != DOMNodeInterface::ELEMENT_NODE ||
            interface_node->GetNodeName() != "interface") {
          DLOG("meanless xml note, name: %s",
               interface_node->GetNodeName().c_str());
          continue;
        }
        DOMElementInterface *element =
            down_cast<DOMElementInterface*>(interface_node);
        if (element->GetAttribute("name") != interface_)
          continue;
        for (DOMNodeInterface *sub_node = interface_node->GetFirstChild();
             sub_node; sub_node = sub_node->GetNextSibling()) {
          if (sub_node->GetNodeType() == DOMNodeInterface::ELEMENT_NODE &&
              !ParseOneMethodNode(down_cast<DOMElementInterface*>(sub_node)) &&
              !ParseOneSignalNode(down_cast<DOMElementInterface*>(sub_node)))
            LOG("failed to parse one node, node type: %s",
                sub_node->GetNodeName().c_str());
        }
      }
      result = true;
    }
  } else {
    LOG("%s: %s", error.name, error.message);
  }
exit:
  domdoc->Detach();
  dbus_error_free(&error);
  delete xml_parser;
  return result;
}

bool DBusProxy::Impl::ParseOneSignalNode(DOMElementInterface* node) {
  if (node->GetNodeName() != "signal") return false;
  std::string name = node->GetAttribute("name");
  if (name.empty()) return false;
  Prototype signal(name.c_str());
  for (DOMNodeInterface* sub_node = node->GetFirstChild();
       sub_node; sub_node = sub_node->GetNextSibling()) {
    if (sub_node->GetNodeType() != DOMNodeInterface::ELEMENT_NODE ||
        sub_node->GetNodeName() != "arg")
      continue;
    node = down_cast<DOMElementInterface*>(sub_node);
    name = node->GetAttribute("name");
    std::string type = node->GetAttribute("type");
    if (type.empty()) return false;
    signal.out_args.push_back(Argument(name.c_str(), type.c_str()));
  }
  signals_.push_back(signal);
  return true;
}

bool DBusProxy::Impl::ParseOneMethodNode(DOMElementInterface* node) {
  if (node->GetNodeName() != "method") return false;
  std::string name = node->GetAttribute("name");
  if (name.empty()) return false;
  Prototype method(name.c_str());
  for (DOMNodeInterface* sub_node = node->GetFirstChild();
       sub_node; sub_node = sub_node->GetNextSibling()) {
    if (sub_node->GetNodeType() != DOMNodeInterface::ELEMENT_NODE ||
        sub_node->GetNodeName() != "arg")
      continue;
    node = down_cast<DOMElementInterface*>(sub_node);
    name = node->GetAttribute("name");
    std::string type = node->GetAttribute("type");
    if (type.empty()) return false;
    Argument arg(name.c_str(), type.c_str());
    std::string direction = node->GetAttribute("direction");
    if (direction.empty()) return false;
    if (direction == "out") {
      method.out_args.push_back(arg);
    } else if (direction == "in") {
      method.in_args.push_back(arg);
    } else {
      LOG("direction is missed or invalid: *%s*", direction.c_str());
      return false;
    }
  }
  method_calls_.push_back(method);
  return true;
}

DBusHandlerResult DBusProxy::Impl::MessageFilter(DBusConnection *connection,
                                                 DBusMessage *message,
                                                 void *user_data) {
  DLOG("Get message, type %d, sender: %s, path: %s, interface: %s, member: %s",
       dbus_message_get_type(message), dbus_message_get_sender(message),
       dbus_message_get_path(message), dbus_message_get_interface(message),
       dbus_message_get_member(message));
  DBusProxy::Impl *this_p = reinterpret_cast<DBusProxy::Impl*>(user_data);
  switch (dbus_message_get_type(message)) {
    case DBUS_MESSAGE_TYPE_SIGNAL:
      for (SignalSlotMap::iterator it = this_p->signal_slots_.begin();
           it != this_p->signal_slots_.end(); ++it) {
        if (dbus_message_is_signal(message,
                                   this_p->interface_.c_str(),
                                   it->first.c_str())) {
          ASSERT(it->second);
          (*it->second)();
        }
      }
      break;
    case DBUS_MESSAGE_TYPE_METHOD_RETURN:
      {
        dbus_uint32_t serial = dbus_message_get_reply_serial(message);
        DLOG("serial of reply: %d", serial);
        MethodSlotMap::iterator it = this_p->method_slots_.find(serial);
        if (it == this_p->method_slots_.end()) {
          LOG("No slot registered to handle this reply.");
        } else {
          if (this_p->replies_.find(serial) != this_p->replies_.end()) {
            LOG("should not happen!");
          }
          this_p->replies_[serial] = message;
          (*it->second)(serial);
        }
        return DBUS_HANDLER_RESULT_HANDLED;
      }
    default:
      DLOG("other message type: %d", dbus_message_get_type(message));
  }
  /** This signal is globaly useful, do not return other value
   * to stop other client listening this signal.
   */
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

bool DBusProxy::Impl::SyncCall(const char *method,
                               int timeout,
                               bool not_wait_for_reply,
                               Arguments *in_arguments,
                               Arguments *out_arguments) {
  ASSERT(method && *method != '\0');
  PrototypeVector::iterator it;
  bool number_dismatch;
  if (!CheckMethodArgsValidity(method, in_arguments, out_arguments,
                               &it, &number_dismatch)) {
    if (it == method_calls_.end()) {
      DLOG("no method %s registered by Introspectable interface.", method);
    } else if (number_dismatch) {
      LOG("arg number dismatch for method %s", method);
      return false;
    } else {
      LOG("Warning: Arguments for %s dismatch with the prototyp by "
          "Introspectable interface.", method);
      ASSERT(false);
    }
  }
  DBusMessage *message = dbus_message_new_method_call(name_.c_str(),
                                                      path_.c_str(),
                                                      interface_.c_str(),
                                                      method);
  DBusMarshaller marshaller(message);
  if (!marshaller.AppendArguments(*in_arguments)) {
    LOG("marshal failed.");
    dbus_message_unref(message);
    return false;
  }
  if (not_wait_for_reply) {
    DLOG("no output argument interested, do not collect pending result.");
    dbus_connection_send(connection_, message, NULL);
    dbus_connection_flush(connection_);
    return true;
  }
  DBusError error;
  dbus_error_init(&error);
  DBusMessage *reply = dbus_connection_send_with_reply_and_block(connection_,
                                                                 message,
                                                                 timeout,
                                                                 &error);
  bool ret = false;
  if (!reply || dbus_error_is_set(&error)) {
    LOG("%s: %s", error.name, error.message);
  } else {
    DBusDemarshaller demarshaller(reply);
    ret = demarshaller.GetArguments(out_arguments);
  }
  dbus_error_free(&error);
  if (reply) dbus_message_unref(reply);
  return ret;
}

bool DBusProxy::Impl::ConvertToArguments(const char *method,
                                         Arguments *in, Arguments *out,
                                         MessageType first_arg_type,
                                         va_list *args) {
  PrototypeVector::iterator it = FindMethod(method);
  if (in && !DBusMarshaller::ValistAdaptor(in, first_arg_type, args))
    return false;
  if (out) {
    if (it != method_calls_.end()) {
      *out = it->out_args;
    } else {
      DLOG("not find method %s by Introspect. Marshal by the hint "
           "of the valist.", method);
      va_list tmp_args;
      va_copy(tmp_args, *args);
      if (in)
        first_arg_type = static_cast<MessageType>(va_arg(tmp_args, int));
      if (!DBusMarshaller::ValistToAugrments(out, first_arg_type, &tmp_args))
        return false;
      va_end(tmp_args);
    }
  }
  return true;
}

bool DBusProxy::Impl::SyncCall(const char *method,
                               int timeout,
                               MessageType first_arg_type,
                               va_list *args) {
  Arguments out, in;
  bool return_now, result = false;
  if (!ConvertToArguments(method, &in, &out, first_arg_type, args))
    goto exit;
  first_arg_type = static_cast<MessageType>(va_arg(*args, int));
  return_now = (first_arg_type == MESSAGE_TYPE_INVALID);
  if (!SyncCall(method, timeout, return_now, &in, &out))
    goto exit;
  result = DBusDemarshaller::ValistAdaptor(out, first_arg_type, args);
exit:
  DeallocateContainerArugments(in);
  DeallocateContainerArugments(out);
  return result;
}

uint32_t DBusProxy::Impl::AsyncCall(const char *method,
                                    Slot1<bool, uint32_t> *slot,
                                    MessageType first_arg_type,
                                    va_list *args) {
  Arguments in;
  bool ret = false;
  if (!ConvertToArguments(method, &in, NULL, first_arg_type, args))
    goto exit;
  ret = AsyncCall(method, slot, &in);
exit:
  DeallocateContainerArugments(in);
  return ret;
}

uint32_t DBusProxy::Impl::AsyncCall(const char *method,
                                    Slot1<bool, uint32_t> *slot,
                                    Arguments *in_arguments) {
  ASSERT(method && *method != '\0');
  PrototypeVector::iterator it;
  bool number_dismatch;
  if (!CheckMethodArgsValidity(method, in_arguments, NULL,
                               &it, &number_dismatch)) {
    if (it == method_calls_.end()) {
      DLOG("no method %s registered by Introspectable interface.", method);
    } else if (number_dismatch) {
      LOG("arg number dismatch for method %s", method);
      return false;
    } else {
      LOG("Warning: Arguments for %s dismatch with the prototyp by "
          "Introspectable interface.", method);
      ASSERT(false);
    }
  }
  DBusMessage *message = dbus_message_new_method_call(name_.c_str(),
                                                      path_.c_str(),
                                                      interface_.c_str(),
                                                      method);
  DBusMarshaller marshaller(message);
  if (!marshaller.AppendArguments(*in_arguments)) {
    LOG("marshal failed.");
    dbus_message_unref(message);
    return false;
  }
  dbus_uint32_t serial = 0;
  dbus_connection_send(connection_, message, &serial);
  DLOG("serial of sent message: %d", serial);
  dbus_message_unref(message);
  if (slot) {
    MethodSlotMap::iterator it = method_slots_.find(serial);
    if (it != method_slots_.end()) {
      delete it->second;
      it->second = slot;
    } else {
      method_slots_[serial] = slot;
    }
  }
  return serial;
}

bool DBusProxy::Impl::CollectResult(uint32_t call_id,
                                    MessageType first_arg_type,
                                    va_list *args) {
  Arguments out;
  bool ret = false;
  if (!ConvertToArguments(NULL, NULL, &out, first_arg_type, args))
    goto exit;
  if (!CollectResult(call_id, &out))
    goto exit;
  ret = DBusDemarshaller::ValistAdaptor(out, first_arg_type, args);
exit:
  DeallocateContainerArugments(out);
  return ret;
}

bool DBusProxy::Impl::CollectResult(uint32_t call_id,
                                    Arguments *out_arguments) {
  ReplyMap::iterator it = replies_.find(call_id);
  if (it == replies_.end()) {
    LOG("the reply has not been received yet.");
    return false;
  }
  DBusMessage *reply = it->second;

  bool result = false;
  switch (dbus_message_get_type(reply)) {
    case DBUS_MESSAGE_TYPE_METHOD_RETURN:
      {
        DBusDemarshaller demarshaller(reply);
        Arguments out;
        result = demarshaller.GetArguments(out_arguments);
      }
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
  replies_.erase(it);
  return result;
}

void DBusProxy::Impl::ConnectToSignal(const char *signal,
                                              Slot0<void> *dbus_signal_slot) {
  ASSERT(signal);
  if (!signal || !dbus_signal_slot) return;
  SignalSlotMap::iterator iter = signal_slots_.find(signal);
  if (iter != signal_slots_.end() && iter->second) {
    delete iter->second;
    iter->second = dbus_signal_slot;
  } else {
    signal_slots_[signal] = dbus_signal_slot;
  }
}

inline std::string DBusProxy::Impl::MatchRule() const {
  if (name_[0] == ':') {
    return StringPrintf("type='signal',sender='%s',path='%s',interface='%s'",
                        name_.c_str(), path_.c_str(), interface_.c_str());
  } else {
    return StringPrintf("type='signal',path='%s',interface='%s'",
                        path_.c_str(), interface_.c_str());
  }
  return "";
}

bool DBusProxy::Impl::EnumerateMethods(Slot2<bool,
                                       const char*, Slot*> *slot) const {
  ASSERT(slot);
  for (PrototypeVector::const_iterator it = method_calls_.begin();
       it != method_calls_.end(); ++it) {
    MethodSlot *method_slot = new MethodSlot(owner_, *it);
    if (!(*slot)(it->name.c_str(), method_slot)) {
      delete slot;
      return false;
    }
  }
  delete slot;
  return true;
}

bool DBusProxy::Impl::EnumerateSignals(Slot2<bool,
                                       const char*, Slot*> *slot) const {
  return true;
}

DBusProxy::DBusProxy(DBusConnection* connection,
                     const char* name,
                     const char* path,
                     const char* interface) : impl_(NULL) {
  ASSERT(connection);
  impl_ = new Impl(this, connection, name, path, interface);
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

bool DBusProxy::SyncCall(const char* method, int timeout,
                         bool not_wait_for_reply,
                         const Variant *in_arguments, size_t count,
                         VariantList *out_arguments) {
  if (!impl_) return false;
  Arguments in_args, out_args;
  VariantListToArguments(in_arguments, count, &in_args);
  if (!impl_->SyncCall(method, timeout, not_wait_for_reply, &in_args, &out_args))
    return false;
  ArgumentsToVariantList(out_args, out_arguments);
  return true;
}

uint32_t DBusProxy::AsyncCall(const char* method,
                              Slot1<bool, uint32_t> *slot,
                              MessageType first_arg_type,
                              ...) {
  if (!impl_) return 0;

  va_list args;
  va_start(args, first_arg_type);
  uint32_t id = impl_->AsyncCall(method, slot, first_arg_type, &args);
  va_end(args);
  return id;
}

uint32_t DBusProxy::AsyncCall(const char* method,
                              Slot1<bool, uint32_t> *slot,
                              const Variant *in_arguments, size_t count) {
  if (!impl_) return 0;

  Arguments args;
  VariantListToArguments(in_arguments, count, &args);
  return impl_->AsyncCall(method, slot, &args);
}

bool DBusProxy::CollectResult(uint32_t call_id,
                              MessageType first_arg_type,
                              ...) {
  if (!impl_) return 0;

  va_list args;
  va_start(args, first_arg_type);
  bool ret = impl_->CollectResult(call_id, first_arg_type, &args);
  va_end(args);
  return ret;
}

bool DBusProxy::CollectResult(uint32_t call_id,
                              VariantList *out_arguments) {
  if (!impl_) return false;
  Arguments args;
  if (!impl_->CollectResult(call_id, &args))
    return false;
  ArgumentsToVariantList(args, out_arguments);
  return true;
}

void DBusProxy::ConnectToSignal(const char* signal,
                                Slot0<void>* dbus_signal_slot) {
  if (!impl_) return;

  impl_->ConnectToSignal(signal, dbus_signal_slot);
}

bool DBusProxy::EnumerateMethods(Slot2<bool, const char*, Slot*> *slot) const {
  if (!impl_) {
    delete slot;
    return false;
  }
  return impl_->EnumerateMethods(slot);
}

bool DBusProxy::EnumerateSignals(Slot2<bool, const char*, Slot*> *slot) const {
  if (!impl_) {
    delete slot;
    return false;
  }
  return impl_->EnumerateSignals(slot);
}

void DeallocateContainerVariant(const Variant &container) {
  if (container.type() != Variant::TYPE_SCRIPTABLE) return;
  ScriptableInterface *scriptable =
      VariantValue<ScriptableInterface*>()(container);
  // we only free the memory for DBus Container
  if (!scriptable->IsInstanceOf(kDBusContainerID)) return;
  ContainerFreeMan freeman;
  scriptable->Attach();
  scriptable->EnumerateProperties(NewSlot(
      &freeman, &ContainerFreeMan::PropertyCallback));
  scriptable->EnumerateElements(NewSlot(
      &freeman, &ContainerFreeMan::ArrayCallback));
  scriptable->Detach();
}

}  // namespace dbus
}  // namespace ggadget
