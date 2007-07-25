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
#include "variant.h"
#include "slot.h"

namespace ggadget {

bool IntOrString::operator==(IntOrString another) const {
  if (type != another.type)
    return false;

  switch (type) {
    case TYPE_INT:
      return v.int_value == another.v.int_value;
    case TYPE_STRING:
      return v.string_value == another.v.string_value ||
             strcmp(v.string_value, another.v.string_value) == 0;
    default:
      return false;
  }
}

// Used in unittests.
std::ostream &operator<<(std::ostream &out, IntOrString v) {
  switch (v.type) {
    case IntOrString::TYPE_INT:
      return out << "INT:" << v.v.int_value;
    case IntOrString::TYPE_STRING:
      return out << "STRING:" << v.v.string_value;
    default:
      return out << "UNKNOWN:" << v.type;
  }
}

bool Variant::operator==(Variant another) const {
  if (type != another.type)
    return false;

  switch (type) {
    case TYPE_VOID:
      return true;
    case TYPE_BOOL:
      return v.bool_value == another.v.bool_value;
    case TYPE_INT64:
      return v.int64_value == another.v.int64_value;
    case TYPE_DOUBLE:
      return v.double_value == another.v.double_value;
    case TYPE_STRING:
      return v.string_value == another.v.string_value ||
             strcmp(v.string_value, another.v.string_value) == 0;
    case TYPE_SCRIPTABLE:
      return v.scriptable_value == another.v.scriptable_value;
    case TYPE_SLOT: {
      Slot *slot1 = v.slot_value;
      Slot *slot2 = another.v.slot_value;
      return slot1 == slot2 || (slot1 && slot2 && *slot1 == *slot2);
    }
    case TYPE_INT_OR_STRING:
      return v.int_or_string_value == another.v.int_or_string_value; 
    default:
      return false;
  }
}

// Used in unittests.
std::ostream &operator<<(std::ostream &out, Variant v) {
  switch (v.type) {
    case Variant::TYPE_VOID:
      return out << "VOID";
    case Variant::TYPE_BOOL:
      return out << "BOOL:" << v.v.bool_value;
    case Variant::TYPE_INT64:
      return out << "INT64:" << v.v.int64_value;
    case Variant::TYPE_DOUBLE:
      return out << "DOUBLE:" << v.v.double_value;
    case Variant::TYPE_STRING:
      return out << "STRING:" << v.v.string_value;
    case Variant::TYPE_SCRIPTABLE:
      return out << "SCRIPTABLE";
    case Variant::TYPE_SLOT:
      return out << "SLOT";
    case Variant::TYPE_INT_OR_STRING:
      return out << "INT_OR_STRING:" << v.v.int_or_string_value;
    default:
      return out << "UNKNOWN:" << v.type;
  }
}

} // namespace ggadget
