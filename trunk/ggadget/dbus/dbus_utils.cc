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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <dbus/dbus.h>
#include "dbus_utils.h"
#include <ggadget/common.h>
#include <ggadget/logger.h>
#include <ggadget/main_loop_interface.h>

namespace ggadget {
namespace dbus {

namespace {

const struct Type {
  MessageType type;
  int dbus_type;
} types[] = {
  { MESSAGE_TYPE_INVALID, DBUS_TYPE_INVALID    },
  { MESSAGE_TYPE_BYTE,    DBUS_TYPE_BYTE       },
  { MESSAGE_TYPE_BOOLEAN, DBUS_TYPE_BOOLEAN    },
  { MESSAGE_TYPE_INT16,   DBUS_TYPE_INT16      },
  { MESSAGE_TYPE_UINT16,  DBUS_TYPE_UINT16     },
  { MESSAGE_TYPE_INT32,   DBUS_TYPE_INT32      },
  { MESSAGE_TYPE_UINT32,  DBUS_TYPE_UINT32     },
  { MESSAGE_TYPE_INT64,   DBUS_TYPE_INT64      },
  { MESSAGE_TYPE_UINT64,  DBUS_TYPE_UINT64     },
  { MESSAGE_TYPE_DOUBLE,  DBUS_TYPE_DOUBLE     },
  { MESSAGE_TYPE_STRING,  DBUS_TYPE_STRING     },
  { MESSAGE_TYPE_ARRAY,   DBUS_TYPE_ARRAY      },
  { MESSAGE_TYPE_STRUCT,  DBUS_TYPE_STRUCT     },
  { MESSAGE_TYPE_VARIANT, DBUS_TYPE_VARIANT    },
  { MESSAGE_TYPE_DICT,    DBUS_TYPE_DICT_ENTRY }
};

int MessageTypeToDBusType(MessageType type) {
  // TODO: more effient way is needed.
  for (std::size_t i = 0; i < arraysize(types); ++i)
    if (types[i].type == type)
      return types[i].dbus_type;
  return DBUS_TYPE_INVALID;
}

bool IsBasicType(int type) {
  return dbus_type_is_basic(type);
}

bool IsBasicType(const char *s) {
  if (!s || s[1]) return false;
  return IsBasicType(static_cast<int>(*s));
}

std::string GetElementType(const char *signature) {
  if (!signature) return "";
  if (*signature == 'a')
    return std::string("a") + GetElementType(signature + 1);
  // don't consider such invalid cases: {is(s}.
  // it is user's reponsibility to make them right.
  if (*signature == '(' || *signature == '{') {
    const char *index = signature;
    char start = *signature;
    char end = (start == '(') ? ')' : '}';
    int counter = 1;
    while (counter != 0) {
      char ch = *++index;
      if (!ch) return "";
      if (ch == start)
        counter++;
      else if (ch == end)
        counter--;
    }
    return std::string(signature, static_cast<size_t>(index - signature + 1));
  }
  return std::string(signature, 1);
}

/** used for container type except array. */
bool GetSubElements(const char *signature,
                    StringList *sig_list) {
  if (IsBasicType(signature) || *signature == 'a') return false;
  StringList tmp_list;
  const char *begin = signature + 1;
  const char *end = signature + strlen(signature);
  while (begin < end - 1) {
    std::string sig = GetElementType(begin);
    if (sig.empty()) return false;
    tmp_list.push_back(sig);
    begin += sig.size();
  }
  sig_list->swap(tmp_list);
  return sig_list->size();
}

bool CheckSignatureValidity(const char* signature, bool single) {
  DBusError error;
  dbus_error_init(&error);
  bool ret = true;
  if ((single && !dbus_signature_validate_single(signature, &error)) ||
      !dbus_signature_validate(signature, &error)) {
    LOG("Check Validity for signature %s failed, %s: %s",
        signature, error.name, error.message);
    ret = false;
  }
  dbus_error_free(&error);
  return ret;
}

}  // anonymous namespace

std::string GetVariantSignature(const Variant &value) {
  switch (value.type()) {
    case Variant::TYPE_BOOL:
      return "b";
    case Variant::TYPE_INT64:
      return "i";
    case Variant::TYPE_DOUBLE:
      return "d";
    case Variant::TYPE_STRING:
    case Variant::TYPE_UTF16STRING:
    case Variant::TYPE_JSON:
      return "s";
    case Variant::TYPE_SCRIPTABLE:
      {
        ScriptableInterface *scriptable =
            VariantValue<ScriptableInterface*>()(value);
        Variant length = GetPropertyByName(scriptable, "length");
        if (length.type() != Variant::TYPE_VOID) {
          /* firstly treat as an array. */
          ArrayIterator iterator;
          scriptable->EnumerateElements(
              NewSlot(&iterator, &ArrayIterator::Callback));
          std::string sig = iterator.signature();
          if (!sig.empty()) return sig;
        }
        DictIterator iterator;
        if (!scriptable->EnumerateProperties(
            NewSlot(&iterator, &DictIterator::Callback)))
          return "";
        std::string dict_signature = "a{s";
        dict_signature += iterator.signature();
        dict_signature += "}";
        return dict_signature;
      }
      break;
    default:
      LOG("other meanless type: %d", value.type());
  }
  return "";
}

class DBusMarshaller::Impl {
 public:
  Impl(DBusMessage *message) : iter_(NULL), parent_iter_(NULL) {
    iter_ = new DBusMessageIter;
    dbus_message_iter_init_append(message, iter_);
  }
  Impl(DBusMessageIter *parent, int type, const char *sig) :
      iter_(NULL), parent_iter_(parent) {
    iter_ = new DBusMessageIter;
    dbus_message_iter_open_container(parent, type, sig, iter_);
  }
  ~Impl() {
    if (parent_iter_)
      dbus_message_iter_close_container(parent_iter_, iter_);
    delete iter_;
  }
  bool AppendArgument(const Argument &arg) {
    if (arg.signature.empty()) {
      std::string sig = GetVariantSignature(arg.value);
      if (sig.empty()) return false;
      return AppendArgument(Argument(sig.c_str(), arg.value));
    }
    const char* index = arg.signature.c_str();
    if (!CheckSignatureValidity(index, true)) return false;
    switch (*index) {
      case 'y':
        {
          int64_t i;
          if (!arg.value.ConvertToInt64(&i)) {
            LOG("type dismatch. expected type: %d, actual value:%s",
                Variant::TYPE_INT64, arg.value.Print().c_str());
            return false;
          }
          Append(static_cast<unsigned char>(i));
          break;
        }
      case 'b':
        {
          bool b;
          if (!arg.value.ConvertToBool(&b)) {
            LOG("type dismatch. expected type: %d, actual value:%s",
                Variant::TYPE_INT64, arg.value.Print().c_str());
            return false;
          }
          Append(b);
          break;
        }
      case 'n':
        {
          int64_t i;
          if (!arg.value.ConvertToInt64(&i)) {
            LOG("type dismatch. expected type: %d, actual value:%s",
                Variant::TYPE_INT64, arg.value.Print().c_str());
            return false;
          }
          Append(static_cast<int16_t>(i));
          break;
        }
      case 'q':
        {
          int64_t i;
          if (!arg.value.ConvertToInt64(&i)) {
            LOG("type dismatch. expected type: %d, actual value:%s",
                Variant::TYPE_INT64, arg.value.Print().c_str());
            return false;
          }
          Append(static_cast<uint16_t>(i));
          break;
        }
      case 'i':
        {
          int64_t i;
          if (!arg.value.ConvertToInt64(&i)) {
            LOG("type dismatch. expected type: %d, actual value:%s",
                Variant::TYPE_INT64, arg.value.Print().c_str());
            return false;
          }
          Append(static_cast<int32_t>(i));
          break;
        }
      case 'u':
        {
          int64_t i;
          if (!arg.value.ConvertToInt64(&i)) {
            LOG("type dismatch. expected type: %d, actual value:%s",
                Variant::TYPE_INT64, arg.value.Print().c_str());
            return false;
          }
          Append(static_cast<uint32_t>(i));
          break;
        }
      case 'x':
        {
          int64_t v;
          if (!arg.value.ConvertToInt64(&v)) {
            LOG("type dismatch. expected type: %d, actual value:%s",
                Variant::TYPE_INT64, arg.value.Print().c_str());
            return false;
          }
          Append(v);
          break;
        }
      case 't':
        {
          int64_t i;
          if (!arg.value.ConvertToInt64(&i)) {
            LOG("type dismatch. expected type: %d, actual value:%s",
                Variant::TYPE_INT64, arg.value.Print().c_str());
            return false;
          }
          Append(static_cast<uint64_t>(i));
          break;
        }
      case 'd':
        {
          double v;
          if (!arg.value.ConvertToDouble(&v)) {
            LOG("type dismatch. expected type: %d, actual value:%s",
                Variant::TYPE_INT64, arg.value.Print().c_str());
            return false;
          }
          Append(v);
          break;
        }
      case 's':
        {
          std::string s;
          if (!arg.value.ConvertToString(&s)) {
            LOG("type dismatch. expected type: %d, actual value:%s",
                Variant::TYPE_INT64, arg.value.Print().c_str());
            return false;
          }
          Append(s.c_str());
          break;
        }
      case 'a':
        {
          if (arg.value.type() != Variant::TYPE_SCRIPTABLE) {
            LOG("type dismatch. expected type: %d, actual value:%s",
                Variant::TYPE_INT64, arg.value.Print().c_str());
            return false;
          }
          /* special handle dict case */
          if (*(index + 1) == '{') {
            std::string dict_sig = GetElementType(index + 1);
            StringList sig_list;
            if (!GetSubElements(dict_sig.c_str(), &sig_list) ||
                sig_list.size() != 2 || !IsBasicType(sig_list[0].c_str()))
              return false;
            ScriptableInterface *dict =
                VariantValue<ScriptableInterface*>()(arg.value);
            Impl *sub = new Impl(iter_, DBUS_TYPE_ARRAY, dict_sig.c_str());
            DictIterator slot(sub, sig_list[0].c_str(), sig_list[1].c_str());
            if (!dict->EnumerateProperties(
                NewSlot(&slot, &DictIterator::Callback)))
              return false;
            index += dict_sig.size();
          } else {
            std::string signature = GetElementType(index + 1);
            Impl *sub = new Impl(iter_, DBUS_TYPE_ARRAY, signature.c_str());
            ScriptableInterface *array =
                VariantValue<ScriptableInterface*>()(arg.value);
            ArrayIterator slot(sub, signature.c_str());
            if (!array->EnumerateElements(
                NewSlot(&slot, &ArrayIterator::Callback)))
              return false;
            index += signature.size();
          }
          break;
        }
      case '(':
      /* normally, {} should not exist without a.
       * We add it here just for safety.
       */
      case '{':
        {
          if (arg.value.type() != Variant::TYPE_SCRIPTABLE) {
            LOG("type dismatch. expected type: %d, actual value:%s",
                Variant::TYPE_INT64, arg.value.Print().c_str());
            return false;
          }
          StringList sig_list;
          std::string struct_signature = GetElementType(index);
          if (!GetSubElements(struct_signature.c_str(), &sig_list))
            return false;
          Impl *sub = new Impl(iter_, *index == '(' ?
                               DBUS_TYPE_STRUCT : DBUS_TYPE_DICT_ENTRY, NULL);
          ScriptableInterface *structure =
              VariantValue<ScriptableInterface*>()(arg.value);
          StructIterator slot(sub, sig_list);
          if (!structure->EnumerateElements(
              NewSlot(&slot, &StructIterator::Callback)))
            return false;
          index += struct_signature.size() - 1;
          break;
        }
      case 'v':
        {
          std::string sig = GetVariantSignature(arg.value);
          Impl sub(iter_, DBUS_TYPE_VARIANT, sig.c_str());
          if (!sub.AppendArgument(Argument(sig.c_str(), arg.value)))
            return false;
          break;
        }
      default:
        LOG("Unsupported type: %s", index);
        return false;
    }
    if (index != arg.signature.c_str() + arg.signature.size() - 1)
      return false;
    return true;
  }
  static bool ValistAdaptor(Arguments *in_args, bool copy_value,
                            MessageType first_arg_type,
                            va_list *va_args) {
    Arguments tmp_args;
    while (first_arg_type != MESSAGE_TYPE_INVALID) {
      Argument arg;
      if (!ValistItemAdaptor(&arg, copy_value, first_arg_type, va_args))
        return false;
      tmp_args.push_back(arg);
      first_arg_type = static_cast<MessageType>(va_arg(*va_args, int));
    }
    in_args->swap(tmp_args);
    return true;
  }
 private:
  class ArrayIterator {
   public:
    ArrayIterator(Impl *impl, const char *sig) :
      marshaller_(impl), signature_(sig) {
    }
    ~ArrayIterator() {
      delete marshaller_;
    }
    bool Callback(int id, const Variant &value) {
      Argument arg(signature_.c_str(), value);
      return marshaller_->AppendArgument(arg);
    }
   private:
    Impl *marshaller_;
    std::string signature_;
  };

