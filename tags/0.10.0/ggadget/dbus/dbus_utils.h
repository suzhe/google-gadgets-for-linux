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

#ifndef GGADGET_DBUS_DBUS_UTILS_H__
#define GGADGET_DBUS_DBUS_UTILS_H__

#include <map>
#include <vector>
#include <string>

#include "dbus_proxy.h"
#include <ggadget/common.h>
#include <ggadget/scriptable_helper.h>

class DBusConnection;
class DBusMessage;

namespace ggadget {

class MainLoopInterface;

namespace dbus {

/**
 * Container object to hold value transfered between DBusProxy and the caller.
 */
class ScriptableDBusContainer : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x7829c86eb35a4168, ScriptableInterface);
  ScriptableDBusContainer() : array_(NULL), count_(0) {}
  ScriptableDBusContainer(Variant *start, std::size_t size)
      : array_(start), count_(size) {
    AddArray(start, size);
  }
  virtual ~ScriptableDBusContainer() {
    delete [] array_;
  }

  /**
   * Don't use @c RegisterConstant() directly, since we want to register
   * constant properties by dynamic char pointer. This case is not applicable
   * for the map used in scriptable_helper object, which use const char*
   * not std::string as the key.
   */
  void AddProperty(const char *name, const Variant &value) {
    if (!name || *name == 0) return;
    keys_.push_back(name);
    RegisterConstant((keys_.end() - 1)->c_str(), value);
  }

  /** Creates a @c ScriptableArray with an iterator and count. */
  template <typename Iterator>
  void AddArray(Iterator start, size_t count) {
    Variant *variant_array = new Variant[count];
    for (size_t i = 0; i < count; i++)
      variant_array[i] = Variant(*start++);
    AddArray(variant_array, count);
  }

  /**
   * Add array with an iterator and count, the object will own the array, and
   * delete it when finalized.
   */
  void AddArray(Variant *start, size_t count) {
    if (!start) return;
    if (array_ && array_ != start) delete [] array_;
    array_ = start;
    count_ = count;
    RegisterConstant("length", count);
  }

  bool EnumerateElements(EnumerateElementsCallback *callback) {
    ASSERT(callback);
    for (size_t i = 0; i < count_; i++)
      if (!(*callback)(static_cast<int>(i), array_[i])) {
        delete callback;
        return false;
      }
    delete callback;
    return true;
  }

 private:
  std::vector<std::string> keys_;
  Variant *array_;
  std::size_t count_;
};

std::string GetVariantSignature(const Variant &value);

typedef std::vector<std::string> StringList;
class ArrayIterator {
 public:
  ArrayIterator() : is_array_(true) {}
  std::string signature() const {
    if (signature_list_.empty()) return "";
    if (is_array_) return std::string("a") + signature_list_[0];
    std::string sig = "(";
    for (StringList::const_iterator it = signature_list_.begin();
         it != signature_list_.end(); ++it)
      sig += *it;
    sig += ")";
    return sig;
  }
  bool Callback(int id, const Variant &value) {
    std::string sig = GetVariantSignature(value);
    if (sig.empty()) return true;
    if (is_array_ && !signature_list_.empty() && sig != signature_list_[0])
      is_array_ = false;
    signature_list_.push_back(sig);
    return true;
  }
 private:
  bool is_array_;
  StringList signature_list_;
};

class DictIterator {
 public:
  std::string signature() const { return signature_; }
  bool Callback(const char *name, ScriptableInterface::PropertyType type,
                const Variant &value) {
    if (type == ScriptableInterface::PROPERTY_METHOD ||
        value.type() == Variant::TYPE_VOID) {
      // Ignore method and void type properties.
      return true;
    }
    std::string sig = GetVariantSignature(value);
    if (signature_.empty()) {
      signature_ = sig;
    } else if (signature_ != sig) {
      return false;
    }
    return true;
  }
 private:
  std::string signature_;
};

struct Argument {
  Argument() {}
  explicit Argument(const Variant& v) : value(v) {}
  explicit Argument(const char *sig) : signature(sig) {}
  Argument(const char *n, const char *sig) : name(n), signature(sig) {}
  Argument(const char *sig, const Variant &v) : signature(sig), value(v) {}
  bool operator!=(const Argument& another) const {
    return signature != another.signature;
  }

  std::string name;
  std::string signature;
  Variant value;
};
typedef std::vector<Argument> Arguments;

struct Prototype {
  explicit Prototype(const char *n) : name(n) {}
  std::string name;
  Arguments in_args;
  Arguments out_args;
};
typedef std::vector<Prototype> PrototypeVector;

/**
 * Marshaller for DBusMessage. Not a external API, user should not use it
 * directly.
 */
class DBusMarshaller {
 public:
  explicit DBusMarshaller(DBusMessage *message);
  ~DBusMarshaller();

  bool AppendArguments(const Arguments &args);
  bool AppendArgument(const Argument &arg);

  static bool ValistAdaptor(Arguments *in_args,
                            MessageType first_arg_type, va_list *va_args);
  static bool ValistToAugrments(Arguments *out_args,
                                MessageType first_arg_type,
                                va_list *va_args);
 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(DBusMarshaller);
};

/**
 * Marshaller for DBusMessage. Not a external API, user should not use it
 * directly.
 */
class DBusDemarshaller {
 public:
  explicit DBusDemarshaller(DBusMessage *message);
  ~DBusDemarshaller();

  bool GetArguments(Arguments *args);
  bool GetArgument(Argument *arg);

  static bool ValistAdaptor(const Arguments &out_args,
                            MessageType first_arg_type, va_list *va_args);
 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(DBusDemarshaller);
};

/**
 * a wrapper of DBusConnection for working with our MainLoopInterface.
 */
class DBusMainLoopClosure {
 public:
  DBusMainLoopClosure(DBusConnection* connection,
                      MainLoopInterface* main_loop);
  ~DBusMainLoopClosure();
 private:
  class Impl;
  Impl *impl_;
};

}  // namespace dbus
}  // namespace ggadget

#endif  // GGADGET_DBUS_DBUS_UTILS_H__
