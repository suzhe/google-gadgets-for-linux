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

#include <cstring>
#include <map>
#include <vector>
#include "scriptable_helper.h"
#include "logger.h"
#include "scriptable_holder.h"
#include "scriptable_interface.h"
#include "signals.h"
#include "slot.h"
#include "string_utils.h"

namespace ggadget {

// Define it to get verbose debug info about reference counting, especially
// useful when run with valgrind.
// #define VERBOSE_DEBUG_REF

namespace internal {

class ScriptableHelperImpl : public ScriptableHelperImplInterface {
 public:
  ScriptableHelperImpl(Slot0<void> *do_register);
  virtual ~ScriptableHelperImpl();

  virtual void RegisterProperty(const char *name, Slot *getter, Slot *setter);
  virtual void RegisterStringEnumProperty(const char *name,
                                          Slot *getter, Slot *setter,
                                          const char **names, int count);
  virtual void RegisterMethod(const char *name, Slot *slot);
  virtual void RegisterSignal(const char *name, Signal *signal);
  virtual void RegisterVariantConstant(const char *name, const Variant &value);
  virtual void SetInheritsFrom(ScriptableInterface *inherits_from);
  virtual void SetArrayHandler(Slot *getter, Slot *setter);
  virtual void SetDynamicPropertyHandler(Slot *getter, Slot *setter);

  // The following 3 methods declared in ScriptableInterface should never be
  // called.
  virtual uint64_t GetClassId() const { return 0; }
  virtual bool IsInstanceOf(uint64_t class_id) const {
    ASSERT(false); return false;
  }
  virtual bool IsStrict() const { ASSERT(false); return false; }

  virtual void Ref();
  virtual void Unref(bool transient);
  virtual int GetRefCount() const { return ref_count_; }

  virtual Connection *ConnectOnReferenceChange(Slot2<void, int, int> *slot);
  virtual PropertyType GetPropertyInfo(const char *name, Variant *prototype);
  virtual ResultVariant GetProperty(const char *name);
  virtual bool SetProperty(const char *name, const Variant &value);
  virtual ResultVariant GetPropertyByIndex(int index);
  virtual bool SetPropertyByIndex(int index, const Variant &value);

  virtual void SetPendingException(ScriptableInterface *exception);
  virtual ScriptableInterface *GetPendingException(bool clear);
  virtual bool EnumerateProperties(EnumeratePropertiesCallback *callback);
  virtual bool EnumerateElements(EnumerateElementsCallback *callback);

  virtual RegisterableInterface *GetRegisterable() { return this; }

 private:
  void EnsureRegistered();
  class InheritedPropertiesCallback;
  class InheritedElementsCallback;
  void AddPropertyInfo(const char *name, PropertyType type,
                       const Variant &prototype,
                       Slot *getter, Slot *setter);
  struct PropertyInfo {
    PropertyInfo() : type(PROPERTY_NOT_EXIST) {
      memset(&u, 0, sizeof(u));
    }

    void OnRefChange(int ref_count, int change) {
      // We have a similar mechanism in ScriptableHolder.
      // Please see the comments there.
      if (ref_count == 0 && change == 0) {
        ASSERT(u.scriptable_info.ref_change_connection &&
               u.scriptable_info.scriptable);
        u.scriptable_info.ref_change_connection->Disconnect();
        u.scriptable_info.ref_change_connection = NULL;
        u.scriptable_info.scriptable->Unref(true);
        u.scriptable_info.scriptable = NULL;
        prototype = Variant(static_cast<ScriptableInterface *>(NULL));
      }
    }

    PropertyType type;
    Variant prototype;
    union {
      // For normal properties.
      struct {
        Slot *getter, *setter;
      } slots;
      // For ScriptableInterface * constants. Not using ScriptableHolder to
      // make it possible to use union to save memory.
      struct {
        // This is a dup of the scriptable pointer stored in prototype, to
        // avoid virtual method call during destruction of the scriptable
        // object.
        ScriptableInterface *scriptable;
        Connection *ref_change_connection;
      } scriptable_info;
    } u;
  };
  // Because PropertyInfo is a copy-able struct, we must deallocate the
  // resource outside of the struct instead of in ~PropertyInfo().
  static void DestroyPropertyInfo(PropertyInfo *info);