  class StructIterator {
   public:
    StructIterator(Impl *impl, const std::vector<std::string>& signatures) :
      marshaller_(impl), signature_list_(signatures), index_(0) {}
    ~StructIterator() {
      delete marshaller_;
    }
    bool Callback(int id, const Variant &value) {
      if (index_ >= signature_list_.size()) {
        LOG("the signature of the variant does not match the specified "
            "signature.");
        return false;
      }
      Argument arg(signature_list_[index_].c_str(), value);
      index_++;
      return marshaller_->AppendArgument(arg);
    }
   private:
    Impl *marshaller_;
    const std::vector<std::string> &signature_list_;
    std::string::size_type index_;
  };

  class DictIterator {
   public:
    DictIterator(Impl *impl,
                 const char *key_signature,
                 const char *value_signature) : marshaller_(impl) {
      if (key_signature) key_signature_ = key_signature;
      if (value_signature) value_signature_ = value_signature;
    }
    ~DictIterator() {
      delete marshaller_;
    }
    bool Callback(int id, const char *name,
                  const Variant &value, bool is_method) {
      if (is_method) return true;  /* ignore method. */
      Argument key_arg(key_signature_.c_str(), Variant(name));
      Argument value_arg(value_signature_.c_str(), value);
      Impl *sub = new Impl(marshaller_->iter_, DBUS_TYPE_DICT_ENTRY, NULL);
      bool ret = sub->AppendArgument(key_arg) && sub->AppendArgument(value_arg);
      delete sub;
      return ret;
    }
   private:
    Impl *marshaller_;
    std::string key_signature_;
    std::string value_signature_;
  };

