// Copyright 2007 Google Inc. All Rights Reserved.
// Author: wangxianzhu@google.com (Xianzhu Wang)

#ifndef GGADGET_VARIANT_H__
#define GGADGET_VARIANT_H__

#include <string>
#include "commons.h"

namespace ggadget {

class ScriptableInterface;
class Slot;

/**
 * A @c Variant contains a value of arbitrary type that can be transfered
 * between C++ and script engines, or between a @c Signal and a @c Slot.
 * It doesn't support <code>[unsigned] long</code> types.
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
    /** String type. */
    TYPE_STRING,
    /** @c Scriptable object pointer. */
    TYPE_SCRIPTABLE,
    /** @c Slot target. */
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
   * This @c Variant doesn't owns the @c Slot pointer.
   * The type of the constructed @c Variant is @c TYPE_SLOT.
   */
  explicit Variant(Slot *value) : type(TYPE_SLOT) {
    v.slot_value = value;
  }

  /**
   * For testing convenience.
   * Not suggested to use in production code.
   */
  bool operator==(const Variant &another) {
    if (another.type != type)
      return false;
    switch (type) {
      case TYPE_VOID: return true;
      case TYPE_BOOL: return another.v.bool_value == v.bool_value;
      case TYPE_INT64: return another.v.int64_value == v.int64_value;
      case TYPE_DOUBLE: return another.v.double_value == v.double_value;
      case TYPE_SCRIPTABLE:
        return another.v.scriptable_value == v.scriptable_value;
      case TYPE_SLOT: return another.v.slot_value == v.slot_value;
      default: return false;
    }
  }

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
 * Get the @c Variant::Type of a type.
 * This template is for all integral types.
 */
template <typename T>
inline Variant::Type VariantType() {
  return Variant::TYPE_INT64;
}

/**
 * Get the @c Variant::Type of a type.
 * Specialized for @c void type.
 */
template <>
inline Variant::Type VariantType<void>() {
  return Variant::TYPE_VOID;
}

/**
 * Get the @c Variant::Type of a type.
 * Specialized for @c bool type.
 */
template <>
inline Variant::Type VariantType<bool>() {
  return Variant::TYPE_BOOL;
}

/**
 * Get the @c Variant::Type of a type.
 * Specialized for @c double type.
 */
template <>
inline Variant::Type VariantType<double>() {
  return Variant::TYPE_DOUBLE;
}

/**
 * Get the @c Variant::Type of a type.
 * Specialized for <code>const char *</code> type.
 */
template <>
inline Variant::Type VariantType<const char *>() {
  return Variant::TYPE_STRING;
}

/**
 * Get the @c Variant::Type of a type.
 * Specialized for <code>const std::string &</code> type.
 */
template <>
inline Variant::Type VariantType<const std::string &>() {
  return Variant::TYPE_STRING;
}

/**
 * Get the @c Variant::Type of a type.
 * Specialized for @c std::string type.
 */
template <>
inline Variant::Type VariantType<std::string>() {
  return Variant::TYPE_STRING;
}

/**
 * Get the @c Variant::Type of a type.
 * Specialized for <code>ScriptableInterface *</code> type.
 */
template <>
inline Variant::Type VariantType<ScriptableInterface *>() {
  return Variant::TYPE_SCRIPTABLE;
}
 
/**
 * Get the @c Variant::Type of a type.
 * Specialized for <code>Slot *</code> type.
 */
template <>
inline Variant::Type VariantType<Slot *>() {
  return Variant::TYPE_SLOT;
}

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
