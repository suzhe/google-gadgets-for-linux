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

#ifndef GGADGET_VARIANT_H__
#define GGADGET_VARIANT_H__

#include <string>
#include <ostream>
#include "common.h"

namespace ggadget {

class ScriptableInterface;
class Slot;

/**
 * A value that can be a string or a integer.
 * Used to represent some properties in Gadget API, such as
 * @c basicElement.width.
 */
struct IntOrString {
  enum Type {
    TYPE_INT,
    TYPE_STRING,
  };

  Type type;
  union {
    int int_value;
    const char *string_value;
  } v;

  bool operator==(IntOrString another) const;
  std::string ToString() const;
};

/**
 * Construct a @c IntOrString with a @c int value.
 * Because @c IntOrString is used in a union in @c Variant, it is not allowed
 * to have constructors.
 */
inline IntOrString CreateIntOrString(int value) {
  IntOrString v;
  v.type = IntOrString::TYPE_INT;
  v.v.int_value = value;
  return v;
}

/**
 * Construct a @c IntOrString with a <code>const char *</code> value.
 * Because @c IntOrString is used in a union in @c Variant, it is not allowed
 * to have constructors.
 */
inline IntOrString CreateIntOrString(const char *value) {
  IntOrString v;
  v.type = IntOrString::TYPE_STRING;
  v.v.string_value = value;
  return v;
}

/**
 * Print a @c Variant into an output stream, only for debugging and testing.
 */ 
inline std::ostream &operator<<(std::ostream &out, IntOrString v) {
  return out << v.ToString();
}

/**
 * A @c Variant contains a value of arbitrary type that can be transfered
 * between C++ and script engines, or between a @c Signal and a @c Slot.
 */
struct Variant {
  /**
   * Type enum definition of the value of the @c Variant.
   */
  enum Type {
    /** No value. */
    TYPE_VOID,
    /** @c true / @c false @c bool type. */
    TYPE_BOOL,
    /** 64 bit signed integer type, can also contain other integral types. */
    TYPE_INT64,
    /** @c double type. */
    TYPE_DOUBLE,
    /** <code>const char *</code> type. */
    TYPE_STRING,
    /** <code>ScriptableInterface *</code> type. */
    TYPE_SCRIPTABLE,
    /** <code>Slot *</code> type. */
    TYPE_SLOT,
    /** @c StringOrInt type */
    TYPE_INT_OR_STRING,
  };

  /**
   * Construct a @c Variant without a value.
   * The type of the constructed @c Variant is @c TYPE_VOID.
   */
  Variant() : type(TYPE_VOID) { }

  /**
   * Construct a @c Variant by type with default value (zero).
   */
  Variant(Type a_type) : type(a_type) { 
    if (type == TYPE_STRING) v.string_value = "";
    else v.double_value = 0;  // Also sets other unions to 0.
  }

  /**
   * Construct a @c Variant with a @c bool value.
   * The type of the constructed @c Variant is @c TYPE_BOOL.
   */
  explicit Variant(bool value) : type(TYPE_BOOL) {
    v.bool_value = value;
  }

  /**
   * Construct a @c Variant with a @c int32_t value.
   * Also supports <code>[unsigned]</code> char and <code>[unsigned]
   * short</code>.
   * The type of the constructed @c Variant is @c TYPE_INT64.
   */
  explicit Variant(int32_t value) : type(TYPE_INT64) {
    v.int64_value = value;
  }

  /**
   * Construct a @c Variant with a @c uint32_t value.
   * The type of the constructed @c Variant is @c TYPE_INT64.
   */
  explicit Variant(uint32_t value) : type(TYPE_INT64) {
    v.int64_value = static_cast<int64_t>(value);
  }

#ifndef LONG_INT64_T_EQUIVALENT
  /**
   * Construct a @c Variant with a @c long value.
   * The type of the constructed @c Variant is @c TYPE_INT64.
   */
  explicit Variant(long value) : type(TYPE_INT64) {
    v.int64_value = value;
  }

  /**
   * Construct a @c Variant with a <code>unsigned long</code> value.
   * The type of the constructed @c Variant is @c TYPE_INT64.
   */
  explicit Variant(unsigned long value) : type(TYPE_INT64) {
    v.int64_value = static_cast<int64_t>(value);
  }
#endif // LONG_INT64_T_EQUIVALENT