  static bool ValistItemAdaptor(Argument *in_arg, bool copy_value,
                                MessageType first_arg_type, va_list *va_args) {
    ASSERT(in_arg);
    MessageType type = first_arg_type;
    if (type == MESSAGE_TYPE_INVALID) return false;
    switch (type) {
      case MESSAGE_TYPE_BYTE:
        in_arg->signature = "y";
        if (copy_value) {
          int64_t v = static_cast<int64_t>(va_arg(*va_args, int));
          in_arg->value = Variant(v);
        }
        break;
      case MESSAGE_TYPE_BOOLEAN:
        in_arg->signature = "b";
        if (copy_value) {
          bool v = va_arg(*va_args, int);
          in_arg->value = Variant(v);
        }
        break;
      case MESSAGE_TYPE_INT16:
        in_arg->signature = "n";
        if (copy_value) {
          int64_t v = static_cast<int64_t>(va_arg(*va_args, int));
          in_arg->value = Variant(v);
        }
        break;
      case MESSAGE_TYPE_UINT16:
        in_arg->signature = "q";
        if (copy_value) {
          int64_t v = static_cast<int64_t>(va_arg(*va_args, int));
          in_arg->value = Variant(v);
        }
        break;
      case MESSAGE_TYPE_INT32:
        in_arg->signature = "i";
        if (copy_value) {
          int64_t v = static_cast<int64_t>(va_arg(*va_args, int32_t));
          in_arg->value = Variant(v);
        }
        break;
      case MESSAGE_TYPE_UINT32:
        in_arg->signature = "u";
        if (copy_value) {
          int64_t v = static_cast<int64_t>(va_arg(*va_args, uint32_t));
          in_arg->value = Variant(v);
        }
        break;
      case MESSAGE_TYPE_INT64:
        in_arg->signature = "x";
        if (copy_value) {
          int64_t v = va_arg(*va_args, int64_t);
          in_arg->value = Variant(v);
        }
        break;
      case MESSAGE_TYPE_UINT64:
        in_arg->signature = "t";
        if (copy_value) {
          int64_t v = static_cast<int64_t>(va_arg(*va_args, uint64_t));
          in_arg->value = Variant(v);
        }
        break;
      case MESSAGE_TYPE_DOUBLE:
        in_arg->signature = "d";
        if (copy_value) {
          double v = va_arg(*va_args, double);
          in_arg->value = Variant(v);
        }
        break;
      case MESSAGE_TYPE_STRING:
        in_arg->signature = "s";
        if (copy_value) {
          const char *str = va_arg(*va_args, const char*);
          in_arg->value = Variant(str);
        }
        break;
      case MESSAGE_TYPE_ARRAY:
        {
          Argument arg;
          std::string signature("a"), item_sig;
          std::size_t size = static_cast<std::size_t>(va_arg(*va_args, int));
          Variant *vec = new Variant[size];
          bool ret = true;
          for (std::size_t i = 0; i < size && ret; ++i) {
            type = static_cast<MessageType>(va_arg(*va_args, int));
            ret = ValistItemAdaptor(&arg, copy_value, type, va_args);
            if (item_sig.empty()) {
              item_sig = arg.signature;
            } else if (item_sig != arg.signature) {
              LOG("types of items in the array are not the same.");
              ret = false;
            }
            vec[i] = arg.value;
          }
          if (!ret) {
            delete [] vec;
            return false;
          }
          signature.append(item_sig);
          ScriptableDBusContainer *array =
              new ScriptableDBusContainer(vec, size);
          in_arg->value = Variant(array);
          in_arg->signature = signature;
          break;
        }
      case MESSAGE_TYPE_STRUCT:
        {
          std::size_t size = static_cast<std::size_t>(va_arg(*va_args, int));
          Variant *vec = new Variant[size];
          std::string signature("(");
          Argument arg;
          bool ret = true;
          for (std::size_t i = 0; i < size && ret; ++i) {
            type = static_cast<MessageType>(va_arg(*va_args, int));
            ret = ValistItemAdaptor(&arg, copy_value, type, va_args);
            signature.append(arg.signature);
            vec[i] = arg.value;
          }
          if (!ret) {
            delete [] vec;
            return false;
          }
          signature.append(")");
          ScriptableDBusContainer *array =
              new ScriptableDBusContainer(vec, size);
          in_arg->signature = signature;
          in_arg->value = Variant(array);
          break;
        }
      case MESSAGE_TYPE_VARIANT:
        type = static_cast<MessageType>(va_arg(*va_args, int));
        if (type == MESSAGE_TYPE_INVALID || type == MESSAGE_TYPE_VARIANT)
          return false;
        if (!ValistItemAdaptor(in_arg, copy_value, type, va_args))
          return false;
        in_arg->signature = "v";
        break;
      case MESSAGE_TYPE_DICT:
        {
          std::size_t size = static_cast<std::size_t>(va_arg(*va_args, int));
          ScriptableDBusContainer *obj = new ScriptableDBusContainer;
          std::string signature("a{"), key_sig, value_sig;
          Argument arg;
          bool ret = true;
          for (std::size_t i = 0; i < size && ret; ++i) {
            type = static_cast<MessageType>(va_arg(*va_args, int));
            ret = ValistItemAdaptor(&arg, copy_value, type, va_args);
            if (!ret) break;
            if (key_sig.empty()) {
              key_sig = arg.signature;
            } else if (key_sig != arg.signature) {
              ret = false;
              break;
            }
            std::string str;
            if (!arg.value.ConvertToString(&str)) {
              LOG("%s can not be converted to string to be a dict key",
                  arg.value.Print().c_str());
              ret = false;
              break;
            }
            type = static_cast<MessageType>(va_arg(*va_args, int));
            ret = ValistItemAdaptor(&arg, copy_value, type, va_args);
            if (!ret) break;
            if (value_sig.empty()) {
              value_sig = arg.signature;
            } else if (value_sig != arg.signature) {
              ret = false;
              break;
            }
            obj->AddProperty(str.c_str(), arg.value);
          }
          if (!ret) {
            delete obj;
            return false;
          }
          signature.append(key_sig);
          signature.append(value_sig);
          signature.append("}");
          in_arg->signature = signature;
          in_arg->value = Variant(obj);
          break;
        }
      default:
        LOG("unsupported type: %d", type);
        return false;
    }
    if (!copy_value)
      va_arg(*va_args, void*);
    return true;
  }

