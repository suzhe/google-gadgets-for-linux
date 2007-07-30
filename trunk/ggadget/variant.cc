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

// Glibc require this macro in order to use the format macros in inttypes.h. 
#define __STDC_FORMAT_MACROS  
#include <inttypes.h>  // for PRId64
#include <string.h>
#include "variant.h"
#include "slot.h"

namespace ggadget {

bool Variant::operator==(const Variant &another) const {
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
    case TYPE_VARIANT:
      return true;
    default:
      return false;
  }
}

// Used in unittests.
std::string Variant::ToString() const {
  char buffer[32];
  switch (type) {
    case Variant::TYPE_VOID:
      return std::string("VOID");
    case Variant::TYPE_BOOL:
      return std::string("BOOL:") + (v.bool_value ? "true" : "false");
    case Variant::TYPE_INT64:
      sprintf(buffer, "INT64:%" PRId64, v.int64_value);
      return std::string(buffer);
    case Variant::TYPE_DOUBLE:
      sprintf(buffer, "DOUBLE:%lf", v.double_value);
      return std::string(buffer);
    case Variant::TYPE_STRING:
      return std::string("STRING:") + v.string_value;
    case Variant::TYPE_SCRIPTABLE:
      sprintf(buffer, "SCRIPTABLE:%p", v.scriptable_value);
      return std::string(buffer);
    case Variant::TYPE_SLOT:
      sprintf(buffer, "SLOT:%p", v.slot_value);
      return std::string(buffer);
    case Variant::TYPE_VARIANT:
      return std::string("VARIANT");
    default:
      return std::string("INVALID");
  }
}

} // namespace ggadget
