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

#include <cstring>
#include <map>
#include <vector>
#include "scriptable_helper.h"
#include "logger.h"
#include "scriptable_interface.h"
#include "signals.h"
#include "slot.h"
#include "string_utils.h"

namespace ggadget {
namespace internal {

class ScriptableHelperImpl : public ScriptableHelperImplInterface {
 public:
  ScriptableHelperImpl();
  virtual ~ScriptableHelperImpl();

  virtual void RegisterProperty(const char *name, Slot *getter, Slot *setter);
  virtual void RegisterStringEnumProperty(const char *name,
                                          Slot *getter, Slot *setter,
                                          const char **names, int count);
  virtual void RegisterMethod(const char *name, Slot *slot);
  virtual void RegisterSignal(const char *name, Signal *signal);
  virtual void RegisterConstants(int count,
                                 const char * const names[],
                                 const Variant values[]);
  virtual void SetPrototype(ScriptableInterface *prototype);
  virtual void SetArrayHandler(Slot *getter, Slot *setter);
  virtual void SetDynamicPropertyHandler(Slot *getter, Slot *setter);

  // The following 5 methods declared in ScriptableInterface should never be
  // called.
  virtual uint64_t GetClassId() const { return 0; }
  virtual bool IsInstanceOf(uint64_t class_id) const {
    ASSERT(false); return false;
  }
  virtual bool IsStrict() const { ASSERT(false); return false; }
  virtual OwnershipPolicy Attach() { ASSERT(false); return NATIVE_OWNED; }
  virtual bool Detach() { ASSERT(false); return false; }

  virtual Connection *ConnectToOnDeleteSignal(Slot0<void> *slot);
  virtual bool GetPropertyInfoByName(const char *name,
                                     int *id, Variant *prototype,
                                     bool *is_method);
  virtual bool GetPropertyInfoById(int id, Variant *prototype,
                                   bool *is_method, const char **name);
  virtual Variant GetProperty(int id);
  virtual bool SetProperty(int id, const Variant &value);

  virtual void SetPendingException(ScriptableInterface *exception);
  virtual ScriptableInterface *GetPendingException(bool clear);
  virtual bool EnumerateProperties(EnumeratePropertiesCallback *callback);
  virtual bool EnumerateElements(EnumerateElementsCallback *callback);

 private:
  class PrototypePropertiesCallback;
  class PrototypeElementsCallback;
  void AddPropertyInfo(const char *name, const Variant &prototype,
                       Slot *getter, Slot *setter);

  typedef std::map<const char *, int, GadgetCharPtrComparator> SlotIndexMap;
  typedef std::vector<Variant> VariantVector;
  typedef std::vector<Slot *> SlotVector;
  typedef std::vector<const char *> NameVector;
  typedef std::map<const char *, Variant, GadgetCharPtrComparator> ConstantMap;

  // If true, no more new RegisterXXX or SetPrototype can be called.
  // It'll be set to true in any ScriptableInterface operation on properties. 
  bool sealed_;

  // Index of property slots.  The keys are property names, and the values
  // are indexes into slot_prototype_, getter_slots_ and setter_slots_.
  SlotIndexMap slot_index_;
  VariantVector slot_prototypes_;
  SlotVector getter_slots_;
  SlotVector setter_slots_;
  NameVector slot_names_;
  SlotVector extra_slots_;

  // Redundant value to simplify code.
  // It should always equal to the size of above collections.
  int property_count_;

  // Containing constant definitions.  The keys are property names, and the
  // values are constant values.
  ConstantMap constants_;

  Signal0<void> ondelete_signal_;
  ScriptableInterface *prototype_;
  Slot *array_getter_;
  Slot *array_setter_;
  Slot *dynamic_property_getter_;
  Slot *dynamic_property_setter_;
  const char *last_dynamic_or_constant_name_;
  Variant last_dynamic_or_constant_value_;

