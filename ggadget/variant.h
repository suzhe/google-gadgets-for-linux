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
 * <code>Variant</code>s are immutable. 
 */
class Variant {
 public:
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
    /**
     * @c TYPE_VARIANT is only used to indicate a parameter or a return type
     * can accept any above type.  A @c Variant with this type can only act
     * as a prototype, not a real value. 
     */
    TYPE_VARIANT,
  };

  /**
   * Construct a @c Variant without a value.
   * The type of the constructed @c Variant is @c TYPE_VOID.
   */
  Variant() : type_(TYPE_VOID) {
    v_.double_value_ = 0;  // Also sets other unions to 0.
  }

  /**
   * Construct a @c Variant by type with default value (zero).
   */
  Variant(Type a_type) : type_(a_type) {
    v_.double_value_ = 0;  // Also sets other unions to 0.
  }

  /**
   * Construct a @c Variant with a @c bool value.
   * The type of the constructed @c Variant is @c TYPE_BOOL.
   */
  explicit Variant(bool value) : type_(TYPE_BOOL) {
    v_.bool_value_ = value;
  }

  /**
   * Construct a @c Variant with a @c int32_t value.
   * Also supports <code>[unsigned]</code> char and <code>[unsigned]
   * short</code>.
   * The type of the constructed @c Variant is @c TYPE_INT64.
   */
  explicit Variant(int32_t value) : type_(TYPE_INT64) {
    v_.int64_value_ = value;
  }

  /**
   * Construct a @c Variant with a @c uint32_t value.
   * The type of the constructed @c Variant is @c TYPE_INT64.
   */
  explicit Variant(uint32_t value) : type_(TYPE_INT64) {
    v_.int64_value_ = static_cast<int64_t>(value);
  }

#ifndef LONG_INT64_T_EQUIVALENT
  /**
   * Construct a @c Variant with a @c long value.
   * The type of the constructed @c Variant is @c TYPE_INT64.
   */
  explicit Variant(long value) : type_(TYPE_INT64) {
    v_.int64_value_ = value;
  }

  /**
   * Construct a @c Variant with a <code>unsigned long</code> value.
   * The type of the constructed @c Variant is @c TYPE_INT64.
   */
  explicit Variant(unsigned long value) : type_(TYPE_INT64) {
    v_.int64_value_ = static_cast<int64_t>(value);
  }
#endif // LONG_INT64_T_EQUIVALENT

  /**
   * Construct a @c Variant with a @c int64_t value.
   * The type of the constructed @c Variant is @c TYPE_INT64.
   */
  explicit Variant(int64_t value) : type_(TYPE_INT64) {
    v_.int64_value_ = value;
  }

  /**
   * Construct a @c Variant with a @c uint64_t value.
   * The type of the constructed @c Variant is @c TYPE_INT64.
   */
  explicit Variant(uint64_t value) : type_(TYPE_INT64) {
    v_.int64_value_ = static_cast<int64_t>(value);
  }

  /**
   * Construct a @c Variant with a @c double value.
   * The type of the constructed @c Variant is @c TYPE_DOUBLE.
   */
  explicit Variant(double value) : type_(TYPE_DOUBLE) {
    v_.double_value_ = value;
  }

  /**
   * Construct a @c Variant with a @c std::string value.
   * The type of the constructed @c Variant is @c TYPE_STRING.
   */
  explicit Variant(const std::string &value)
      : type_(TYPE_STRING),
        string_value_(value) {
    v_.charptr_value_ = NULL;
  }

  /**
   * Construct a @c Variant with a <code>cost char *</code> value.
   * This @c Variant doesn't owns the @a value pointer.
   * If you want this @c Variant to hold a independent string value, pass a
   * @c std::string parameter instead. 
   * The type of the constructed @c Variant is @c TYPE_STRING.
   */
  explicit Variant(const char *value) : type_(TYPE_STRING) {
    v_.charptr_value_ = value;
  }

  /**
   * Construct a @c Variant with a <code>ScriptableInterface *</code> value.
   * This @c Variant doesn't owns the @c Scriptable pointer.
   * The type of the constructed @c Variant is @c TYPE_SCRIPTABLE.
   */
  explicit Variant(ScriptableInterface *value) : type_(TYPE_SCRIPTABLE) {
    v_.scriptable_value_ = value;
  }

  /**
   * Construct a @c Variant with a @c Slot pointer value.
   * This @c Variant doesn't owns the <code>Slot *</code>.
   * The type of the constructed @c Variant is @c TYPE_SLOT.
   */
  explicit Variant(Slot *value) : type_(TYPE_SLOT) {
    v_.slot_value_ = value;
  }

  /**
   * For testing convenience.
   * Not suggested to use in production code.
   */
  bool operator==(const Variant &another) const;

  std::string ToString() const;

  Type type() const { return type_; }

 private:
  bool CheckScriptableType(int class_id) const;

  /**
   * Type of the <code>Variant</code>'s value.
   */
  Type type_;

  /**
   * Value of the Variant.
   */
  union {
    bool bool_value_;
    int64_t int64_value_;
    double double_value_;
    const char *charptr_value_;
    ScriptableInterface *scriptable_value_;
    Slot *slot_value_;
  } v_;

  /**
   * Because a std::string object has constructors, it is not allowed to be
   * stored in the union.  This field is used to store an independent string
   * value when needed, while @c v_.charptr_value_ is used to store a dependent
   * string value which must be used transiently.
   */ 
  std::string string_value_;

  template <typename T> friend struct VariantType;
  template <typename T> friend struct VariantValue;
};