  /**
   * Construct a @c Variant with a @c int64_t value.
   * The type of the constructed @c Variant is @c TYPE_INT64.
   */
  explicit Variant(int64_t value) : type(TYPE_INT64) {
    v.int64_value = value;
  }

  /**
   * Construct a @c Variant with a @c uint64_t value.
   * The type of the constructed @c Variant is @c TYPE_INT64.
   */
  explicit Variant(uint64_t value) : type(TYPE_INT64) {
    v.int64_value = static_cast<int64_t>(value);
  }

  /**
   * Construct a @c Variant with a @c double value.
   * The type of the constructed @c Variant is @c TYPE_DOUBLE.
   */
  explicit Variant(double value) : type(TYPE_DOUBLE) {
    v.double_value = value;
  }

  /**
   * Construct a @c Variant with a @c std::string value.
   * The parameter must live longer than this @c Variant.
   * The type of the constructed @c Variant is @c TYPE_STRING.
   */
  explicit Variant(const std::string &value) : type(TYPE_STRING) {
    v.string_value = value.c_str();
  }

  /**
   * Construct a @c Variant with a <code>cost char *</code> value.
   * The type of the constructed @c Variant is @c TYPE_STRING.
   */
  explicit Variant(const char *value) : type(TYPE_STRING) {
    v.string_value = value;
  }

  /**
   * Construct a @c Variant with a <code>ScriptableInterface *</code> value.
   * This @c Variant doesn't owns the @c Scriptable pointer.
   * The type of the constructed @c Variant is @c TYPE_SCRIPTABLE.
   */
  explicit Variant(ScriptableInterface *value) : type(TYPE_SCRIPTABLE) {
    v.scriptable_value = value;
  }

  /**
   * Construct a @c Variant with a @c Slot pointer value.
   * This @c Variant doesn't owns the <code>Slot *</code>.
   * The type of the constructed @c Variant is @c TYPE_SLOT.
   */
  explicit Variant(Slot *value) : type(TYPE_SLOT) {
    v.slot_value = value;
  }

  /**
   * Construct a @c Variant with a @c StringOrInt value.
   * The type of the constructed @c Variant is @c TYPE_STRING_OR_INT.
   */
  explicit Variant(IntOrString value) : type(TYPE_INT_OR_STRING) {
    v.int_or_string_value = value;
  }

  /**
   * For testing convenience.
   * Not suggested to use in production code.
   */
  bool operator==(Variant another) const;

  std::string ToString() const;

  /**
   * Type of the <code>Variant</code>'s value.
   */
  Type type;