  Slot0<void> *do_register_;
  int ref_count_;

  typedef std::map<const char *, PropertyInfo,
                   GadgetCharPtrComparator> PropertyInfoMap;

  // Index of property slots.  The keys are property names, and the values
  // are indexes into slot_prototypes_, getter_slots_ and setter_slots_.
  PropertyInfoMap property_info_map_;

  Signal2<void, int, int> on_reference_change_signal_;
  ScriptableInterface *inherits_from_;
  Slot *array_getter_;
  Slot *array_setter_;
  Slot *dynamic_property_getter_;
  Slot *dynamic_property_setter_;
  ScriptableInterface *pending_exception_;
};

ScriptableHelperImplInterface *NewScriptableHelperImpl(
    Slot0<void> *do_register) {
  return new ScriptableHelperImpl(do_register);
}

ScriptableHelperImpl::ScriptableHelperImpl(Slot0<void> *do_register)
    : do_register_(do_register),
      ref_count_(0),
      inherits_from_(NULL),
      array_getter_(NULL),
      array_setter_(NULL),
      dynamic_property_getter_(NULL),
      dynamic_property_setter_(NULL),
      pending_exception_(NULL) {
}

ScriptableHelperImpl::~ScriptableHelperImpl() {
  // Emit the ondelete signal, as early as possible.
  on_reference_change_signal_(0, 0);
  ASSERT(ref_count_ == 0);

  // Free all owned slots.
  for (PropertyInfoMap::iterator it = property_info_map_.begin();
       it != property_info_map_.end(); ++it) {
    DestroyPropertyInfo(&it->second);
  }

  delete array_getter_;
  delete array_setter_;
  delete dynamic_property_getter_;
  delete dynamic_property_setter_;
  delete do_register_;
}

void ScriptableHelperImpl::EnsureRegistered() {
  if (do_register_) {
    (*do_register_)();
    delete do_register_;
    do_register_ = NULL;
  }
}

void ScriptableHelperImpl::DestroyPropertyInfo(PropertyInfo *info) {
  if (info->prototype.type() == Variant::TYPE_SLOT)
    delete VariantValue<Slot *>()(info->prototype);

  if (info->type == PROPERTY_NORMAL) {
    delete info->u.slots.getter;
    delete info->u.slots.setter;
  } else if (info->type == PROPERTY_CONSTANT &&
             info->prototype.type() == Variant::TYPE_SCRIPTABLE) {
    if (info->u.scriptable_info.scriptable) {
      ASSERT(info->u.scriptable_info.ref_change_connection);
      info->u.scriptable_info.ref_change_connection->Disconnect();
      info->u.scriptable_info.ref_change_connection = NULL;
      info->u.scriptable_info.scriptable->Unref();
      info->u.scriptable_info.scriptable = NULL;
      info->prototype = Variant(static_cast<ScriptableInterface *>(NULL));
    }
  }
}

void ScriptableHelperImpl::AddPropertyInfo(const char *name, PropertyType type,
                                           const Variant &prototype,
                                           Slot *getter, Slot *setter) {
  PropertyInfo *info = &property_info_map_[name];
  if (info->type != PROPERTY_NOT_EXIST) {
    // A previously registered property is overriden.
    DestroyPropertyInfo(info);
  }
  info->type = type;
  info->prototype = prototype;

  if (type == PROPERTY_CONSTANT &&
      prototype.type() == Variant::TYPE_SCRIPTABLE) {
    ScriptableInterface *scriptable =
        VariantValue<ScriptableInterface *>()(prototype);
    if (scriptable) {
      info->u.scriptable_info.ref_change_connection =
          scriptable->ConnectOnReferenceChange(
              NewSlot(info, &PropertyInfo::OnRefChange));
      info->u.scriptable_info.scriptable = scriptable;
      scriptable->Ref();
    }
  } else {
    info->u.slots.getter = getter;
    info->u.slots.setter = setter;
  }
}

static Variant DummyGetter() {
  return Variant();
}

void ScriptableHelperImpl::RegisterProperty(const char *name,
                                            Slot *getter, Slot *setter) {
  ASSERT(name);
  Variant prototype;
  ASSERT(!setter || setter->GetArgCount() == 1);
  if (getter) {
    ASSERT(getter->GetArgCount() == 0);
    prototype = Variant(getter->GetReturnType());
    ASSERT(!setter || prototype.type() == setter->GetArgTypes()[0]);
  } else {
    getter = NewSlot(DummyGetter);
    if (setter)
      prototype = Variant(setter->GetArgTypes()[0]);

    if (prototype.type() == Variant::TYPE_SLOT) {
      DLOG("Warning: property '%s' is of type Slot, please make sure the return"
           " type of this Slot parameter is void or Variant, or use"
           " RegisterSignal instead.", name);
    }
  }

  AddPropertyInfo(name, PROPERTY_NORMAL, prototype, getter, setter);
}

class StringEnumGetter : public Slot0<const char *> {
 public:
  StringEnumGetter(Slot *slot, const char **names, int count)
      : slot_(slot), names_(names), count_(count) { }
  virtual ~StringEnumGetter() {
    delete slot_;
  }
  virtual ResultVariant Call(int argc, const Variant argv[]) const {
    int index = VariantValue<int>()(slot_->Call(0, NULL).v());
    return ResultVariant(index >= 0 && index < count_ ?
                         Variant(names_[index]) : Variant(""));
  }
  virtual bool operator==(const Slot &another) const {
    return false; // Not used.
  }
  Slot *slot_;
  const char **names_;
  int count_;
};

class StringEnumSetter : public Slot1<void, const char *> {
 public:
  StringEnumSetter(Slot *slot, const char **names, int count)
      : slot_(slot), names_(names), count_(count) { }
  virtual ~StringEnumSetter() {
    delete slot_;
  }
  virtual ResultVariant Call(int argc, const Variant argv[]) const {
    const char *name = VariantValue<const char *>()(argv[0]);
    for (int i = 0; i < count_; i++) {
      if (strcmp(name, names_[i]) == 0) {
        Variant param(i);
        slot_->Call(1, &param);
        return ResultVariant();
      }
    }
    LOG("Invalid enumerated name: %s", name);
    return ResultVariant();
  }
  virtual bool operator==(const Slot &another) const {
    return false; // Not used.
  }
  Slot *slot_;
  const char **names_;
  int count_;
};

void ScriptableHelperImpl::RegisterStringEnumProperty(
    const char *name, Slot *getter, Slot *setter,
    const char **names, int count) {
  ASSERT(getter);
  RegisterProperty(name, new StringEnumGetter(getter, names, count),
                   setter ? new StringEnumSetter(setter, names, count) : NULL);
}

void ScriptableHelperImpl::RegisterMethod(const char *name, Slot *slot) {
  ASSERT(name);
  ASSERT(slot && slot->HasMetadata());
  AddPropertyInfo(name, PROPERTY_METHOD, Variant(slot), NULL, NULL);
}

void ScriptableHelperImpl::RegisterSignal(const char *name, Signal *signal) {
  ASSERT(name);
  ASSERT(signal);

  // Create a SignalSlot as the value of the prototype to let others know
  // the calling convention.  It is owned by slot_prototypes.
  Variant prototype = Variant(new SignalSlot(signal));
  // Allocate an initially unconnected connection.  This connection is
  // dedicated to be used by the script.
  Connection *connection = signal->ConnectGeneral(NULL);
  // The getter returns the connected slot of the connection.
  Slot *getter = NewSlot(connection, &Connection::slot);
  // The setter accepts a Slot * parameter and connect it to the signal.
  Slot *setter = NewSlot(connection, &Connection::Reconnect);

  AddPropertyInfo(name, PROPERTY_NORMAL, prototype, getter, setter);
}

void ScriptableHelperImpl::RegisterVariantConstant(const char *name,
                                                   const Variant &value) {
  ASSERT(name);
  ASSERT_M(value.type() != Variant::TYPE_SLOT,
           ("Don't register Slot constant. Use RegisterMethod instead."));
  AddPropertyInfo(name, PROPERTY_CONSTANT, value, NULL, NULL);
}

void ScriptableHelperImpl::SetInheritsFrom(
    ScriptableInterface *inherits_from) {
  inherits_from_ = inherits_from;
}

void ScriptableHelperImpl::SetArrayHandler(Slot *getter, Slot *setter) {
  ASSERT(getter && getter->GetArgCount() == 1 &&
         getter->GetArgTypes()[0] == Variant::TYPE_INT64);
  ASSERT(!setter || (setter->GetArgCount() == 2 &&
                     setter->GetArgTypes()[0] == Variant::TYPE_INT64 &&
                     setter->GetReturnType() == Variant::TYPE_BOOL));
  array_getter_ = getter;
  array_setter_ = setter;
}

void ScriptableHelperImpl::SetDynamicPropertyHandler(
    Slot *getter, Slot *setter) {
  ASSERT(getter && getter->GetArgCount() == 1 &&
         getter->GetArgTypes()[0] == Variant::TYPE_STRING);
  ASSERT(!setter || (setter->GetArgCount() == 2 &&
                     setter->GetArgTypes()[0] == Variant::TYPE_STRING &&
                     setter->GetReturnType() == Variant::TYPE_BOOL));
  dynamic_property_getter_ = getter;
  dynamic_property_setter_ = setter;
}

void ScriptableHelperImpl::Ref() {
#ifdef VERBOSE_DEBUG_REF
  DLOG("Ref ref_count_ = %d", ref_count_);
#endif
  ASSERT(ref_count_ >= 0);
  on_reference_change_signal_(ref_count_, 1);
  ref_count_++;
}

void ScriptableHelperImpl::Unref(bool transient) {
  // The parameter traisnent is ignored here. Let the ScriptableHelper
  // template deal with it.
#ifdef VERBOSE_DEBUG_REF
  DLOG("Unref ref_count_ = %d", ref_count_);
#endif
  ASSERT(ref_count_ > 0);
  on_reference_change_signal_(ref_count_, -1);
  ref_count_--;
}

Connection *ScriptableHelperImpl::ConnectOnReferenceChange(
    Slot2<void, int, int> *slot) {
  return on_reference_change_signal_.Connect(slot);
}

ScriptableInterface::PropertyType ScriptableHelperImpl::GetPropertyInfo(
    const char *name, Variant *prototype) {
  EnsureRegistered();
  PropertyInfoMap::const_iterator it = property_info_map_.find(name);
  if (it != property_info_map_.end()) {
    if (prototype)
      *prototype = it->second.prototype;
    return it->second.type;
  }

  // Try dynamic properties.
  if (dynamic_property_getter_) {
    Variant param(name);
    Variant dynamic_value = dynamic_property_getter_->Call(1, &param).v();
    if (dynamic_value.type() != Variant::TYPE_VOID) {
      if (prototype)
        *prototype = Variant(dynamic_value.type());
      return PROPERTY_DYNAMIC;
    }
  }

  // Try inherited properties.
  if (inherits_from_)
    return inherits_from_->GetPropertyInfo(name, prototype);
  return PROPERTY_NOT_EXIST;
}

// NOTE: Must be exception-safe because the handler may throw exceptions.
ResultVariant ScriptableHelperImpl::GetProperty(const char *name) {
  EnsureRegistered();
  PropertyInfoMap::const_iterator it = property_info_map_.find(name);
  if (it != property_info_map_.end()) {
    const PropertyInfo *info = &it->second;
    switch (info->type) {
      case PROPERTY_NORMAL:
        ASSERT(info->u.slots.getter);
        return info->u.slots.getter->Call(0, NULL);
      case PROPERTY_CONSTANT:
      case PROPERTY_METHOD:
        return ResultVariant(info->prototype);
      default:
        ASSERT(false);
        break;
    }
  } else {
    if (dynamic_property_getter_) {
      Variant param(name);
      ResultVariant result = dynamic_property_getter_->Call(1, &param);
      if (result.v().type() != Variant::TYPE_VOID)
        return result;
    }
    if (inherits_from_)
      return inherits_from_->GetProperty(name);
  }
  return ResultVariant();
}

// NOTE: Must be exception-safe because the handler may throw exceptions.
bool ScriptableHelperImpl::SetProperty(const char *name,
                                       const Variant &value) {
  EnsureRegistered();
  PropertyInfoMap::const_iterator it = property_info_map_.find(name);
  if (it != property_info_map_.end()) {
    const PropertyInfo *info = &it->second;
    switch (info->type) {
      case PROPERTY_NORMAL:
        if (info->u.slots.setter) {
          info->u.slots.setter->Call(1, &value);
          return true;
        }
        return false;
      case PROPERTY_CONSTANT:
      case PROPERTY_METHOD:
        return false;
      default:
        ASSERT(false);
        break;
    }
  } else {
    if (dynamic_property_setter_) {
      Variant params[] = { Variant(name), value };
      Variant result = dynamic_property_setter_->Call(2, params).v();
      ASSERT(result.type() == Variant::TYPE_BOOL);
      if (VariantValue<bool>()(result))
        return true;
    }
    if (inherits_from_ && inherits_from_->SetProperty(name, value))
      return true;
  }
  return false;
}

// NOTE: Must be exception-safe because the handler may throw exceptions.
ResultVariant ScriptableHelperImpl::GetPropertyByIndex(int index) {
  EnsureRegistered();
  if (array_getter_) {
    Variant param(index);
    return array_getter_->Call(1, &param);
  }
  return ResultVariant();
}

// NOTE: Must be exception-safe because the handler may throw exceptions.
bool ScriptableHelperImpl::SetPropertyByIndex(int index,
                                              const Variant &value) {
  EnsureRegistered();
  if (array_setter_) {
    Variant params[] = { Variant(index), value };
    Variant result = array_setter_->Call(2, params).v();
    ASSERT(result.type() == Variant::TYPE_BOOL);
    return VariantValue<bool>()(result);
  }
  return false;
}

void ScriptableHelperImpl::SetPendingException(ScriptableInterface *exception) {
  ASSERT(pending_exception_ == NULL);
  pending_exception_ = exception;
}

ScriptableInterface *ScriptableHelperImpl::GetPendingException(bool clear) {
  ScriptableInterface *result = pending_exception_;
  if (clear)
    pending_exception_ = NULL;
  return result;
}

class ScriptableHelperImpl::InheritedPropertiesCallback {
 public:
  InheritedPropertiesCallback(ScriptableHelperImpl *owner,
                              EnumeratePropertiesCallback *callback)
      : owner_(owner), callback_(callback) {
  }

