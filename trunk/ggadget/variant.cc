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

#include <cmath>
#include <cstring>
#include "variant.h"
#include "logger.h"
#include "scriptable_interface.h"
#include "signals.h"
#include "slot.h"
#include "string_utils.h"

namespace ggadget {

Variant::Variant() : type_(TYPE_VOID) {
  memset(&v_, 0, sizeof(v_));
}

Variant::Variant(const Variant &source) : type_(TYPE_VOID) {
  memset(&v_, 0, sizeof(v_));
  operator=(source);
}

Variant::Variant(Type type) : type_(type) {
  memset(&v_, 0, sizeof(v_));
}

Variant::Variant(ScriptableInterface *value) : type_(TYPE_SCRIPTABLE) {
  if (value) {
    value->Ref();
    v_.scriptable_value_.refchange_connection_ =
        value->ConnectOnReferenceChange(NewSlot(this, &Variant::OnRefChange));
  } else {
    v_.scriptable_value_.refchange_connection_ = NULL;
  }
  v_.scriptable_value_.value_ = value;
}

Variant::~Variant() {
  if (type_ == TYPE_STRING || type_ == TYPE_JSON)
    delete v_.string_value_;
  else if (type_ == TYPE_UTF16STRING)
    delete v_.utf16_string_value_;
  else if (type_ == TYPE_SCRIPTABLE && v_.scriptable_value_.value_) {
    v_.scriptable_value_.refchange_connection_->Disconnect();
    v_.scriptable_value_.value_->Unref();
  }
}

Variant &Variant::operator=(const Variant &source) {
  if (type_ == TYPE_STRING || type_ == TYPE_JSON)
    delete v_.string_value_;
  else if (type_ == TYPE_UTF16STRING)
    delete v_.utf16_string_value_;
  else if (type_ == TYPE_SCRIPTABLE && v_.scriptable_value_.value_) {
    v_.scriptable_value_.refchange_connection_->Disconnect();
    v_.scriptable_value_.value_->Unref();
  }

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
      v_.scriptable_value_.value_ = source.v_.scriptable_value_.value_;
      if (v_.scriptable_value_.value_) {
        v_.scriptable_value_.value_->Ref();
        v_.scriptable_value_.refchange_connection_ =
            v_.scriptable_value_.value_->ConnectOnReferenceChange(
                NewSlot(this, &Variant::OnRefChange));
      } else {
        v_.scriptable_value_.refchange_connection_ = NULL;
      }
      break;
    case TYPE_SLOT:
      v_.slot_value_ = source.v_.slot_value_;
      break;
    case TYPE_DATE:
      v_.int64_value_ = source.v_.int64_value_;
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
      return v_.scriptable_value_.value_ == another.v_.scriptable_value_.value_;
    case TYPE_SLOT: {
      Slot *slot1 = v_.slot_value_;
      Slot *slot2 = another.v_.slot_value_;
      return slot1 == slot2 || (slot1 && slot2 && *slot1 == *slot2);
    }
    case TYPE_DATE:
      return v_.int64_value_ == another.v_.int64_value_;
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

static std::string FitString(const std::string &input) {
  return input.size() > 70 ? input.substr(0, 70) + "..." : input;
}

// Used in unittests.
std::string Variant::Print() const {
  switch (type_) {
    case TYPE_VOID:
      return "VOID";
    case TYPE_BOOL:
      return std::string("BOOL:") + (v_.bool_value_ ? "true" : "false");
    case TYPE_INT64:
      return "INT64:" + StringPrintf("%jd", v_.int64_value_);
    case TYPE_DOUBLE:
      return "DOUBLE:" + StringPrintf("%g", v_.double_value_);
    case TYPE_STRING:
      return std::string("STRING:") +
             (v_.string_value_ ? FitString(*v_.string_value_) : "(nil)");
    case TYPE_JSON:
      return std::string("JSON:") +
             FitString(VariantValue<JSONString>()(*this).value);
    case TYPE_UTF16STRING:
      if (v_.utf16_string_value_) {
        std::string utf8_string;
        ConvertStringUTF16ToUTF8(*v_.utf16_string_value_, &utf8_string);
        return "UTF16STRING:" + FitString(utf8_string);
      }
      return "UTF16STRING:(nil)";
    case TYPE_SCRIPTABLE:
      return StringPrintf("SCRIPTABLE:%p(CLASS_ID=%jx)",
                          v_.scriptable_value_.value_,
                          v_.scriptable_value_.value_ ?
                              v_.scriptable_value_.value_->GetClassId() : 0);
    case TYPE_SLOT:
      return StringPrintf("SLOT:%p", v_.slot_value_);
    case TYPE_DATE:
      return StringPrintf("DATE:%ju", v_.int64_value_);
    case TYPE_ANY:
      return StringPrintf("ANY:%p", v_.any_value_);
    case TYPE_CONST_ANY:
      return StringPrintf("ANY:%p", v_.const_any_value_);
    case TYPE_VARIANT:
      return "VARIANT";
    default:
      return "INVALID";
  }
}

bool Variant::ConvertToString(std::string *result) const {
  ASSERT(result);
  switch (type_) {
    case TYPE_VOID:
      *result = "";
      return true;
    case TYPE_BOOL:
      *result = v_.bool_value_ ? "true" : "false";
      return true;
    case TYPE_INT64:
      *result = StringPrintf("%jd", v_.int64_value_);
      return true;
    case TYPE_DOUBLE:
      *result = StringPrintf("%g", v_.double_value_);
      return true;
    case TYPE_STRING:
      *result = v_.string_value_ ? *v_.string_value_ : "";
      return true;
    case TYPE_JSON:
      return false;
    case TYPE_UTF16STRING:
      if (v_.utf16_string_value_)
        ConvertStringUTF16ToUTF8(*v_.utf16_string_value_, result);
      else
        *result = "";
      return true;
    case TYPE_SCRIPTABLE:
    case TYPE_SLOT:
    case TYPE_DATE:
    case TYPE_ANY:
    case TYPE_CONST_ANY:
    case TYPE_VARIANT:
    default:
      return false;
  }
}

static bool ParseStringToBool(const char *str_value, bool *result) {
  if (!*str_value || GadgetStrCmp(str_value, "false") == 0) {
    *result = false;
    return true;
  }
  if (GadgetStrCmp(str_value, "true") == 0) {
    *result = true;
    return true;
  }
  return false;
}

bool Variant::ConvertToBool(bool *result) const {
  ASSERT(result);
  switch (type_) {
    case TYPE_VOID:
      *result = false;
      return true;
    case TYPE_BOOL:
      *result = v_.bool_value_;
      return true;
    case TYPE_INT64:
      *result = v_.int64_value_ != 0;
      return true;
    case TYPE_DOUBLE:
      *result = v_.double_value_ != 0;
      return true;
    case TYPE_STRING:
      return ParseStringToBool(
          v_.string_value_ ? v_.string_value_->c_str() : "", result);
    case TYPE_JSON:
      return false;
    case TYPE_UTF16STRING: {
      std::string s;
      if (v_.utf16_string_value_)
        ConvertStringUTF16ToUTF8(*v_.utf16_string_value_, &s);
      return ParseStringToBool(s.c_str(), result);
    }
    case TYPE_SCRIPTABLE:
      *result = v_.scriptable_value_.value_ != NULL;
      return true;
    case TYPE_SLOT:
      *result = v_.slot_value_ != NULL;
      return true;
    case TYPE_DATE:
      *result = true;
      return true;
    case TYPE_ANY:
      *result = v_.any_value_ != NULL;
      return true;
    case TYPE_CONST_ANY:
      *result = v_.const_any_value_ != NULL;
      return true;
    case TYPE_VARIANT:
    default:
      return false;
  }
}

bool Variant::ConvertToInt(int *result) const {
  int64_t i;
  if (ConvertToInt64(&i)) {
    *result = static_cast<int>(i);
    return true;
  }
  return false;
}

static bool ParseStringToDouble(const char *str_value, double *result) {
  char *end_ptr;
  // We don't allow hexidecimal numbers and INFINITY or NAN.
  if (strchr(str_value, 'x') || strchr(str_value, 'X') ||
      strchr(str_value, 'n') || strchr(str_value, 'N'))
    return false;

  double d = strtod(str_value, &end_ptr);
  if (*end_ptr == '\0' && !std::isnan(d) && !std::isinf(d)) {
    *result = d;
    return true;
  }
  return false;
}

static bool ParseStringToInt64(const char *str_value, int64_t *result) {
  char *end_ptr;
  // TODO: Check if strtoll is available 
  int64_t i = static_cast<int64_t>(std::strtoll(str_value, &end_ptr, 10));
  if (*end_ptr == '\0') {
    *result = i;
    return true;
  }
  // Then try to parse double.
  double d;
  if (ParseStringToDouble(str_value, &d)) {
    *result = static_cast<int64_t>(round(d));
    return true;
  }
  return false;
}

bool Variant::ConvertToInt64(int64_t *result) const {
  ASSERT(result);
  switch (type_) {
    case TYPE_VOID:
      return false;
    case TYPE_BOOL:
      *result = v_.bool_value_ ? 1 : 0;
      return true;
    case TYPE_INT64:
      *result = v_.int64_value_;
      return true;
    case TYPE_DOUBLE:
      if (std::isnan(v_.double_value_) || std::isinf(v_.double_value_))
        return false;
      *result = static_cast<int64_t>(v_.double_value_);
      return true;
    case TYPE_STRING:
      return ParseStringToInt64(
          v_.string_value_ ? v_.string_value_->c_str() : "", result);
    case TYPE_JSON:
      return false;
    case TYPE_UTF16STRING: {
      std::string s;
      if (v_.utf16_string_value_)
        ConvertStringUTF16ToUTF8(*v_.utf16_string_value_, &s);
      return ParseStringToInt64(s.c_str(), result);
    }
    case TYPE_SCRIPTABLE:
    case TYPE_SLOT:
    case TYPE_DATE:
    case TYPE_ANY:
    case TYPE_CONST_ANY:
    case TYPE_VARIANT:
    default:
      return false;
  }
}

bool Variant::ConvertToDouble(double *result) const {
  ASSERT(result);
  switch (type_) {
    case TYPE_VOID:
      return false;
    case TYPE_BOOL:
      *result = v_.bool_value_ ? 1 : 0;
      return true;
    case TYPE_INT64:
      *result = static_cast<double>(v_.int64_value_);
      return true;
    case TYPE_DOUBLE:
      *result = v_.double_value_;
      return true;
    case TYPE_STRING:
      return ParseStringToDouble(
          v_.string_value_ ? v_.string_value_->c_str() : "", result);
    case TYPE_JSON:
      return false;
    case TYPE_UTF16STRING: {
      std::string s;
      if (v_.utf16_string_value_)
        ConvertStringUTF16ToUTF8(*v_.utf16_string_value_, &s);
      return ParseStringToDouble(s.c_str(), result);
    }
    case TYPE_SCRIPTABLE:
    case TYPE_SLOT:
    case TYPE_DATE:
    case TYPE_ANY:
    case TYPE_CONST_ANY:
    case TYPE_VARIANT:
    default:
      return false;
  }
}

bool Variant::CheckScriptableType(uint64_t class_id) const {
  ASSERT(type_ == TYPE_SCRIPTABLE);
  if (v_.scriptable_value_.value_ &&
      !v_.scriptable_value_.value_->IsInstanceOf(class_id)) {
    LOG("The parameter is not an instance pointer of 0x%jx", class_id);
    return false;
  }
  return true;
}

void Variant::OnRefChange(int ref_count, int change) {
  ASSERT(type_ == TYPE_SCRIPTABLE && v_.scriptable_value_.value_);
  if (ref_count == 0 && change == 0) {
    // The object's destructor is being called.
    // This Variant still holds a reference on this scriptable object.
    // If the program reaches here, the object must be a native owned object,
    // so it can be deleted before refcount reaches 0. We must remove the
    // last reference here.
    v_.scriptable_value_.refchange_connection_->Disconnect();
    v_.scriptable_value_.value_->Unref(true);
    v_.scriptable_value_.value_ = NULL;
  }
}

} // namespace ggadget