  void Append(unsigned char value) {
    dbus_message_iter_append_basic(iter_, DBUS_TYPE_BYTE, &value);
  }
  void Append(bool value) {
    dbus_bool_t v = static_cast<dbus_bool_t>(value);
    dbus_message_iter_append_basic(iter_, DBUS_TYPE_BOOLEAN, &v);
  }
  void Append(int16_t value) {
    dbus_int16_t v = static_cast<dbus_int16_t>(value);
    dbus_message_iter_append_basic(iter_, DBUS_TYPE_INT16, &v);
  }
  void Append(uint16_t value) {
    dbus_uint16_t v = static_cast<dbus_uint16_t>(value);
    dbus_message_iter_append_basic(iter_, DBUS_TYPE_UINT16, &v);
  }
  void Append(int32_t value) {
    dbus_int32_t v = static_cast<dbus_int32_t>(value);
    dbus_message_iter_append_basic(iter_, DBUS_TYPE_INT32, &v);
  }
  void Append(uint32_t value) {
    dbus_uint32_t v = static_cast<dbus_uint32_t>(value);
    dbus_message_iter_append_basic(iter_, DBUS_TYPE_UINT32, &v);
  }
  void Append(int64_t value) {
    dbus_int64_t v = static_cast<dbus_int64_t>(value);
    dbus_message_iter_append_basic(iter_, DBUS_TYPE_INT64, &v);
  }
  void Append(uint64_t value) {
    dbus_uint64_t v = static_cast<dbus_uint64_t>(value);
    dbus_message_iter_append_basic(iter_, DBUS_TYPE_UINT64, &v);
  }
  void Append(double value) {
    dbus_message_iter_append_basic(iter_, DBUS_TYPE_DOUBLE, &value);
  }
  void Append(const char* str) {
    const char *v = str;
    dbus_message_iter_append_basic(iter_, DBUS_TYPE_STRING, &v);
  }
 private:
  DBusMessageIter *iter_;
  DBusMessageIter *parent_iter_;
};

DBusMarshaller::DBusMarshaller(DBusMessage *message) : impl_(new Impl(message)) {
}

DBusMarshaller::~DBusMarshaller() {
  delete impl_;
}

bool DBusMarshaller::AppendArguments(const Arguments &args) {
  for (Arguments::const_iterator it = args.begin(); it != args.end(); ++it)
    if (!AppendArgument(*it))
      return false;
  return true;
}

bool DBusMarshaller::AppendArgument(const Argument &arg) {
  return impl_->AppendArgument(arg);
}

bool DBusMarshaller::ValistAdaptor(Arguments *in_args,
                                   MessageType first_arg_type,
                                   va_list *va_args) {
  return Impl::ValistAdaptor(in_args, true, first_arg_type, va_args);
}

bool DBusMarshaller::ValistToAugrments(Arguments *out_args,
                                       MessageType first_arg_type,
                                       va_list *va_args) {
  return Impl::ValistAdaptor(out_args, false, first_arg_type, va_args);
}

class DBusDemarshaller::Impl {
 public:
  Impl(DBusMessage *message) : iter_(NULL), parent_iter_(NULL) {
    iter_ = new DBusMessageIter;
    dbus_message_iter_init(message, iter_);
  }
  Impl(DBusMessageIter *parent) : iter_(NULL), parent_iter_(parent) {
    iter_ = new DBusMessageIter;
    dbus_message_iter_recurse(parent, iter_);
  }
  ~Impl() {
    delete iter_;
  }
  bool MoveToNextItem() {
    return dbus_message_iter_next(iter_);
  }
  bool GetArgument(Argument *arg) {
    if (arg->signature.empty()) {
      char *sig = dbus_message_iter_get_signature(iter_);
      arg->signature = sig;
      dbus_free(sig);
      // no value remained in current message iterator
      if (arg->signature.empty()) return false;
    }
    const char *index = arg->signature.c_str();
    if (!CheckSignatureValidity(index, true))
      return false;
    int type = dbus_message_iter_get_arg_type(iter_);
    if (type != GetTypeBySignature(index)) {
      LOG("Demarshal failed. Type dismatch, message type: %c, expected: %c",
          type, GetTypeBySignature(index));
      return false;
    }
    switch (type) {
      case DBUS_TYPE_BYTE:
        {
          unsigned char value;
          dbus_message_iter_get_basic(iter_, &value);
          arg->value = Variant(static_cast<int64_t>(value));
          break;
        }
      case DBUS_TYPE_BOOLEAN:
        {
          dbus_bool_t value;
          dbus_message_iter_get_basic(iter_, &value);
          arg->value = Variant(static_cast<bool>(value));
          break;
        }
      case DBUS_TYPE_INT16:
        {
          dbus_int16_t value;
          dbus_message_iter_get_basic(iter_, &value);
          arg->value = Variant(static_cast<int64_t>(value));
          break;
        }
      case DBUS_TYPE_UINT16:
        {
          dbus_uint16_t value;
          dbus_message_iter_get_basic(iter_, &value);
          arg->value = Variant(static_cast<int64_t>(value));
          break;
        }
      case DBUS_TYPE_INT32:
        {
          dbus_int32_t value;
          dbus_message_iter_get_basic(iter_, &value);
          arg->value = Variant(static_cast<int64_t>(value));
          break;
        }
      case DBUS_TYPE_UINT32:
        {
          dbus_uint32_t value;
          dbus_message_iter_get_basic(iter_, &value);
          arg->value = Variant(static_cast<int64_t>(value));
          break;
        }
      case DBUS_TYPE_INT64:
        {
          dbus_int64_t value;
          dbus_message_iter_get_basic(iter_, &value);
          arg->value = Variant(static_cast<int64_t>(value));
          break;
        }
      case DBUS_TYPE_UINT64:
        {
          dbus_uint64_t value;
          dbus_message_iter_get_basic(iter_, &value);
          arg->value = Variant(static_cast<int64_t>(value));
          break;
        }
      case DBUS_TYPE_DOUBLE:
        {
          double value;
          dbus_message_iter_get_basic(iter_, &value);
          arg->value = Variant(value);
          break;
        }
      case DBUS_TYPE_STRING:
      case DBUS_TYPE_OBJECT_PATH:
        {
          const char *str;
          dbus_message_iter_get_basic(iter_, &str);
          arg->value = Variant(str);
          break;
        }
      case DBUS_TYPE_ARRAY:
        {
          if (*(index + 1) == '{') {  /* it is a dict. */
            std::string dict_sig = GetElementType(index + 1);
            StringList sig_list;
            if (!GetSubElements(dict_sig.c_str(), &sig_list))
              return false;
            if (sig_list.size() != 2 || !IsBasicType(sig_list[0].c_str()))
              return false;
            Impl dict(iter_);
            ScriptableDBusContainer *obj = new ScriptableDBusContainer;
            bool ret;
            do {
              Impl sub(dict.iter_);
              Argument key(sig_list[0].c_str());
              Argument value(sig_list[1].c_str());
              ret = sub.GetArgument(&key) && sub.MoveToNextItem()
                  && sub.GetArgument(&value);
              if (!ret) break;
              std::string name;
              if (!key.value.ConvertToString(&name)) {
                ret = false;
                break;
              }
              obj->AddProperty(name.c_str(), value.value);
            } while(dict.MoveToNextItem());
            if (!ret) {
              delete obj;
              return false;
            }
            arg->value = Variant(obj);
            index += dict_sig.size();
          } else {
            std::string sub_type = GetElementType(index + 1);
            Impl sub(iter_);
            bool ret;
            std::vector<Variant> var_array;
            do {
              Argument arg(sub_type.c_str());
              ret = sub.GetArgument(&arg);
              if (ret)
                var_array.push_back(arg.value);
            } while (ret && sub.MoveToNextItem());
            if (!ret) {
              DLOG("something wrong.");
              return false;
            }
            ScriptableDBusContainer *obj = new ScriptableDBusContainer;
            obj->AddArray(var_array.begin(), var_array.size());
            arg->value = Variant(obj);
            index += sub_type.size();
          }
          break;
        }
      case DBUS_TYPE_STRUCT:
      case DBUS_TYPE_DICT_ENTRY:
        {
          std::string struct_sig = GetElementType(index);
          StringList sig_list;
          if (!GetSubElements(struct_sig.c_str(), &sig_list))
            return false;
          if (type == DBUS_TYPE_DICT_ENTRY &&
              (sig_list.size() != 2 || !IsBasicType(sig_list[0].c_str())))
            return false;
          Impl sub(iter_);
          std::vector<Variant> var_array;
          bool ret = false;
          for (StringList::iterator it = sig_list.begin();
               it != sig_list.end(); ++it) {
            Argument arg(it->c_str());
            ret = sub.GetArgument(&arg);
            if (!ret) break;
            var_array.push_back(arg.value);
            sub.MoveToNextItem();
          }
          if (!ret) return false;
          ScriptableDBusContainer *obj = new ScriptableDBusContainer;
          obj->AddArray(var_array.begin(), var_array.size());
          arg->value = Variant(obj);
          index += struct_sig.size() - 1;
          break;
        }
      case DBUS_TYPE_VARIANT:
        {
          DBusMessageIter subiter;
          dbus_message_iter_recurse(iter_, &subiter);
          char *sig = dbus_message_iter_get_signature(&subiter);
          if (!sig) {
            LOG("sub type of variant is invalid.");
            return false;
          }
          Impl sub(iter_);
          Argument variant(sig);
          bool ret = sub.GetArgument(&variant);
          dbus_free(sig);
          if (!ret) return false;
          arg->value = variant.value;
          break;
        }
      default:
        LOG("unsupported type: %d", type);
        return false;
    }
    return true;
  }
  static bool ValistAdaptor(const Arguments &out_args,
                            MessageType first_arg_type, va_list *va_args) {
    MessageType type = first_arg_type;
    Arguments::const_iterator it = out_args.begin();
    while (type != MESSAGE_TYPE_INVALID) {
      if (it == out_args.end()) {
        LOG("Too few arguments in reply.");
        return false;
      }
      int arg_type = GetTypeBySignature(it->signature.c_str());
      if (arg_type != MessageTypeToDBusType(type)) {
        LOG("type dismatch! the type in message is %d, "
            " but in this function it is %d", arg_type, type);
        ASSERT(false);
        return false;
      }
      if (!ValistItemAdaptor(*it, type, va_args))
        return false;
      it++;
      type = static_cast<MessageType>(va_arg(*va_args, int));
    }
    return true;
  }
 private:
  static bool ValistItemAdaptor(const Argument &out_arg,
                                MessageType first_arg_type, va_list *va_args) {
    if (first_arg_type == MESSAGE_TYPE_INVALID)
      return false;
    void *return_storage = va_arg(*va_args, void*);
    if (return_storage) {
      switch (first_arg_type) {
        case MESSAGE_TYPE_BYTE:
          {
            int64_t i;
            if (!out_arg.value.ConvertToInt64(&i)) {
              LOG("type dismatch.");
              return false;
            }
            unsigned char v = static_cast<unsigned char>(i);
            memcpy(return_storage, &v, sizeof(unsigned char));
            break;
          }
        case MESSAGE_TYPE_BOOLEAN:
          {
            bool v;
            if (!out_arg.value.ConvertToBool(&v)) {
              LOG("type dismatch.");
              return false;
            }
            memcpy(return_storage, &v, sizeof(bool));
            break;
          }
        case MESSAGE_TYPE_INT16:
          {
            int64_t i;
            if (!out_arg.value.ConvertToInt64(&i)) {
              LOG("type dismatch.");
              return false;
            }
            int16_t v = static_cast<int16_t>(i);
            memcpy(return_storage, &v, sizeof(int16_t));
            break;
          }
        case MESSAGE_TYPE_UINT16:
          {
            int64_t i;
            if (!out_arg.value.ConvertToInt64(&i)) {
              LOG("type dismatch.");
              return false;
            }
            uint16_t v = static_cast<uint16_t>(i);
            memcpy(return_storage, &v, sizeof(dbus_uint16_t));
            break;
          }
        case MESSAGE_TYPE_INT32:
          {
            int64_t i;
            if (!out_arg.value.ConvertToInt64(&i)) {
              LOG("type dismatch.");
              return false;
            }
            int v = static_cast<int>(i);
            memcpy(return_storage, &v, sizeof(dbus_int32_t));
            break;
          }
        case MESSAGE_TYPE_UINT32:
          {
            int64_t i;
            if (!out_arg.value.ConvertToInt64(&i)) {
              LOG("type dismatch.");
              return false;
            }
            uint32_t v = static_cast<uint32_t>(i);
            memcpy(return_storage, &v, sizeof(dbus_uint32_t));
            break;
          }
        case MESSAGE_TYPE_INT64:
          {
            int64_t i;
            if (!out_arg.value.ConvertToInt64(&i)) {
              LOG("type dismatch.");
              return false;
            }
            memcpy(return_storage, &i, sizeof(dbus_int64_t));
            break;
          }
        case MESSAGE_TYPE_UINT64:
          {
            int64_t i;
            if (!out_arg.value.ConvertToInt64(&i)) {
              LOG("type dismatch.");
              return false;
            }
            uint64_t v = static_cast<uint64_t>(i);
            memcpy(return_storage, &v, sizeof(dbus_uint64_t));
            break;
          }
        case MESSAGE_TYPE_DOUBLE:
          {
            double v;
            if (!out_arg.value.ConvertToDouble(&v)) {
              LOG("type dismatch.");
              return false;
            }
            memcpy(return_storage, &v, sizeof(double));
            break;
          }
        case MESSAGE_TYPE_STRING:
          {
            std::string str;
            if (!out_arg.value.ConvertToString(&str)) {
              LOG("type dismatch.");
              return false;
            }
            const char* s = str.c_str();
            memcpy(return_storage, &s, sizeof(const char*));
            break;
          }
        // directly return the Variant for container types
        case MESSAGE_TYPE_ARRAY:
        case MESSAGE_TYPE_STRUCT:
        case MESSAGE_TYPE_VARIANT:
        case MESSAGE_TYPE_DICT:
          memcpy(return_storage, &out_arg.value, sizeof(Variant));
        default:
          LOG("the type %d is not supported yet!", first_arg_type);
          return false;
      }
    }
    return true;
  }
  static int GetTypeBySignature(const char *signature) {
    if (*signature == '(') return static_cast<int>('r');
    if (*signature == '{') return static_cast<int>('e');
    return static_cast<int>(*signature);
  }
  DBusMessageIter *iter_;
  DBusMessageIter *parent_iter_;
};

DBusDemarshaller::DBusDemarshaller(DBusMessage *message) :
  impl_(new Impl(message)) {}

DBusDemarshaller::~DBusDemarshaller() {
  delete impl_;
}

bool DBusDemarshaller::GetArguments(Arguments *args) {
  Arguments tmp;
  bool ret = true;
  while (ret) {
    Argument arg;
    ret = GetArgument(&arg);
    if (ret) tmp.push_back(arg);
  }
  args->swap(tmp);
  return true;
}

bool DBusDemarshaller::GetArgument(Argument *arg) {
  bool ret = impl_->GetArgument(arg);
  impl_->MoveToNextItem();
  return ret;
}

bool DBusDemarshaller::ValistAdaptor(const Arguments &out_args,
                                     MessageType first_arg_type,
                                     va_list *va_args) {
  return Impl::ValistAdaptor(out_args, first_arg_type, va_args);
}

class DBusMainLoopClosure::Impl {
 public:
  Impl(DBusConnection* connection,
       MainLoopInterface* main_loop) :
      connection_(connection), main_loop_(main_loop) {
    Setup();
  }
  ~Impl() {
    RemoveFunctions();
  }
 private:
  bool Setup();
  void RemoveFunctions();
  class DBusWatchCallBack;
  class DBusTimeoutCallBack;
  static dbus_bool_t AddWatch(DBusWatch *watch, void* data);
  static void RemoveWatch(DBusWatch* watch, void* data);
  static void WatchToggled(DBusWatch* watch, void* data);
  static dbus_bool_t AddTimeout(DBusTimeout *timeout, void* data);
  static void RemoveTimeout(DBusTimeout *timeout, void* data);
  static void TimeoutToggled(DBusTimeout *timeout, void* data);
  static void DispatchStatusFunction(DBusConnection *connection,
                                     DBusDispatchStatus new_status,
                                     void *data);
 private:
  DBusConnection* connection_;
  MainLoopInterface* main_loop_;
};

void DBusMainLoopClosure::Impl::DispatchStatusFunction(
    DBusConnection *connection, DBusDispatchStatus new_status, void *data) {
  int status;
  while ((status = dbus_connection_dispatch(connection))
         == DBUS_DISPATCH_DATA_REMAINS)
    ;
  if (status != DBUS_DISPATCH_COMPLETE)
    LOG("dispatch failed.");
}

class DBusMainLoopClosure::Impl::DBusWatchCallBack : public
                                                     WatchCallbackInterface {
 public:
  DBusWatchCallBack(DBusConnection* connection,
                    DBusWatch* watch) : connection_(connection), watch_(watch) {
    enabled_ = dbus_watch_get_enabled(watch_);
  }
  virtual ~DBusWatchCallBack() {}
  virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
    LOG("Call DBusWatchCallBack, watch id: %d", watch_id);
    if (!enabled_) return true;
    if (dbus_connection_get_dispatch_status(connection_) !=
        DBUS_DISPATCH_COMPLETE) {
      dbus_connection_dispatch(connection_);
    }
    int flag = dbus_watch_get_flags(watch_);
    dbus_watch_handle(watch_, flag);
    // alwasy return true to not remove this callback. It should be removed by
    // dbus library by invoking RemoveWatch
    return true;
  }
  virtual void OnRemove(MainLoopInterface* main_loop, int watch_id) {
    DLOG("Remove DBusWatchCallBack.");
    delete this;
  }
  void SetEnabled(bool enabled) {
    enabled_ = enabled;
  }
  void AttachToMainLoop(MainLoopInterface* main_loop, int fd) {
    watch_id_ = main_loop->AddIOReadWatch(fd, this);
  }
  void RemoveSelf(MainLoopInterface* main_loop) {
    main_loop->RemoveWatch(watch_id_);
  }
 private:
  DBusConnection* connection_;
  bool            enabled_;
  DBusWatch*      watch_;
  int             watch_id_;
};

/** watch related methods */
dbus_bool_t DBusMainLoopClosure::Impl::AddWatch(DBusWatch *watch,
                                                void* data) {
  Impl *this_p = reinterpret_cast<Impl*>(data);
  ASSERT(this_p);
#ifdef HAVE_DBUS_WATCH_GET_UNIX_FD
  int fd = dbus_watch_get_unix_fd(watch);
#else
  int fd = dbus_watch_get_fd(watch);
#endif
  int flag = dbus_watch_get_flags(watch);
  DLOG("add watch, fd: %d, flag: %d", fd, flag);
  if (flag == DBUS_WATCH_READABLE) {
    DBusWatchCallBack* callback = new DBusWatchCallBack(this_p->connection_,
                                                        watch);
    dbus_watch_set_data(watch, callback, NULL);
    callback->AttachToMainLoop(this_p->main_loop_, fd);
  } else if (flag == DBUS_WATCH_WRITABLE) {
    /** do nothing */
  } else {
    LOG("invalid flag: %d", flag);
  }
  return TRUE;
}

void DBusMainLoopClosure::Impl::RemoveWatch(DBusWatch* watch,
                                            void* data) {
  DBusWatchCallBack* callback = reinterpret_cast<DBusWatchCallBack*>(
      dbus_watch_get_data(watch));
  if (callback) {
    Impl *this_p = reinterpret_cast<Impl*>(data);
    ASSERT(this_p);
    callback->RemoveSelf(this_p->main_loop_);
  } else {
    DLOG("be called but the callback is NULL!");
  }
}

void DBusMainLoopClosure::Impl::WatchToggled(DBusWatch* watch,
                                             void* data) {
  DBusWatchCallBack* callback = reinterpret_cast<DBusWatchCallBack*>(
      dbus_watch_get_data(watch));
  if (callback)
    callback->SetEnabled(dbus_watch_get_enabled(watch));
  else
    DLOG("be called but the callback is NULL!");
}

class DBusMainLoopClosure::Impl::DBusTimeoutCallBack : public
                                                       WatchCallbackInterface {
 public:
  DBusTimeoutCallBack(DBusConnection* connection,
                      DBusTimeout* timeout) :
    connection_(connection), timeout_(timeout) {
    enabled_ = dbus_timeout_get_enabled(timeout);
  }
  virtual ~DBusTimeoutCallBack() {}
  virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
    DLOG("Call DBusTimeoutCallBack, watch id: %d", watch_id);
    if (dbus_connection_get_dispatch_status(connection_) !=
        DBUS_DISPATCH_COMPLETE) {
      dbus_connection_dispatch(connection_);
    }
    dbus_timeout_handle(timeout_);
    return true;
  }
  virtual void OnRemove(MainLoopInterface* main_loop, int watch_id) {
    DLOG("remove timeout call back.");
    delete this;
  }
  void SetEnabled(bool enabled) {
    enabled_ = enabled;
  }
  void AttachToMainLoop(MainLoopInterface* main_loop, int time_in_ms) {
    watch_id_ = main_loop->AddTimeoutWatch(time_in_ms, this);
  }
  void RemoveSelf(MainLoopInterface* main_loop) {
    main_loop->RemoveWatch(watch_id_);
  }
 private:
  DBusConnection* connection_;
  bool            enabled_;
  DBusTimeout*    timeout_;
  int             watch_id_;
};