  ScriptableInterface *pending_exception_;
};

ScriptableHelperImplInterface *NewScriptableHelperImpl() {
  return new ScriptableHelperImpl();
}

ScriptableHelperImpl::ScriptableHelperImpl()
    : sealed_(false),
      property_count_(0),
      prototype_(NULL),
      array_getter_(NULL),
      array_setter_(NULL),
      dynamic_property_getter_(NULL),
      dynamic_property_setter_(NULL),
      last_dynamic_or_constant_name_(NULL),
      pending_exception_(NULL) {
}

ScriptableHelperImpl::~ScriptableHelperImpl() {
  // Emit the ondelete signal, as early as possible.
  ondelete_signal_();

  // Free all owned slots.
  for (VariantVector::const_iterator it = slot_prototypes_.begin();
       it != slot_prototypes_.end(); ++it) {
    if (it->type() == Variant::TYPE_SLOT)
      delete VariantValue<Slot *>()(*it);
  }

  for (SlotVector::const_iterator it = getter_slots_.begin();
       it != getter_slots_.end(); ++it) {
    delete *it;
  }

  for (SlotVector::const_iterator it = setter_slots_.begin();
       it != setter_slots_.end(); ++it) {
    delete *it;
  }

  for (SlotVector::const_iterator it = extra_slots_.begin();
       it != extra_slots_.end(); ++it) {
    delete *it;
  }

  delete array_getter_;
  delete array_setter_;
  delete dynamic_property_getter_;
  delete dynamic_property_setter_;
}

void ScriptableHelperImpl::AddPropertyInfo(const char *name,
                                           const Variant &prototype,
                                           Slot *getter, Slot *setter) {
  SlotIndexMap::iterator it = slot_index_.find(name);
  if (it != slot_index_.end()) {
    int index = it->second;
    // DLOG("Property %s(%d) is overriden", name, index);
    slot_prototypes_[index] = prototype;
    delete getter_slots_[index];
    getter_slots_[index] = getter;
    delete setter_slots_[index];
    setter_slots_[index] = setter;
  } else {
    slot_index_[name] = property_count_;
    slot_prototypes_.push_back(prototype);
    getter_slots_.push_back(getter);
    setter_slots_.push_back(setter);
    slot_names_.push_back(name);
    property_count_++;
  }
  ASSERT(property_count_ == static_cast<int>(slot_index_.size()));
  ASSERT(property_count_ == static_cast<int>(slot_prototypes_.size()));
  ASSERT(property_count_ == static_cast<int>(slot_names_.size()));
  ASSERT(property_count_ == static_cast<int>(getter_slots_.size()));
  ASSERT(property_count_ == static_cast<int>(setter_slots_.size()));
}

void ScriptableHelperImpl::RegisterProperty(const char *name,
                                            Slot *getter, Slot *setter) {
  ASSERT(!sealed_);
  ASSERT(name);
  Variant prototype;
  ASSERT(!setter || setter->GetArgCount() == 1);
  if (getter) {
    ASSERT(getter->GetArgCount() == 0);
    ASSERT_M(getter->GetReturnType() != Variant::TYPE_CONST_SCRIPTABLE,
             ("Can't return 'const ScriptableInterface *' to script"));
    //Returning Slot * should be allowed, see Audioclip::GetOnStateChange().
    //ASSERT_M(getter->GetReturnType() != Variant::TYPE_SLOT,
    //         ("Can't return 'Slot *' (name: %s) to script", name));
    prototype = Variant(getter->GetReturnType());
    ASSERT(!setter || prototype.type() == setter->GetArgTypes()[0]);
  } else {
    ASSERT(setter);
    prototype = Variant(setter->GetArgTypes()[0]);
#ifdef _DEBUG
    if (prototype.type() == Variant::TYPE_SLOT) {
      LOG("Warning: property '%s' is of type Slot, please make sure the return"
          " type of this Slot parameter is void or Variant, or use"
          " RegisterSignal instead.", name);
    }
#endif
  }

  AddPropertyInfo(name, prototype, getter, setter);
}

class StringEnumGetter {
 public:
  StringEnumGetter(Slot *slot, const char **names, int count)
      : slot_(slot), names_(names), count_(count) { }
  const char *operator()() const {
    int index = VariantValue<int>()(slot_->Call(0, NULL));
    return (index >= 0 && index < count_) ? names_[index] : NULL;
  }
  bool operator==(const StringEnumGetter &another) const {
    return false;
  }
  Slot *slot_;
  const char **names_;
  int count_;
};

class StringEnumSetter {
 public:
  StringEnumSetter(Slot *slot, const char **names, int count)
      : slot_(slot), names_(names), count_(count) { }
  void operator()(const char *name) const {
    for (int i = 0; i < count_; i++)
      if (strcmp(name, names_[i]) == 0) {
        Variant param(i);
        slot_->Call(1, &param);
        return;
      }
    LOG("Invalid enumerated name: %s", name);
  }
  bool operator==(const StringEnumSetter &another) const {
    return false;
  }
  Slot *slot_;
  const char **names_;
  int count_;
};

void ScriptableHelperImpl::RegisterStringEnumProperty(
    const char *name, Slot *getter, Slot *setter,
    const char **names, int count) {
  ASSERT(getter);
  Slot *new_getter = NewFunctorSlot<const char *>(
      StringEnumGetter(getter, names, count));
  extra_slots_.push_back(getter);

  Slot *new_setter = NULL;
  if (setter) {
    new_setter = NewFunctorSlot<void, const char *>(
        StringEnumSetter(setter, names, count));
    extra_slots_.push_back(setter);
  }

  RegisterProperty(name, new_getter, new_setter);
}

void ScriptableHelperImpl::RegisterMethod(const char *name, Slot *slot) {
  ASSERT(!sealed_);
  ASSERT(name);
  ASSERT(slot && slot->HasMetadata());
  ASSERT_M(slot->GetReturnType() != Variant::TYPE_CONST_SCRIPTABLE,
           ("Can't return 'const ScriptableInterface *' to script"));
  ASSERT_M(slot->GetReturnType() != Variant::TYPE_SLOT,
           ("Can't return 'Slot *' to script"));
#ifdef _DEBUG
  for (int i = 0; i < slot->GetArgCount(); i++) {
    if (slot->GetArgTypes()[i] == Variant::TYPE_SLOT) {
      LOG("Warning: method '%s' has a parameter of type Slot, please make sure"
          " the return type of this Slot parameter is void or Variant.", name);
    }
  }
#endif

  AddPropertyInfo(name, Variant(slot), NULL, NULL);
}

void ScriptableHelperImpl::RegisterSignal(const char *name, Signal *signal) {
  ASSERT(!sealed_);
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

  AddPropertyInfo(name, prototype, getter, setter);
}

void ScriptableHelperImpl::RegisterConstants(int count,
                                             const char *const names[],
                                             const Variant values[]) {
  ASSERT(names);
  for (int i = 0; i < count; i++) {
    ASSERT(names[i]);
    constants_[names[i]] = values ? values[i] : Variant(i);
  }
}

void ScriptableHelperImpl::SetPrototype(ScriptableInterface *prototype) {
  ASSERT(!sealed_);
  prototype_ = prototype;
}

void ScriptableHelperImpl::SetArrayHandler(Slot *getter, Slot *setter) {
  ASSERT(!sealed_);
  ASSERT(getter && getter->GetArgCount() == 1 &&
         getter->GetArgTypes()[0] == Variant::TYPE_INT64);
  ASSERT(!setter || (setter->GetArgCount() == 2 &&
         setter->GetArgTypes()[0] == Variant::TYPE_INT64));
  array_getter_ = getter;
  array_setter_ = setter;
}

void ScriptableHelperImpl::SetDynamicPropertyHandler(
    Slot *getter, Slot *setter) {
  ASSERT(!sealed_);
  ASSERT(getter && getter->GetArgCount() == 1 &&
         getter->GetArgTypes()[0] == Variant::TYPE_STRING);
  ASSERT(!setter || (setter->GetArgCount() == 2 &&
         setter->GetArgTypes()[0] == Variant::TYPE_STRING));
  dynamic_property_getter_ = getter;
  dynamic_property_setter_ = setter;
}

Connection *ScriptableHelperImpl::ConnectToOnDeleteSignal(Slot0<void> *slot) {
  return ondelete_signal_.ConnectGeneral(slot);
}

// NOTE: Must be exception-safe because the handler may throw exceptions.
bool ScriptableHelperImpl::GetPropertyInfoByName(const char *name,
                                                 int *id, Variant *prototype,
                                                 bool *is_method) {
  ASSERT(name);
  ASSERT(id);
  ASSERT(prototype);
  ASSERT(is_method);
  sealed_ = true;

  // First check if the property is a constant.
  ConstantMap::const_iterator constants_it = constants_.find(name);
  if (constants_it != constants_.end()) {
    *id = ScriptableInterface::kConstantPropertyId;
    *prototype = constants_it->second;
    *is_method = false;
    last_dynamic_or_constant_name_ = name;
    last_dynamic_or_constant_value_ = *prototype;
    return true;
  }

  // Find the index by name.
  SlotIndexMap::const_iterator slot_index_it = slot_index_.find(name);
  if (slot_index_it != slot_index_.end()) {
    int index = slot_index_it->second;
    // 0, 1, 2, ... ==> -1, -2, -3, ... to distinguish property ids from
    // array indexes.
    *id = -(index + 1);
    *prototype = slot_prototypes_[index];
    *is_method = (getter_slots_[index] == NULL && setter_slots_[index] == NULL);
    return true;
  }

  // Not found in registered properties, try dynamic property getter.
  if (dynamic_property_getter_) {
    Variant param(name);
    last_dynamic_or_constant_value_ = dynamic_property_getter_->Call(1, &param);
    if (last_dynamic_or_constant_value_.type() != Variant::TYPE_VOID) {
      *id = ScriptableInterface::kDynamicPropertyId;
      last_dynamic_or_constant_name_ = name;
      *prototype = last_dynamic_or_constant_value_;
      *is_method = false;
      return true;
    }
  }

  // Try prototype finally.
  if (prototype_) {
    bool result = prototype_->GetPropertyInfoByName(name, id,
                                                    prototype, is_method);
    // Make the id distinct.
    if (result) {
      if (*id == kConstantPropertyId || *id == kDynamicPropertyId) {
        last_dynamic_or_constant_name_ = name;
        last_dynamic_or_constant_value_ = *prototype;
      } else {
        *id -= property_count_;
      }
    }
    return result;
  }

  return false;
}

bool ScriptableHelperImpl::GetPropertyInfoById(int id, Variant *prototype,
                                               bool *is_method,
                                               const char **name) {
  ASSERT(prototype);
  ASSERT(is_method);
  ASSERT(id != kDynamicPropertyId && id != kConstantPropertyId);
  sealed_ = true;

  if (id >= 0) {
    if (array_getter_) {
      Variant params[] = { Variant(id) };
      *prototype = array_getter_->Call(1, params);
      *is_method = false;
      return true;
    }
    return false;
  }

  // -1, -2, -3, ... ==> 0, 1, 2, ...
  int index = -id - 1;
  if (index >= property_count_) {
    if (prototype_)
      return prototype_->GetPropertyInfoById(id + property_count_,
                                             prototype, is_method, name);
    else
      return false;
  }

  *prototype = slot_prototypes_[index];
  *is_method = (getter_slots_[index] == NULL && setter_slots_[index] == NULL);
  *name = slot_names_[index];
  return true;
}

// NOTE: Must be exception-safe because the handler may throw exceptions.
Variant ScriptableHelperImpl::GetProperty(int id) {
  sealed_ = true;
  if (id >= 0) {
    // The id is an array index.
    if (array_getter_) {
      Variant params[] = { Variant(id) };
      return array_getter_->Call(1, params);
    }
    // Array index is not supported.
    return Variant();
  }

  if (id == ScriptableInterface::kConstantPropertyId ||
      id == ScriptableInterface::kDynamicPropertyId) {
    // We require the script engine call GetProperty immediately after calling
    // GetPropertyInfoByName() if the returned id is kDynamicPropertyId.
    // Here return the value cached in GetPropertyInfoByName().
    // For constants, GetProperty returns the last cached constant value.
    return last_dynamic_or_constant_value_;
  }

  // -1, -2, -3, ... ==> 0, 1, 2, ...
  int index = -id - 1;
  if (index >= property_count_) {
    if (prototype_)
      return prototype_->GetProperty(id + property_count_);
    else
      return Variant();
  }

  Slot *slot = getter_slots_[index];
  if (slot == NULL)
    // This property is a method, return the prototype.
    // Normally won't reach here, because the script engine will handle
    // method properties.
    return slot_prototypes_[index];

  return slot->Call(0, NULL);
}

// NOTE: Must be exception-safe because the handler may throw exceptions.
bool ScriptableHelperImpl::SetProperty(int id, const Variant &value) {
  sealed_ = true;
  if (id >= 0) {
    // The id is an array index.
    if (array_getter_) {
      Variant params[] = { Variant(id), value };
      Variant result = array_setter_->Call(2, params);
      if (result.type() == Variant::TYPE_VOID)
        return true;
      else
        return VariantValue<bool>()(result);
    }
    // Array index is not supported.
    return false;
  }

  if (id == ScriptableInterface::kConstantPropertyId)
    return false;

  if (id == ScriptableInterface::kDynamicPropertyId) {
    // We require the script engine call GetProperty immediately after calling
    // GetPropertyInfoByName() if the returned id is ID_DYNAMIC_PROPERTY.
    ASSERT(dynamic_property_getter_);
    ASSERT(last_dynamic_or_constant_name_);
    if (dynamic_property_setter_) {
      Variant params[] = { Variant(last_dynamic_or_constant_name_), value };
      Variant result = dynamic_property_setter_->Call(2, params);
      if (result.type() == Variant::TYPE_VOID)
        return true;
      else
        return VariantValue<bool>()(result);
    }
    // Dynamic properties are readonly.
    return false;
  }

  int index = -id - 1;
  if (index >= property_count_) {
    if (prototype_)
      return prototype_->SetProperty(id + property_count_, value);
    else
      return false;
  }

  Slot *slot = setter_slots_[index];
  if (slot == NULL)
    return false;

  slot->Call(1, &value);
  return true;
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

class ScriptableHelperImpl::PrototypePropertiesCallback {
 public:
  PrototypePropertiesCallback(ScriptableHelperImpl *owner,
                              EnumeratePropertiesCallback *callback)
      : owner_(owner), callback_(callback) {
  }

  bool Callback(int id, const char *name,
                const Variant &value, bool is_method) {
    if (owner_->slot_index_.find(name) == owner_->slot_index_.end() &&
        owner_->constants_.find(name) == owner_->constants_.end())
      return (*callback_)(id, name, value, is_method);
    return true;
  }

  ScriptableHelperImpl *owner_;
  EnumeratePropertiesCallback *callback_;
};

bool ScriptableHelperImpl::EnumerateProperties(
    EnumeratePropertiesCallback *callback) {
  ASSERT(callback);
  if (prototype_) {
    PrototypePropertiesCallback prototype_callback(this, callback);
    if (!prototype_->EnumerateProperties(
        NewSlot(&prototype_callback, &PrototypePropertiesCallback::Callback))) {
      delete callback;
      return false;
    }
  }
  for (ConstantMap::const_iterator it = constants_.begin();
       it != constants_.end(); ++it) {
    if (!(*callback)(kConstantPropertyId, it->first, it->second, false)) {
      delete callback;
      return false;
    }
  }
  for (NameVector::const_iterator it = slot_names_.begin();
       it != slot_names_.end(); ++it) {
    if (constants_.find(*it) == constants_.end()) {
      int id;
      Variant prototype;
      bool is_method;
      if (GetPropertyInfoByName(*it, &id, &prototype, &is_method)) {
        Variant value = GetProperty(id);
        if (!(*callback)(id, *it, value, is_method)) {
          delete callback;
          return false;
        }
      }
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

Variant GetPropertyByName(ScriptableInterface *scriptable, const char *name) {
  int id;
  Variant prototype;
  bool is_method;
  Variant result;
  if (scriptable->GetPropertyInfoByName(name, &id, &prototype, &is_method))
    result = scriptable->GetProperty(id);
  return result;
}

} // namespace ggadget
