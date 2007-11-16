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
#include <string.h>
#include "scriptable_interface.h"
#include "slot.h"

namespace ggadget {

Variant::Variant(const Variant &source) : type_(TYPE_VOID) {
  v_.double_value_ = 0;
  operator=(source);
}

Variant::~Variant() {
  if (type_ == TYPE_STRING || type_ == TYPE_JSON)
    delete v_.string_value_;
  if (type_ == TYPE_UTF16STRING)
    delete v_.utf16_string_value_;
}

Variant &Variant::operator=(const Variant &source) {
  if (type_ == TYPE_STRING || type_ == TYPE_JSON)
    delete v_.string_value_;
  if (type_ == TYPE_UTF16STRING)
    delete v_.utf16_string_value_;

  type_ = source.type_;
  switch (type_) {
    case TYPE_VOID:
      break;
    case TYPE_BOOL:
      v_.bool_value_ = source.v_.bool_value_;
      break;
    case TYPE_INT64:
      v_.int64_value_ = source.v_.int64_value_;
      break;
    case TYPE_DOUBLE:
      v_.double_value_ = source.v_.double_value_;
      break;
    case TYPE_STRING:
    case TYPE_JSON:
      v_.string_value_ = source.v_.string_value_ ?
                         new std::string(*source.v_.string_value_) : NULL;
      break;
    case TYPE_UTF16STRING:
      v_.utf16_string_value_ = source.v_.utf16_string_value_ ?
                               new UTF16String(*source.v_.utf16_string_value_) :
                               NULL;
      break;
    case TYPE_SCRIPTABLE:
      v_.scriptable_value_ = source.v_.scriptable_value_;
      break;
    case TYPE_CONST_SCRIPTABLE:
      v_.const_scriptable_value_ = source.v_.const_scriptable_value_;
      break;
    case TYPE_SLOT:
      v_.slot_value_ = source.v_.slot_value_;
      break;
    case TYPE_ANY:
      v_.any_value_ = source.v_.any_value_;
      break;
    case TYPE_CONST_ANY:
      v_.const_any_value_ = source.v_.const_any_value_;
      break;
    case TYPE_VARIANT:
      // A Variant of type TYPE_VARIANT is only used as a prototype.
      break;
    default:
      break;
  }
  return *this;
}

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
    case TYPE_STRING:
    case TYPE_JSON:
      return v_.string_value_ == another.v_.string_value_ ||
            (v_.string_value_ && another.v_.string_value_ &&
             *v_.string_value_ == *another.v_.string_value_);
    case TYPE_UTF16STRING:
      return v_.utf16_string_value_ == another.v_.utf16_string_value_ ||
            (v_.utf16_string_value_ && another.v_.utf16_string_value_ &&
             *v_.utf16_string_value_ == *another.v_.utf16_string_value_);
    case TYPE_SCRIPTABLE:
      return v_.scriptable_value_ == another.v_.scriptable_value_;
    case TYPE_CONST_SCRIPTABLE:
      return v_.const_scriptable_value_ == another.v_.const_scriptable_value_;
    case TYPE_SLOT: {
      Slot *slot1 = v_.slot_value_;
      Slot *slot2 = another.v_.slot_value_;
      return slot1 == slot2 || (slot1 && slot2 && *slot1 == *slot2);
    }
    case TYPE_ANY:
      return v_.any_value_ == another.v_.any_value_;
    case TYPE_CONST_ANY:
      return v_.const_any_value_ == another.v_.const_any_value_;
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
      snprintf(buffer, sizeof(buffer), "INT64:%jd", v_.int64_value_);
      return std::string(buffer);
    case Variant::TYPE_DOUBLE:
      snprintf(buffer, sizeof(buffer), "DOUBLE:%lf", v_.double_value_);
      return std::string(buffer);
    case Variant::TYPE_STRING:
      return std::string("STRING:") +
             (v_.string_value_ ? v_.string_value_->c_str() : "(nil");
    case Variant::TYPE_JSON:
      return std::string("JSON:") + VariantValue<JSONString>()(*this).value;
    case Variant::TYPE_UTF16STRING:
      if (v_.utf16_string_value_) {
        std::string utf8_string;
        ConvertStringUTF16ToUTF8(*v_.utf16_string_value_, &utf8_string);
        return "UTF16STRING:" + utf8_string;
      }
      return "UTF16STRING:(nil)"; 
    case Variant::TYPE_SCRIPTABLE:
      snprintf(buffer, sizeof(buffer),
               "SCRIPTABLE:%p(CLASS_ID=%jx)", v_.scriptable_value_,
               v_.scriptable_value_ ?
                   v_.scriptable_value_->GetClassId() : 0);
      return std::string(buffer);
    case Variant::TYPE_CONST_SCRIPTABLE:
      snprintf(buffer, sizeof(buffer), "CONST_SCRIPTABLE:%p(CLASS_ID=%jx):",
               v_.const_scriptable_value_,
               v_.const_scriptable_value_ ?
                   v_.const_scriptable_value_->GetClassId() : 0);
      return std::string(buffer);
    case Variant::TYPE_SLOT:
      snprintf(buffer, sizeof(buffer), "SLOT:%p", v_.slot_value_);
      return std::string(buffer);
    case Variant::TYPE_ANY:
      snprintf(buffer, sizeof(buffer), "ANY:%p", v_.any_value_);
      return std::string(buffer);
    case Variant::TYPE_CONST_ANY:
      snprintf(buffer, sizeof(buffer), "CONST_ANY:%p", v_.const_any_value_);
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