dbus_bool_t DBusMainLoopClosure::Impl::AddTimeout(DBusTimeout *timeout,
                                                  void* data) {
  Impl *this_p = reinterpret_cast<Impl*>(data);
  ASSERT(this_p);
  int time_in_ms = dbus_timeout_get_interval(timeout);
  if (time_in_ms > 0) {
    DLOG("add timeout: %d ms.", time_in_ms);
    DBusTimeoutCallBack *callback =
        new DBusTimeoutCallBack(this_p->connection_, timeout);
    callback->AttachToMainLoop(this_p->main_loop_, time_in_ms);
    dbus_timeout_set_data(timeout, callback, NULL);
  }
  return TRUE;
}

void DBusMainLoopClosure::Impl::RemoveTimeout(DBusTimeout *timeout,
                                              void* data) {
  Impl *this_p = reinterpret_cast<Impl*>(data);
  ASSERT(this_p);
  DBusTimeoutCallBack *callback = reinterpret_cast<DBusTimeoutCallBack*>(
      dbus_timeout_get_data(timeout));

  if (callback) {
    DLOG("remove timeout: %p", callback);
    callback->RemoveSelf(this_p->main_loop_);
  }
}

void DBusMainLoopClosure::Impl::TimeoutToggled(DBusTimeout *timeout,
                                               void* data) {
  DBusTimeoutCallBack *callback = reinterpret_cast<DBusTimeoutCallBack*>(
      dbus_timeout_get_data(timeout));
  if (callback)
    callback->SetEnabled(dbus_timeout_get_enabled(timeout));
  else
    DLOG("be called but the callback is NULL!");
}

