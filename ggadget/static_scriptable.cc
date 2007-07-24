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

#include <string.h>
#include <map>
#include <vector>
#include "static_scriptable.h"
#include "signal.h"
#include "signal_consts.h"
#include "slot.h"

namespace ggadget {

class StaticScriptable::Impl {
 public:
  Impl();
  ~Impl();

  void RegisterProperty(const char *name, Slot *getter, Slot *setter);
  void RegisterMethod(const char *name, Slot *slot);
  void RegisterSignal(const char *name, Signal *signal);
  Connection *ConnectToOnDeleteSignal(Slot0<void> *slot);
  bool GetPropertyInfoByName(const char *name,
                             int *id, Variant *prototype,
                             bool *is_method);
  bool GetPropertyInfoById(int id, Variant *prototype, bool *is_method);
  Variant GetProperty(int id);
  bool SetProperty(int id, Variant value);

 private:
  struct CompareString {
    bool operator() (const char* s1, const char* s2) const {
      return (strcmp(s1, s2) < 0);
    }
  };

  typedef std::map<const char *, int, CompareString> SlotIndexMap;
  typedef std::vector<Variant> VariantVector;
  typedef std::vector<Slot *> SlotVector;

  SlotIndexMap slot_index_;
  VariantVector slot_prototypes_;
  SlotVector getter_slots_;
  SlotVector setter_slots_;
  Signal0<void> *ondelete_signal_;
};

StaticScriptable::Impl::Impl()
    : ondelete_signal_(NULL) {
}

StaticScriptable::Impl::~Impl() {
  // Free all owned slots.
  for (VariantVector::iterator it = slot_prototypes_.begin();
       it != slot_prototypes_.end(); ++it) {
    if (it->type == Variant::TYPE_SLOT)
      delete it->v.slot_value;
  }

  for (SlotVector::iterator it = getter_slots_.begin();
       it != getter_slots_.end(); ++it) {
    if (*it != NULL)
      delete *it;
  }

  for (SlotVector::iterator it = setter_slots_.begin();
       it != setter_slots_.end(); ++it) {
    if (*it != NULL)
      delete *it;
  }
}

void StaticScriptable::Impl::RegisterProperty(const char *name,
                                              Slot *getter, Slot *setter) {
  ASSERT(name);
  ASSERT(getter && getter->GetArgCount() == 0);
  Variant prototype(getter->GetReturnType());
  if (setter) {
    ASSERT(setter->GetArgCount() == 1);
    ASSERT(prototype.type == setter->GetArgTypes()[0]);
  }

  slot_index_[name] = slot_prototypes_.size();
  slot_prototypes_.push_back(prototype);
  getter_slots_.push_back(getter);
  setter_slots_.push_back(setter);
}

void StaticScriptable::Impl::RegisterMethod(const char *name, Slot *slot) {
  ASSERT(name);
  ASSERT(slot);

  slot_index_[name] = slot_prototypes_.size();
  slot_prototypes_.push_back(Variant(slot));
  getter_slots_.push_back(NULL);
  setter_slots_.push_back(NULL);
}

void StaticScriptable::Impl::RegisterSignal(const char *name, Signal *signal) {
  ASSERT(name);
  ASSERT(signal);

  if (strcmp(name, kOnDeleteSignal) == 0)
    ondelete_signal_ = down_cast<OnDeleteSignal *>(signal);

  slot_index_[name] = slot_prototypes_.size();

  // Create a SignalSlot as the value of the prototype to let others know
  // the calling convention.  It is owned by slot_prototypes.
  slot_prototypes_.push_back(Variant(new SignalSlot(signal)));

  // Allocate an initially unconnected connection.  This connection is
  // dedicated to be used by the script.
  Connection *connection = signal->ConnectGeneral(NULL);
  // The getter returns the connected slot of the connection.
  getter_slots_.push_back(NewSlot(connection, &Connection::slot));
  // The setter accepts a Slot * parameter and connect it to the signal.
  setter_slots_.push_back(NewSlot(connection, &Connection::Reconnect));
}

Connection *StaticScriptable::Impl::ConnectToOnDeleteSignal(Slot0<void> *slot) {
  if (ondelete_signal_)
    return ondelete_signal_->ConnectGeneral(slot);
  else
    return NULL;
}

bool StaticScriptable::Impl::GetPropertyInfoByName(const char *name,
                                                   int *id, Variant *prototype,
                                                   bool *is_method) {
  ASSERT(name);
  ASSERT(id);
  ASSERT(prototype);
  ASSERT(is_method);

  SlotIndexMap::iterator it = slot_index_.find(name);
  if (it == slot_index_.end())
    return false;

  int index = it->second;
  // 0, 1, 2, ... ==> -1, -2, -3, ... to distinguish property ids from
  // array indexes.
  *id = -(index + 1);
  *prototype = slot_prototypes_[index];
  *is_method = getter_slots_[index] == NULL;
  return true;
}

bool StaticScriptable::Impl::GetPropertyInfoById(int id, Variant *prototype,
                                                 bool *is_method) {
  ASSERT(prototype);
  ASSERT(is_method);

  // Array index is not supported here.
  if (id >= 0)
    return false;

  // -1, -2, -3, ... ==> 0, 1, 2, ...
  int index = -id - 1;
  if (index >= slot_prototypes_.size())
    return false;

  *prototype = slot_prototypes_[index];
  *is_method = getter_slots_[index] == NULL;
  return true;
}

Variant StaticScriptable::Impl::GetProperty(int id) {
  // Array index is not supported here.
  if (id >= 0)
    return Variant();

  // -1, -2, -3, ... ==> 0, 1, 2, ...
  int index = -id - 1;
  if (index >= getter_slots_.size())
    return Variant();

  Slot *slot = getter_slots_[index];
  if (slot == NULL)
    // This property is a method, return the prototype.
    // Normally won't reach here, because the script engine will handle
    // method properties.
    return slot_prototypes_[index];

  return slot->Call(0, NULL);
}

bool StaticScriptable::Impl::SetProperty(int id, Variant value) {
  // Array index is not supported here.
  if (id >= 0)
    return false;

  int index = -id - 1;
  if (index >= setter_slots_.size())
    return false;

  Slot *slot = setter_slots_[index];
  if (slot == NULL)
    return false;

  slot->Call(1, &value);
  return true;
}

// All StaticScriptable operations are delegated to impl_.
StaticScriptable::StaticScriptable()
    : impl_(new Impl()) {
}

StaticScriptable::~StaticScriptable() {
  delete impl_;
}

void StaticScriptable::RegisterProperty(const char *name,
                                        Slot *getter, Slot *setter) {
  impl_->RegisterProperty(name, getter, setter);
}

void StaticScriptable::RegisterMethod(const char *name, Slot *slot) {
  impl_->RegisterMethod(name, slot);
}

void StaticScriptable::RegisterSignal(const char *name, Signal *signal) {
  impl_->RegisterSignal(name, signal);
}

Connection *StaticScriptable::ConnectToOnDeleteSignal(Slot0<void> *slot) {
  return impl_->ConnectToOnDeleteSignal(slot);
}

bool StaticScriptable::GetPropertyInfoByName(const char *name,
                                             int *id, Variant *prototype,
                                             bool *is_method) {
  return impl_->GetPropertyInfoByName(name, id, prototype, is_method);
}

bool StaticScriptable::GetPropertyInfoById(int id, Variant *prototype,
                                           bool *is_method) {
  return impl_->GetPropertyInfoById(id, prototype, is_method);
}

Variant StaticScriptable::GetProperty(int id) {
  return impl_->GetProperty(id);
}

bool StaticScriptable::SetProperty(int id, Variant value) {
  return impl_->SetProperty(id, value);
}

} // namespace ggadget
