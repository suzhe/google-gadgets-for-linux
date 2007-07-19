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

#include "signal.h"

namespace ggadget {

Signal::Signal() {
}

Signal::~Signal() {
  for (ConnectionList::iterator it = connections_.begin();
       it != connections_.end(); ++it) {
    delete *it;
  }
}

Connection *Signal::ConnectGeneral(Slot *slot) {
  if (!slot)
    return NULL;

  // Check the compatibility of the arguments and the return value.
  // First, the slot's count of argument must equal to that of this signal.
  int arg_count = GetArgCount();
  if (slot->GetArgCount() != arg_count)
    return NULL;

  // Second, the slot's return type is compatible with that of this signal.
  // The slot can return any type if this signal returns void.
  Variant::Type return_type = GetReturnType();
  if (return_type != Variant::TYPE_VOID &&
      slot->GetReturnType() != return_type)
    return NULL;

  // Third, argument types must equal.
  const Variant::Type *slot_arg_types = slot->GetArgTypes();
  const Variant::Type *signal_arg_types = GetArgTypes();
  for (int i = 0; i < arg_count; i++)
    if (slot_arg_types[i] != signal_arg_types[i])
      return NULL;

  return Connect(slot);
}

Variant Signal::Emit(int argc, Variant argv[]) {
  Variant result(GetReturnType());
  for (ConnectionList::iterator it = connections_.begin();
       it != connections_.end(); ++it) {
    Connection *connection = *it;
    if (!connection->blocked()) {
      result = connection->slot_->Call(argc, argv);
    }
  }
  return result;
}

Connection *Signal::Connect(Slot *slot) {
  Connection *connection = new Connection(slot);
  connections_.push_back(connection);
  return connection;
}

} // namespace ggadget