  bool Callback(const char *name, PropertyType type, const Variant &value) {
    if (owner_->property_info_map_.find(name) ==
        owner_->property_info_map_.end()) {
      // Only emunerate inherited properties which are not overriden by this
      // scriptable object.
      return (*callback_)(name, type, value);
    }
    return true;
  }

  ScriptableHelperImpl *owner_;
  EnumeratePropertiesCallback *callback_;
};

bool ScriptableHelperImpl::EnumerateProperties(
    EnumeratePropertiesCallback *callback) {
  ASSERT(callback);
  EnsureRegistered();

  if (inherits_from_) {
    InheritedPropertiesCallback inherited_callback(this, callback);
    if (!inherits_from_->EnumerateProperties(
        NewSlot(&inherited_callback, &InheritedPropertiesCallback::Callback))) {
      delete callback;
      return false;
    }
  }
  for (PropertyInfoMap::const_iterator it = property_info_map_.begin();
       it != property_info_map_.end(); ++it) {
    ResultVariant value = GetProperty(it->first);
    if (!(*callback)(it->first, it->second.type, value.v())) {
      delete callback;
      return false;
    }
  }
  delete callback;
  return true;
}

bool ScriptableHelperImpl::EnumerateElements(
    EnumerateElementsCallback *callback) {
  // This helper does nothing.
  delete callback;
  return true;
}

} // namespace internal

} // namespace ggadget