bool DBusMainLoopClosure::Impl::Setup() {
  dbus_connection_set_dispatch_status_function(connection_,
                                               DispatchStatusFunction,
                                               NULL, NULL);
  if (!dbus_connection_set_watch_functions(connection_,
                                           AddWatch,
                                           RemoveWatch,
                                           WatchToggled,
                                           this,
                                           NULL)) {
    LOG("no memory to set watch functions.");
    return false;
  }
  if (!dbus_connection_set_timeout_functions(connection_,
                                             AddTimeout,
                                             RemoveTimeout,
                                             TimeoutToggled,
                                             this,
                                             NULL)) {
    LOG("no memory to set timeout functions.");
    return false;
  }
  if (dbus_connection_get_dispatch_status(connection_) != DBUS_DISPATCH_COMPLETE)
    dbus_connection_dispatch(connection_);
  return true;
}

void DBusMainLoopClosure::Impl::RemoveFunctions() {
  dbus_connection_set_dispatch_status_function(connection_, NULL, NULL, NULL);
  dbus_connection_set_watch_functions(connection_,
                                      NULL, NULL, NULL, NULL, NULL);
  dbus_connection_set_timeout_functions(connection_,
                                        NULL, NULL, NULL, NULL, NULL);
}

DBusMainLoopClosure::DBusMainLoopClosure(DBusConnection* connection,
                                         MainLoopInterface* main_loop) :
    impl_(NULL) {
  impl_ = new Impl(connection, main_loop);
}

DBusMainLoopClosure::~DBusMainLoopClosure() {
  delete impl_;
}

}  // namespace dbus
}  // namespace ggadget
