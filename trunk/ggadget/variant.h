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
#include <ggadget/common.h>
#include <ggadget/unicode_utils.h>

namespace ggadget {

class ScriptableInterface;
class Slot;

/**
 * Used as a wrapper to a string indicating this string contains a JSON
 * expression.  For json, see http://www.json.org.
 */
struct JSONString {
  explicit JSONString(const char *a_value) : value(a_value ? a_value : "") { }
  explicit JSONString(const std::string &a_value) : value(a_value) { }
  bool operator==(const JSONString &another) const {
    return another.value == value;
  }
  std::string value;
};

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
    /** <code>const char *</code> or <code>std::string</code> type. */
    TYPE_STRING,
    /** A string containing a JSON expression */
    TYPE_JSON,
    /** <code>UTF16Char *</code> or <code>UTF16String</code> type. */
    TYPE_UTF16STRING,
    /** <code>ScriptableInterface *</code> type. */
    TYPE_SCRIPTABLE,
    /** <code>const ScriptableInterface *</code> type. */
    TYPE_CONST_SCRIPTABLE,
    /** <code>Slot *</code> type. */
    TYPE_SLOT,
    /** <code>void *</code> type. Only for C++ code, not for script. */
    TYPE_ANY,
    /** const version of @c TYPE_ANY. */
    TYPE_CONST_ANY,
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

  Variant(const Variant &source);

