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
  };

  /**
   * Construct a @c Variant without a value.
   * The type of the constructed @c Variant is @c TYPE_VOID.
   */
  Variant() : type(TYPE_VOID) { }

  /**
   * Construct a @c Variant by type with default value (zero).
   */
  Variant(Type type) : type(type) { 
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
   * For testing convenience.
   * Not suggested to use in production code.
   */
  bool operator==(const Variant &another) const;

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
  } v;
};

/**
 * Print a @c Variant into an output stream, only for debugging and testing.
 */ 
std::ostream &operator<<(std::ostream &out, const Variant &v);

/**
 * Get the @c Variant::Type of a type.
 * This template is for all integral types.
 */
template <typename T>
struct VariantType {
  static const Variant::Type type = Variant::TYPE_INT64; 
};

/**
 * Get the @c Variant::Type of a type.
 * Specialized for @c void type.
 */
template <>
struct VariantType<void> {
  static const Variant::Type type = Variant::TYPE_VOID; 
};

/**
 * Get the @c Variant::Type of a type.
 * Specialized for @c bool type.
 */
template <>
struct VariantType<bool> {
  static const Variant::Type type = Variant::TYPE_BOOL; 
};

/**
 * Get the @c Variant::Type of a type.
 * Specialized for @c double type.
 */
template <>
struct VariantType<double> {
  static const Variant::Type type = Variant::TYPE_DOUBLE; 
};

/**
 * Get the @c Variant::Type of a type.
 * Specialized for <code>const char *</code> type.
 */
template <>
struct VariantType<const char *> {
  static const Variant::Type type = Variant::TYPE_STRING; 
};

/**
 * Get the @c Variant::Type of a type.
 * Specialized for <code>const std::string &</code> type.
 */
template <>
struct VariantType<const std::string &> {
  static const Variant::Type type = Variant::TYPE_STRING; 
};

/**
 * Get the @c Variant::Type of a type.
 * Specialized for @c std::string type.
 */
template <>
struct VariantType<std::string> {
  static const Variant::Type type = Variant::TYPE_STRING; 
};

/**
 * Get the @c Variant::Type of a type.
 * Specialized for <code>ScriptableInterface *</code> type.
 */
template <>
struct VariantType<ScriptableInterface *> {
  static const Variant::Type type = Variant::TYPE_SCRIPTABLE; 
};
 
/**
 * Get the @c Variant::Type of a type.
 * Specialized for <code>Slot *</code> type.
 */
template <>
struct VariantType<Slot *> {
  static const Variant::Type type = Variant::TYPE_SLOT; 
};

/**
 * Get the value of a @c Variant.
 * This template is for all integral types.
 */
template <typename T>
struct VariantValue {
  T operator()(Variant v) {
    ASSERT(v.type == Variant::TYPE_INT64);
    return static_cast<T>(v.v.int64_value);
  }
};

/**
 * Get the value of a @c Variant.
 * Specialized for @c bool type.
 */
template <>
struct VariantValue<bool> {
  bool operator()(Variant v) {
    ASSERT(v.type == Variant::TYPE_BOOL);
    return v.v.bool_value;
  }
};

/**
 * Get the value of a @c Variant.
 * Specialized for @c float type.
 */
template <>
struct VariantValue<float> {
  float operator()(Variant v) {
    ASSERT(v.type == Variant::TYPE_DOUBLE);
    return static_cast<float>(v.v.double_value);
  }
};

/**
 * Get the value of a @c Variant.
 * Specialized for @c double type.
 */
template <>
struct VariantValue<double> {
  double operator()(Variant v) {
    ASSERT(v.type == Variant::TYPE_DOUBLE);
    return v.v.double_value;
  }
};

/**
 * Get the value of a @c Variant.
 * Specialized for <code>const char *</code> type.
 */
template <>
struct VariantValue<const char *> {
  const char *operator()(Variant v) {
    ASSERT(v.type == Variant::TYPE_STRING);
    return v.v.string_value;
  }
};

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

/**
 * Get the value of a @c Variant.
 * Specialized for <code>ScriptableInterface *</code> type.
 */
template <>
struct VariantValue<ScriptableInterface *> {
  ScriptableInterface *operator()(Variant v) {
    ASSERT(v.type == Variant::TYPE_SCRIPTABLE);
    return v.v.scriptable_value;
  }
};

/**
 * Get the value of a @c Variant.
 * Specialized for <code>Slot *</code> type.
 */
template <>
struct VariantValue<Slot *> {
  Slot *operator()(Variant v) {
    ASSERT(v.type == Variant::TYPE_SLOT);
    return v.v.slot_value;
  }
};

} // namespace ggadget

#endif // GGADGET_VARIANT_H__
