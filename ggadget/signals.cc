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

#include "signals.h"

namespace ggadget {

Connection::Connection(const Signal *signal, Slot *slot)
    : blocked_(slot == NULL),
      signal_(signal),
      slot_(slot) {
}

Connection::~Connection() {
  delete slot_;
  slot_ = NULL;
}

void Connection::Disconnect() {
  delete slot_;
  slot_ = NULL;
  blocked_ = true;
}

bool Connection::Reconnect(Slot *slot) {
  delete slot_;
  slot_ = NULL;
  if (slot && !signal_->CheckCompatibility(slot)) {
    // According to our convention, no matter Reconnect succeeds or failes,
    // the slot is always owned by the Connection.
    delete slot;
    return false;
  }
  slot_ = slot;
  Unblock();
  return true;
}

Signal::Signal() {
}

Signal::~Signal() {
  for (ConnectionList::iterator it = connections_.begin();
       it != connections_.end(); ++it) {
    delete *it;
  }
}

Connection *Signal::ConnectGeneral(Slot *slot) {
  if (slot && !CheckCompatibility(slot)) {
    // According to our convention, no matter Reconnect succeeds or failes,
    // the slot is always owned by the Connection.
    delete slot;
    return NULL;
  }
  return Connect(slot);
}

bool Signal::CheckCompatibility(const Slot *slot) const {
  // Check the compatibility of the arguments and the return value.
  // First, the slot's count of argument must equal to that of this signal.
  int arg_count = GetArgCount();
  if (slot->GetArgCount() != arg_count)
    return false;

  // Second, the slot's return type is compatible with that of this signal.
  // The slot can return any type if this signal returns void.
  Variant::Type return_type = GetReturnType();
  if (return_type != Variant::TYPE_VOID &&
      slot->GetReturnType() != return_type)
    return false;

  // Third, argument types must equal.
  const Variant::Type *slot_arg_types = slot->GetArgTypes();
  const Variant::Type *signal_arg_types = GetArgTypes();
  for (int i = 0; i < arg_count; i++)
    if (slot_arg_types[i] != signal_arg_types[i])
      return false;

  return true;
}

bool Signal::HasActiveConnections() const {
  if (connections_.empty())
    return false;
  for (ConnectionList::const_iterator it = connections_.begin();
       it != connections_.end(); ++it) {
    if (!(*it)->blocked())
      return true;
  }
  return false;
}

Variant Signal::Emit(int argc, Variant argv[]) const {
  Variant result(GetReturnType());
  for (ConnectionList::const_iterator it = connections_.begin();
       it != connections_.end(); ++it) {
    Connection *connection = *it;
    if (!connection->blocked()) {
      result = connection->slot_->Call(argc, argv);
    }
  }
  return result;
}

Connection *Signal::Connect(Slot *slot) {
  Connection *connection = new Connection(this, slot);
  connections_.push_back(connection);
  return connection;
}

} // namespace ggadget