/**
 * Print a @c Variant into an output stream, only for debugging and testing.
 */
inline std::ostream &operator<<(std::ostream &out, const Variant &v) {
  return out << v.ToString();
}

/**
 * Get the @c Variant::Type of a C++ type.
 * This template is for all integral types.  All unspecialized types will fall
 * into this template, and if it is integral, compilation error may occur.
 */
template <typename T>
struct VariantType {
  static const Variant::Type type = Variant::TYPE_INT64;
};

/**
 * Get the @c Variant::Type of a C++ type.
 * This template is for all <code>ScriptableInterface *</code> types.
 * All unspecialized types will fall into this template, and if it is not
 * <code>ScriptableInterface *</code>, compilation error may occur.
 */
template <typename T>
struct VariantType<T *> {
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
SPECIALIZE_VARIANT_TYPE(float, TYPE_DOUBLE)
SPECIALIZE_VARIANT_TYPE(double, TYPE_DOUBLE)
SPECIALIZE_VARIANT_TYPE(const char *, TYPE_STRING)
SPECIALIZE_VARIANT_TYPE(const std::string &, TYPE_STRING)
SPECIALIZE_VARIANT_TYPE(std::string, TYPE_STRING)
SPECIALIZE_VARIANT_TYPE(Slot *, TYPE_SLOT)
SPECIALIZE_VARIANT_TYPE(Variant, TYPE_VARIANT)

/**
 * Get the value of a @c Variant.
 * This template is for all integral types.  All unspecialized types will fall
 * into this template, and if it is integral, compilation error may occur.
 */
template <typename T>
struct VariantValue {
  T operator()(const Variant &v) {
    ASSERT(v.type_ == Variant::TYPE_INT64);
    return static_cast<T>(v.v_.int64_value_);
  }
};

/**
 * Get the value of a @c Variant.
 * This template is for all <code>ScriptableInterface *</code> types.
 * All unspecialized types will fall into this template, and if it is not
 * <code>ScriptableInterface *</code>, compilation error may occur.
 */
template <typename T>
struct VariantValue<T *> {
  T *operator()(const Variant &v) {
    ASSERT(v.type_ == Variant::TYPE_SCRIPTABLE);
    return v.CheckScriptableType(T::CLASS_ID) ?
           down_cast<T *>(v.v_.scriptable_value_) : NULL;
  }
};

/**
 * Get the @c Variant::Type of a C++ type.
 * Specialized for certain C++ type.
 */
#define SPECIALIZE_VARIANT_VALUE(c_type, variant_field) \
template <>                                             \
struct VariantValue<c_type> {                           \
  c_type operator()(const Variant &v) {                 \
    ASSERT(v.type_ == VariantType<c_type>::type);       \
    return static_cast<c_type>(v.v_.variant_field);     \
  }                                                     \
};

SPECIALIZE_VARIANT_VALUE(char, int64_value_)
SPECIALIZE_VARIANT_VALUE(bool, bool_value_)
SPECIALIZE_VARIANT_VALUE(float, double_value_)
SPECIALIZE_VARIANT_VALUE(double, double_value_)
SPECIALIZE_VARIANT_VALUE(Slot *, slot_value_)

/**
 * Get the value of a @c Variant.
 * Specialized for <code>const char *</code> type.
 * The returned value can be only used transiently, and if the user want to
 * hold this value, a copy must be made.
 */
template <>
struct VariantValue<const char *> {
  const char *operator()(const Variant &v) {
    ASSERT(v.type_ == Variant::TYPE_STRING);
    return v.v_.charptr_value_ ?
           v.v_.charptr_value_ : v.string_value_.c_str();
  }
};

/**
 * Get the value of a @c Variant.
 * Specialized for <code>std::string</code> type.
 */
template <>
struct VariantValue<std::string> {
  std::string operator()(const Variant &v) {
    ASSERT(v.type_ == Variant::TYPE_STRING);
    return v.v_.charptr_value_ ?
           std::string(v.v_.charptr_value_) : v.string_value_;
  }
};

/**
 * Get the value of a @c Variant.
 * Specialized for <code>const std::string &</code> type.
 */
template <>
struct VariantValue<const std::string &> {
  std::string operator()(const Variant &v) {
    ASSERT(v.type_ == Variant::TYPE_STRING);
    return v.v_.charptr_value_ ?
           std::string(v.v_.charptr_value_) : v.string_value_;
  }
};

/**
 * Get the value of a @c Variant.
 * Specialized for <code>Variant</code> type itself.
 */
template <>
struct VariantValue<Variant> {
  const Variant& operator()(const Variant &v) {
    return v;
  }
};

#undef SPECIALIZE_VARIANT_TYPE
#undef SPECIALIZE_VARIANT_VALUE

} // namespace ggadget

#endif // GGADGET_VARIANT_H__