  /**
   * Value of the Variant.
   */
  union {
    bool bool_value;
    int64_t int64_value;
    double double_value;
    const char *string_value;
    ScriptableInterface *scriptable_value;
    Slot *slot_value;
    IntOrString int_or_string_value;
  } v;
};

/**
 * Print a @c Variant into an output stream, only for debugging and testing.
 */ 
inline std::ostream &operator<<(std::ostream &out, Variant v) {
  return out << v.ToString();
}

/**
 * Get the @c Variant::Type of a C++ type.
 * This template is for all <code>ScriptableInterface *</code> types.
 * All unspecialized types will fall into this template, and if it is not
 * <code>ScriptableInterface *</code>, it may cause compilation error.  
 */
template <typename T>
struct VariantType {
  static const Variant::Type type = Variant::TYPE_SCRIPTABLE; 
};

/**
 * Get the @c Variant::Type of a C++ type.
 * Specialized for certain C++ type.
 */
#define SPECIALIZE_VARIANT_TYPE(c_type, variant_type)      \
template <>                                                \
struct VariantType<c_type> {                               \
  static const Variant::Type type = Variant::variant_type; \
};

SPECIALIZE_VARIANT_TYPE(void, TYPE_VOID)
SPECIALIZE_VARIANT_TYPE(bool, TYPE_BOOL)
SPECIALIZE_VARIANT_TYPE(char, TYPE_INT64)
SPECIALIZE_VARIANT_TYPE(unsigned char, TYPE_INT64)
SPECIALIZE_VARIANT_TYPE(short, TYPE_INT64)
SPECIALIZE_VARIANT_TYPE(unsigned short, TYPE_INT64)
SPECIALIZE_VARIANT_TYPE(int, TYPE_INT64)
SPECIALIZE_VARIANT_TYPE(unsigned int, TYPE_INT64)
#ifndef LONG_INT64_T_EQUIVALENT
SPECIALIZE_VARIANT_TYPE(long, TYPE_INT64)
SPECIALIZE_VARIANT_TYPE(unsigned long, TYPE_INT64)
#endif
SPECIALIZE_VARIANT_TYPE(int64_t, TYPE_INT64)
SPECIALIZE_VARIANT_TYPE(uint64_t, TYPE_INT64)
SPECIALIZE_VARIANT_TYPE(float, TYPE_DOUBLE)
SPECIALIZE_VARIANT_TYPE(double, TYPE_DOUBLE)
SPECIALIZE_VARIANT_TYPE(const char *, TYPE_STRING)
SPECIALIZE_VARIANT_TYPE(const std::string &, TYPE_STRING)
SPECIALIZE_VARIANT_TYPE(std::string, TYPE_STRING)
SPECIALIZE_VARIANT_TYPE(Slot *, TYPE_SLOT)
SPECIALIZE_VARIANT_TYPE(IntOrString, TYPE_INT_OR_STRING)

/**
 * Get the value of a @c Variant.
 * This template is for all <code>ScriptableInterface *</code> types.
 * All unspecialized types will fall into this template, and if it is not
 * <code>ScriptableInterface *</code>, it may cause compilation error.  
 */
template <typename T>
struct VariantValue {
  T operator()(Variant v) {
    ASSERT(v.type == Variant::TYPE_SCRIPTABLE);
    return down_cast<T>(v.v.scriptable_value);
  }
};

/**
 * Get the @c Variant::Type of a C++ type.
 * Specialized for certain C++ type.
 */
#define SPECIALIZE_VARIANT_VALUE(c_type, variant_field) \
template <>                                             \
struct VariantValue<c_type> {                           \
  c_type operator()(Variant v) {                        \
    ASSERT(v.type == VariantType<c_type>::type);        \
    return static_cast<c_type>(v.v.variant_field);      \
  }                                                     \
};

SPECIALIZE_VARIANT_VALUE(char, int64_value)
SPECIALIZE_VARIANT_VALUE(bool, bool_value)
SPECIALIZE_VARIANT_VALUE(unsigned char, int64_value)
SPECIALIZE_VARIANT_VALUE(short, int64_value)
SPECIALIZE_VARIANT_VALUE(unsigned short, int64_value)
SPECIALIZE_VARIANT_VALUE(int, int64_value)
SPECIALIZE_VARIANT_VALUE(unsigned int, int64_value)
#ifndef LONG_INT64_T_EQUIVALENT
SPECIALIZE_VARIANT_VALUE(long, int64_value)
SPECIALIZE_VARIANT_VALUE(unsigned long, int64_value)
#endif
SPECIALIZE_VARIANT_VALUE(int64_t, int64_value)
SPECIALIZE_VARIANT_VALUE(uint64_t, int64_value)
SPECIALIZE_VARIANT_VALUE(float, double_value)
SPECIALIZE_VARIANT_VALUE(double, double_value)
SPECIALIZE_VARIANT_VALUE(const char *, string_value)
SPECIALIZE_VARIANT_VALUE(Slot *, slot_value)
SPECIALIZE_VARIANT_VALUE(IntOrString, int_or_string_value)

/**
 * Get the value of a @c Variant.
 * Specialized for <code>std::string</code> type.
 */
template <>
struct VariantValue<std::string> {
  std::string operator()(Variant v) {
    ASSERT(v.type == Variant::TYPE_STRING);
    if (v.v.string_value) return std::string(v.v.string_value);
    return std::string();
  }
};

/**
 * Get the value of a @c Variant.
 * Specialized for <code>const std::string &</code> type.
 */
template <>
struct VariantValue<const std::string &> {
  std::string operator()(Variant v) {
    ASSERT(v.type == Variant::TYPE_STRING);
    if (v.v.string_value) return std::string(v.v.string_value);
    return std::string();
  }
};

#undef SPECIALIZE_VARIANT_TYPE
#undef SPECIALIZE_VARIANT_VALUE

} // namespace ggadget

#endif // GGADGET_VARIANT_H__
