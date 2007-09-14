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
#include "scriptable_interface.h"
#include "slot.h"

namespace ggadget {

bool Variant::operator==(const Variant &another) const {
  if (type_ != another.type_)
    return false;

  switch (type_) {
    case TYPE_VOID:
      return true;
    case TYPE_BOOL:
      return v_.bool_value_ == another.v_.bool_value_;
    case TYPE_INT64:
      return v_.int64_value_ == another.v_.int64_value_;
    case TYPE_DOUBLE:
      return v_.double_value_ == another.v_.double_value_;
    case TYPE_STRING: {
      const char *s1 = VariantValue<const char *>()(*this);
      const char *s2 = VariantValue<const char *>()(another);
      return s1 == s2 || (s1 && s2 && strcmp(s1, s2) == 0);
    }
    case TYPE_SCRIPTABLE:
      return v_.scriptable_value_ == another.v_.scriptable_value_;
    case TYPE_CONST_SCRIPTABLE:
      return v_.const_scriptable_value_ == another.v_.const_scriptable_value_;
    case TYPE_SLOT: {
      Slot *slot1 = v_.slot_value_;
      Slot *slot2 = another.v_.slot_value_;
      return slot1 == slot2 || (slot1 && slot2 && *slot1 == *slot2);
    }
    case TYPE_VARIANT:
      // A Variant of type TYPE_VARIANT is only used as a prototype,
      // so they are all equal.
      return true;
    default:
      return false;
  }
}

// Used in unittests.
std::string Variant::ToString() const {
  char buffer[32];
  switch (type_) {
    case Variant::TYPE_VOID:
      return std::string("VOID");
    case Variant::TYPE_BOOL:
      return std::string("BOOL:") + (v_.bool_value_ ? "true" : "false");
    case Variant::TYPE_INT64:
      sprintf(buffer, "INT64:%" PRId64, v_.int64_value_);
      return std::string(buffer);
    case Variant::TYPE_DOUBLE:
      sprintf(buffer, "DOUBLE:%lf", v_.double_value_);
      return std::string(buffer);
    case Variant::TYPE_STRING:
      return std::string("STRING:") + VariantValue<const char *>()(*this);
    case Variant::TYPE_SCRIPTABLE:
      sprintf(buffer, "SCRIPTABLE:%p", v_.scriptable_value_);
      return std::string(buffer);
    case Variant::TYPE_CONST_SCRIPTABLE:
      sprintf(buffer, "CONST_SCRIPTABLE:%p", v_.const_scriptable_value_);
      return std::string(buffer);
    case Variant::TYPE_SLOT:
      sprintf(buffer, "SLOT:%p", v_.slot_value_);
      return std::string(buffer);
    case Variant::TYPE_VARIANT:
      return std::string("VARIANT");
    default:
      return std::string("INVALID");
  }
}

bool Variant::CheckScriptableType(uint64_t class_id) const {
  ASSERT(type_ == TYPE_SCRIPTABLE || type_ == TYPE_CONST_SCRIPTABLE);
  if (v_.const_scriptable_value_ &&
      !v_.const_scriptable_value_->IsInstanceOf(class_id)) {
    LOG("The parameter is not an instance pointer of 0x%jx", class_id);
    return false;
  }
  return true;
}

} // namespace ggadget