  /**
   * Construct a @c Variant by type with default value (zero).
   */
  explicit Variant(Type a_type) : type_(a_type) {
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

#if GGL_SIZEOF_LONG_INT != 8
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
#endif

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
      : type_(TYPE_STRING) {
    v_.string_value_ = new std::string(value);
  }

  /**
   * Construct a @c Variant with a <code>cost char *</code> value.
   * This @c Variant makes a copy of the value.
   * The type of the constructed @c Variant is @c TYPE_STRING.
   */
  explicit Variant(const char *value) : type_(TYPE_STRING) {
    v_.string_value_ = value ? new std::string(value) : NULL;
  }

  /**
   * Construct a @c Variant with a @c JSONString value.
   * The type of the constructed @c Variant is @c TYPE_JSON.
   */
  explicit Variant(const JSONString &value)
      : type_(TYPE_JSON) {
    v_.string_value_ = new std::string(value.value);
  }

  /**
   * Construct a @c Variant with a @c UTF16String value.
   * The type of the constructed @c Variant is @c TYPE_UTF16STRING.
   */
  explicit Variant(const UTF16String &value)
      : type_(TYPE_UTF16STRING) {
    v_.utf16_string_value_ = new UTF16String(value);
  }

  /**
   * Construct a @c Variant with a <code>cost UTF16Char *</code> value.
   * This @c Variant makes a copy of the value.
   * The type of the constructed @c Variant is @c TYPE_UTF16STRING.
   */
  explicit Variant(const UTF16Char *value) : type_(TYPE_UTF16STRING) {
    v_.utf16_string_value_ = value ? new UTF16String(value) : NULL;
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
   * Construct a @c Variant with a <code>ScriptableInterface *</code> value.
   * This @c Variant doesn't owns the @c Scriptable pointer.
   * The type of the constructed @c Variant is @c TYPE_SCRIPTABLE.
   */
  explicit Variant(const ScriptableInterface *value)
      : type_(TYPE_CONST_SCRIPTABLE) {
    v_.const_scriptable_value_ = value;
  }

  /**
   * Construct a @c Variant with a @c Slot pointer value.
   * This @c Variant doesn't owns the <code>Slot *</code>.
   * The type of the constructed @c Variant is @c TYPE_SLOT.
   */
  explicit Variant(Slot *value) : type_(TYPE_SLOT) {
    v_.slot_value_ = value;
  }

  /** Don't use <code>const Slot *</code> because it is same as non-const. */
  explicit Variant(const Slot *value) {
    ASSERT_M(false, ("Don't use const Slot *"));
  }

  /**
   * Construct a @c Variant with a <code>void *</code> value.
   * It should be only used for C++ APIs, not for scriptable APIs.
   */
  explicit Variant(void *value) : type_(TYPE_ANY) {
    v_.any_value_ = value;
  }

  /**
   * Construct a @c Variant with a <code>const void *</code> value.
   * It should be only used for C++ APIs, not for scriptable APIs.
   */
  explicit Variant(const void *value) : type_(TYPE_CONST_ANY) {
    v_.const_any_value_ = value;
  }

  ~Variant();

  Variant &operator=(const Variant &source);

  /**
   * For testing convenience.
   * Not suggested to use in production code.
   */
  bool operator==(const Variant &another) const;

  std::string ToString() const;

  Type type() const { return type_; }

 private:
  bool CheckScriptableType(uint64_t class_id) const;

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
    std::string *string_value_;  // For both TYPE_STRING and TYPE_JSON.
    UTF16String *utf16_string_value_;
    ScriptableInterface *scriptable_value_;
    const ScriptableInterface *const_scriptable_value_;
    Slot *slot_value_;
    void *any_value_;
    const void *const_any_value_;
  } v_;

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
 * This template is for all integral types.  All unspecialized non-pointer
 * types will fall into this template, and if it is not integral, compilation
 * error may occur.
 */
template <typename T>
struct VariantType {
  static const Variant::Type type = Variant::TYPE_INT64;
};

/**
 * Get the @c Variant::Type of a C++ type.
 * This template is for all <code>ScriptableInterface *</code> types.
 * All unspecialized pointer types will fall into this template, and if it is
 * not <code>ScriptableInterface *</code>, compilation error may occur.
 */
template <typename T>
struct VariantType<T *> {
  static const Variant::Type type = Variant::TYPE_SCRIPTABLE;
};

/**
 * Get the @c Variant::Type of a C++ type.
 * This template is for all <code>const ScriptableInterface *</code> types.
 * All unspecialized const pointer types will fall into this template, and if
 * it is not <code>const ScriptableInterface *</code>, compilation error may
 * occur.
 */
template <typename T>
struct VariantType<const T *> {
  static const Variant::Type type = Variant::TYPE_CONST_SCRIPTABLE;
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
SPECIALIZE_VARIANT_TYPE(std::string, TYPE_STRING)
SPECIALIZE_VARIANT_TYPE(const std::string &, TYPE_STRING)
SPECIALIZE_VARIANT_TYPE(JSONString, TYPE_JSON)
SPECIALIZE_VARIANT_TYPE(const JSONString &, TYPE_JSON)
SPECIALIZE_VARIANT_TYPE(const UTF16Char *, TYPE_UTF16STRING)
SPECIALIZE_VARIANT_TYPE(UTF16String, TYPE_UTF16STRING)
SPECIALIZE_VARIANT_TYPE(const UTF16String &, TYPE_UTF16STRING)
SPECIALIZE_VARIANT_TYPE(Slot *, TYPE_SLOT)
SPECIALIZE_VARIANT_TYPE(void *, TYPE_ANY)
SPECIALIZE_VARIANT_TYPE(const void *, TYPE_CONST_ANY)
SPECIALIZE_VARIANT_TYPE(Variant, TYPE_VARIANT)
SPECIALIZE_VARIANT_TYPE(const Variant &, TYPE_VARIANT)

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
 * All unspecialized pointer types will fall into this template, and if it is
 * not <code>ScriptableInterface *</code>, compilation error may occur.
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
 * Get the value of a @c Variant.
 * This template is for all <code>const ScriptableInterface *</code> types.
 * All unspecialized const pointer types will fall into this template, and if
 * it is not <code>ScriptableInterface *</code>, compilation error may occur.
 */
template <typename T>
struct VariantValue<const T *> {
  const T *operator()(const Variant &v) {
    ASSERT(v.type_ == Variant::TYPE_CONST_SCRIPTABLE ||
           v.type_ == Variant::TYPE_SCRIPTABLE);
    return v.CheckScriptableType(T::CLASS_ID) ?
           down_cast<const T *>(v.v_.const_scriptable_value_) : NULL;
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
SPECIALIZE_VARIANT_VALUE(void *, any_value_)

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
    return v.v_.string_value_ ? v.v_.string_value_->c_str() : NULL;
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
    return v.v_.string_value_ ? *v.v_.string_value_ : std::string();
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
    return v.v_.string_value_ ? *v.v_.string_value_ : std::string();
  }
};

/**
 * Get the value of a @c Variant.
 * Specialized for @c JSONString type.
 */
template <>
struct VariantValue<JSONString> {
  JSONString operator()(const Variant &v) {
    ASSERT(v.type_ == Variant::TYPE_JSON);
    return JSONString(v.v_.string_value_ ? *v.v_.string_value_ : std::string());
  }
};

/**
 * Get the value of a @c Variant.
 * Specialized for <code>const UTF16Char *</code> type.
 * The returned value can be only used transiently, and if the user want to
 * hold this value, a copy must be made.
 */
template <>
struct VariantValue<const UTF16Char *> {
  const UTF16Char *operator()(const Variant &v) {
    ASSERT(v.type_ == Variant::TYPE_UTF16STRING);
    return v.v_.utf16_string_value_ ? v.v_.utf16_string_value_->c_str() : NULL;
  }
};

/**
 * Get the value of a @c Variant.
 * Specialized for <code>UTF16String</code> type.
 */
template <>
struct VariantValue<UTF16String> {
  UTF16String operator()(const Variant &v) {
    ASSERT(v.type_ == Variant::TYPE_UTF16STRING);
    return v.v_.utf16_string_value_ ? *v.v_.utf16_string_value_ : UTF16String();
  }
};

/**
 * Get the value of a @c Variant.
 * Specialized for <code>const UTF16String &</code> type.
 */
template <>
struct VariantValue<const UTF16String &> {
  UTF16String operator()(const Variant &v) {
    ASSERT(v.type_ == Variant::TYPE_UTF16STRING);
    return v.v_.utf16_string_value_ ? *v.v_.utf16_string_value_ : UTF16String();
  }
};

/**
 * Get the value of a @c Variant.
 * Specialized for <code>const JSONString &</code> type.
 */
template <>
struct VariantValue<const JSONString &> {
  JSONString operator()(const Variant &v) {
    ASSERT(v.type_ == Variant::TYPE_JSON);
    return JSONString(v.v_.string_value_ ? *v.v_.string_value_ : std::string());
  }
};

/**
 * Get the value of a @c Variant.
 * Speccialized for <code>const void *</code> type.
 */
template <>
struct VariantValue<const void *> {
  const void *operator()(const Variant &v) {
    ASSERT(v.type_ == Variant::TYPE_ANY || v.type_ == Variant::TYPE_CONST_ANY);
    return v.v_.const_any_value_;
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

/**
 * Get the value of a @c Variant.
 * Specialized for <code>const Variant &</code> type itself.
 */
template <>
struct VariantValue<const Variant &> {
  const Variant& operator()(const Variant &v) {
    return v;
  }
};

template <>
struct VariantValue<void> {
  void operator()(const Variant &v) { }
};

/**
 * Checks if a type is supported by Variant and causes compilation error
 * if the type is not supported.
 *
 * You need not use this when using the VariantValue template, because
 * the template can automatically check the type.
 */
#define CHECK_VARIANT_TYPE(t)  \
    ASSERT(Variant(VariantValue<t>()(Variant(VariantType<t>::type))).type() == \
           VariantType<t>::type)

#undef SPECIALIZE_VARIANT_TYPE
#undef SPECIALIZE_VARIANT_VALUE

} // namespace ggadget

#endif // GGADGET_VARIANT_H__
