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

// gUnit -- the Google C++ Unit Testing Framework
//
// This header file declares functions and macros used internally by
// gUnit.  They are subject to change without notice.

#ifndef TESTING_GUNIT_INTERNAL_H__
#define TESTING_GUNIT_INTERNAL_H__

#ifdef OS_LINUX
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif  // OS_LINUX

#ifndef GUNIT_HAS_STD_STRING
// The user didn't tell us whether ::std::string is available, so we
// need to figure it out.

#ifdef _MSC_VER  // We are on Windows.
// Assumes that exceptions are enabled by default.
#ifndef _HAS_EXCEPTIONS
#define _HAS_EXCEPTIONS 1
#endif  // _HAS_EXCEPTIONS
// On Windows, we can use ::std::string only if exceptions are
// enabled.
#define GUNIT_HAS_STD_STRING _HAS_EXCEPTIONS
#else  // We are on Linux or Mac OS.
#define GUNIT_HAS_STD_STRING 1
#endif  // _MSC_VER

#endif  // GUNIT_HAS_STD_STRING

#if GUNIT_HAS_STD_STRING
#include <string>
#endif

#ifndef GUNIT_HAS_GLOBAL_STRING
// The user didn't tell us whether ::string is available, so we need
// to figure it out.

#ifdef HAS_GLOBAL_STRING
#define GUNIT_HAS_GLOBAL_STRING 1
#else
#define GUNIT_HAS_GLOBAL_STRING 0
#endif  // HAS_GLOBAL_STRING

#endif  // GUNIT_HAS_GLOBAL_STRING

#include <iomanip>
#include <limits>

#ifdef _MSC_VER
#include <strstream>
#else
#include <sstream>
#endif  // _MSC_VER

// std::strstream is deprecated.  However, we have to use it on
// Windows as std::stringstream won't compile on Windows when
// exceptions are disabled.  We use std::stringstream on other
// platforms to avoid compiler warnings there.
#ifdef _MSC_VER
typedef std::strstream StrStream;
#else
typedef std::stringstream StrStream;
#endif  // _MSC_VER

// Due to C++ preprocessor weirdness, we need double indirection to
// concatenate two tokens when one of them is __LINE__.  Writing
//
//   foo ## __LINE__
//
// will result in the token foo__LINE__, instead of foo followed by
// the current line number.  For more details, see
// http://www.parashift.com/c++-faq-lite/misc-technical-issues.html#faq-39.6
#define GUNIT_CONCAT_TOKEN(foo, bar) GUNIT_CONCAT_TOKEN_IMPL(foo, bar)
#define GUNIT_CONCAT_TOKEN_IMPL(foo, bar) foo ## bar

// gUnit defines the testing::Message class to allow construction of
// test messages via the << operator.  The idea is that anything
// streamable to std::ostream can be streamed to a testing::Message.
// This allows a user to use his own types in gUnit assertions by
// overloading the << operator.
//
// C++'s symbol lookup rule (i.e. Koenig lookup) says that these
// overloads are visible in either the std namespace or the global
// namespace, but not other namespaces, including the testing
// namespace which gUnit's Message class is in.
//
// To allow STL containers (and other types that has a << operator
// defined in the global namespace) to be used in gUnit assertions,
// testing::Message must access the custom << operator from the global
// namespace.  Hence this helper function.
//
// Note: Jeffrey Yasskin suggested an alternative fix by "using
// ::operator<<;" in the definition of Message's operator<<.  That fix
// doesn't require a helper function, but unfortunately doesn't
// compile with MSVC.
template <typename T>
inline void GUnitStreamToHelper(std::ostream* os, const T& val) {
  *os << val;
}

// Use this annotation at the end of a struct / class definition to
// prevent the compiler from optimizing away instances that are never
// used.  This is useful when all interesting logic happens inside the
// c'tor and / or d'tor.  Example:
//
//   struct Foo {
//     Foo() { ... }
//   } GUNIT_ATTRIBUTE_UNUSED;
#if defined(_MSC_VER) || (defined(OS_LINUX) && defined(SWIG))
#define GUNIT_ATTRIBUTE_UNUSED
#else
#define GUNIT_ATTRIBUTE_UNUSED __attribute__ ((unused))
#endif

// Tell the compiler to warn about unused return values for functions declared
// with this macro.  The macro should be used on function declarations
// following the argument list:
//
//   Sprocket* AllocateSprocket() GUNIT_MUST_USE_RESULT;
#if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)) \
  && !defined(COMPILER_ICC)
#define GUNIT_MUST_USE_RESULT __attribute__ ((warn_unused_result))
#else
#define GUNIT_MUST_USE_RESULT
#endif  // (__GNUC__ > 3 || __GNUC__ == 3 && __GNUC_MINOR__ >= 4)

// A macro to disallow the evil copy constructor and operator= functions
// This should be used in the private: declarations for a class.
#define GUNIT_DISALLOW_EVIL_CONSTRUCTORS(type)\
  type(const type &);\
  void operator=(const type &)

namespace testing {

// Forward declaration of classes.

struct TraceInfo;                      // Information about a trace point.
template <typename E> class List;      // A generic list.
template <typename E> class ListNode;  // A node in a generic list.
class Message;                         // Represents a failure message.
class TestPartResult;                  // Result of a test part.
class TestResult;                      // Result of a single Test.
class TestInfo;                        // Information about a test.
class TestInfoImpl;                    // Opaque implementation of TestInfo
class TestCase;                        // A collection of related tests.
class UnitTest;                        // A collection of test cases.
class UnitTestImpl;                    // Opaque implementation of UnitTest
class AssertionResult;                 // Result of an assertion.

// String - a UTF-8 string class.
//
// We cannot use std::string as Microsoft's STL implementation in
// Visual C++ 7.1 has problems when exception is disabled.  There is a
// hack to work around this, but we've seen cases where the hack fails
// to work.
//
// Also, String is different from std::string in that it can represent
// both NULL and the empty string, while std::string cannot represent
// NULL.
//
// NULL and the empty string are considered different.  NULL is less
// than anything (including the empty string) except itself.
//
// This class only provides minimum functionality necessary for
// implementing gUnit.  We do not intend to implement a full-fledged
// string class here.
//
// Since the purpose of this class is to provide a substitute for
// std::string on platforms where it cannot be used, we define a copy
// constructor and assignment operators such that we don't need
// conditional compilation in a lot of places.
//
// In order to make the representation efficient, the d'tor of String
// is not virtual.  Therefore DO NOT INHERIT FROM String.
class String {
 public:
  // Static utility methods

  // Returns the input if it's not NULL, otherwise returns "(null)".
  // This function serves two purposes:
  //
  // 1. ShowCString(NULL) has type 'const char *', instead of the
  // type of NULL (which is int).
  //
  // 2. In MSVC, streaming a null char pointer to StrStream generates
  // an access violation, so we need to convert NULL to "(null)"
  // before streaming it.
  static inline const char * ShowCString(const char * c_str) {
    return c_str ? c_str : "(null)";
  }

  // Returns the input enclosed in double quotes if it's not NULL;
  // otherwise returns "(null)".  For example, "\"Hello\"" is returned
  // for input "Hello".
  //
  // This is useful for printing a C string in the syntax of a literal.
  //
  // Known issue: escape sequences are not handled yet.
  static String ShowCStringQuoted(const char* c_str);

  // Clones a 0-terminated C string, allocating memory using new.  The
  // caller is responsible for deleting the return value using
  // delete[].  Returns the cloned string, or NULL if the input is
  // NULL.
  //
  // This is different from strdup() in string.h, which allocates
  // memory using malloc().
  static const char * CloneCString(const char * c_str);

  // Compares two C strings.  Returns true iff they have the same content.
  //
  // Unlike strcmp(), this function can handle NULL argument(s).  A
  // NULL C string is considered different to any non-NULL C string,
  // including the empty string.
  static bool CStringEquals(const char * lhs, const char * rhs);

  // Converts a wide C string to a String using the UTF-8 encoding.
  // NULL will be converted to "(null)".  If an error occurred during
  // the conversion, "(failed to convert from wide string)" is
  // returned.
  static String ShowWideCString(const wchar_t* wide_c_str);

  // Similar to ShowWideCString(), except that this function encloses
  // the converted string in double quotes.
  static String ShowWideCStringQuoted(const wchar_t* wide_c_str);

  // Compares two wide C strings.  Returns true iff they have the same
  // content.
  //
  // Unlike wcscmp(), this function can handle NULL argument(s).  A
  // NULL C string is considered different to any non-NULL C string,
  // including the empty string.
  static bool WideCStringEquals(const wchar_t * lhs, const wchar_t * rhs);

  // Compares two C strings, ignoring case.  Returns true iff they
  // have the same content.
  //
  // Unlike strcasecmp(), this function can handle NULL argument(s).
  // A NULL C string is considered different to any non-NULL C string,
  // including the empty string.
  static bool CaseInsensitiveCStringEquals(const char * lhs,
                                           const char * rhs);

  // Formats a list of arguments to a String, using the same format
  // spec string as for printf.
  //
  // We do not use the StringPrintf class as it is not universally
  // available.
  //
  // The result is limited to 4096 characters (including the tailing
  // 0).  If 4096 characters are not enough to format the input,
  // "<buffer exceeded>" is returned.
  static String Format(const char * format, ...);

  // C'tors

  // The default c'tor constructs a NULL string.
  String() : c_str_(NULL) {}

  // Constructs a String by cloning a 0-terminated C string.
  explicit String(const char * c_str) : c_str_(NULL) {
    *this = c_str;
  }

  // Constructs a String by copying a given number of chars from a
  // buffer.  E.g. String("hello", 3) will create the string "hel".
  String(const char * buffer, size_t len);

  // The copy c'tor creates a new copy of the string.  The two
  // String objects do not share content.
  String(const String & string) : c_str_(NULL) {
    *this = string;
  }

  // D'tor.  String is intended to be a final class, so the d'tor
  // doesn't need to be virtual.
  ~String() { delete[] c_str_; }

  // Returns true iff this is an empty string (i.e. "").
  bool is_empty() const {
    return (c_str_ != NULL) && (*c_str_ == '\0');
  }

  // Compares this with another String.
  // Returns < 0 if this is less than rhs, 0 if this is equal to rhs, or > 0
  // if this is greater than rhs.
  int Compare(const String & rhs) const;

  // Returns true iff this String equals the given C string.  A NULL
  // string and a non-NULL string are considered not equal.
  bool Equals(const char* c_str) const {
    return CStringEquals(c_str_, c_str);
  }

  // Returns true iff this String ends with the given suffix.  *Any*
  // String is considered to end with a NULL or empty suffix.
  bool EndsWith(const char* suffix) const;

  // Returns the length of the encapsulated string, or -1 if the
  // string is NULL.
  int Length() const { return c_str_ ? static_cast<int>(strlen(c_str_)) : -1; }

  // Gets the 0-terminated C string this String object represents.
  // The String object still owns the string.  Therefore the caller
  // should NOT delete the return value.
  const char * c_str() const { return c_str_; }

  // Sets the 0-terminated C string this String object represents.
  // The old string in this object is deleted, and this object will
  // own a clone of the input string.  This function copies only up to
  // length bytes (plus a terminating null byte), or until the first
  // null byte, whichever comes first.
  //
  // This function works even when the c_str parameter has the same
  // value as that of the c_str_ field.
  void Set(const char* c_str, size_t length);

  // Assigns a C string to this object.  Self-assignment works.
  const String& operator=(const char* c_str);

  // Assigns a String object to this object.  Self-assignment works.
  const String& operator=(const String &rhs) {
    *this = rhs.c_str_;
    return *this;
  }

 private:
  const char* c_str_;
};

// Streams a String to an ostream.
inline std::ostream& operator <<(std::ostream& os, const String& str) {
  // We call String::ShowCString() to convert NULL to "(null)".
  // Otherwise we'll get an access violation on Windows.
  return os << String::ShowCString(str.c_str());
}

// Gets the content of the StrStream's buffer as a String.  Each '\0'
// character in the buffer is replaced with "\\0".
String StrStreamToString(StrStream *);

// Appends the user-supplied message to the gUnit-generated message.
String AppendUserMessage(const String& gunit_msg,
                         const Message& user_msg);

// The possible outcomes of a test part.
enum TestPartResultType {
  TPRT_SUCCESS,           // Succeeded.
  TPRT_NONFATAL_FAILURE,  // Failed but the test can continue.
  TPRT_FATAL_FAILURE,     // Failed and the test should be terminated.
};

// A helper class for creating scoped traces in user programs.
class ScopedTrace {
 public:
  // The c'tor pushes the given source file location and message onto
  // a trace stack maintained by gUnit.
  ScopedTrace(const char* file, int line, const Message& message);

  // The d'tor pops the info pushed by the c'tor.
  //
  // Note that the d'tor is not virtual in order to be efficient.
  // Don't inherit from ScopedTrace!
  ~ScopedTrace();

 private:
  GUNIT_DISALLOW_EVIL_CONSTRUCTORS(ScopedTrace);
} GUNIT_ATTRIBUTE_UNUSED;  // A ScopedTrace object does its job in its
                           // c'tor and d'tor.  Therefore it doesn't
                           // need to be used otherwise.

// This template class serves as a compile-time function from size to
// type.  It maps a size in bytes to a primitive type with that
// size. e.g.
//
//   TypeWithSize<4>::UInt
//
// is typedef-ed to be unsigned int (unsigned integer made up of 4
// bytes).
//
// Such functionality should belong to STL, but I cannot find it
// there.
//
// gUnit uses this class in the implementation of floating-point
// comparison.
//
// For now it only handles UInt (unsigned int) as that's all gUnit
// needs.  Other types can be easily added in the future if need
// arises.
template <size_t size>
class TypeWithSize {
 public:
  // This prevents the user from using TypeWithSize<N> with incorrect
  // values of N.
  typedef void UInt;
};

// The specialization for size 4.
template <>
class TypeWithSize<4> {
 public:
  // unsigned int has size 4 in both gcc and MSVC.
  //
  // As base/basictypes.h doesn't compile on Windows, we cannot use
  // uint32, uint64, and etc here.
  typedef int Int;
  typedef unsigned int UInt;
};

// The specialization for size 8.
template <>
class TypeWithSize<8> {
 public:
#ifdef _MSV_VER  // On Windows?
  typedef __int64 Int;
  typedef unsigned __int64 UInt;
#else
  typedef long long Int;
  typedef unsigned long long UInt;
#endif  // _MSC_VER
};

// A type that represents a number of elapsed milliseconds.
typedef TypeWithSize<8>::Int TimeInMillis;

// A class that enables one to stream messages to assertion macros
class AssertHelper {
 public:
  // Constructor.
  AssertHelper(TestPartResultType type, const char* file, int line,
               const char* message);
  // Message assignment is a semantic trick to enable assertion
  // streaming; see the GUNIT_MESSAGE macro below.
  void operator=(const Message& message) const;
 private:
  TestPartResultType const type_;
  const char*        const file_;
  int                const line_;
  String             const message_;
};

// Converts a streamable value to a String.  A NULL pointer is
// converted to "(null)".  When the input value is a ::string,
// ::std::string, ::wstring, or ::std::wstring object, each NUL
// character in it is replaced with "\\0".
template <typename T>
inline String StreamableToString(const T& streamable) {
  return (Message() << streamable).GetString();
}

// Formats a value to be used in a failure message.
//
// The default implementation is to print the value using the
// streaming (<<) operator.
template <typename T>
inline String FormatForFailureMessage(const T& value) {
  return StreamableToString(value);
}

// This partial specialization makes sure that all pointers (including
// those to char or wchar_t) are printed as raw pointers.
template <typename T>
inline String FormatForFailureMessage(T* pointer) {
  return StreamableToString(static_cast<const void*>(pointer));
}

// These overloaded versions handle narrow and wide characters.
String FormatForFailureMessage(char ch);
String FormatForFailureMessage(wchar_t wchar);

// When this operand is a const char* or char*, and the other operand
// is a ::std::string or ::string, we print this operand as a C string
// rather than a pointer.  We do the same for wide strings.

// This internal macro is used to avoid duplicated code.
#define GUNIT_FORMAT_IMPL(operand2_type, operand1_printer)\
inline String FormatForComparisonFailureMessage(\
    operand2_type::value_type* str, const operand2_type& operand2) {\
  return operand1_printer(str);\
}\
inline String FormatForComparisonFailureMessage(\
    const operand2_type::value_type* str, const operand2_type& operand2) {\
  return operand1_printer(str);\
}

#if GUNIT_HAS_STD_STRING
GUNIT_FORMAT_IMPL(::std::string, String::ShowCStringQuoted)
GUNIT_FORMAT_IMPL(::std::wstring, String::ShowWideCStringQuoted)
#endif  // GUNIT_HAS_STD_STRING

#if GUNIT_HAS_GLOBAL_STRING
GUNIT_FORMAT_IMPL(::string, String::ShowCStringQuoted)
GUNIT_FORMAT_IMPL(::wstring, String::ShowWideCStringQuoted)
#endif  // GUNIT_HAS_GLOBAL_STRING

#undef GUNIT_FORMAT_IMPL

// Constructs and returns the message for an equality assertion
// (e.g. ASSERT_EQ, EXPECT_STREQ, etc) failure.
//
// The first four parameters are the expressions used in the assertion
// and their values, as strings.  For example, for ASSERT_EQ(foo, bar)
// where foo is 5 and bar is 6, we have:
//
//   expected_expression: "foo"
//   actual_expression:   "bar"
//   expected_value:      "5"
//   actual_value:        "6"
//
// The ignoring_case parameter is true iff the assertion is a
// *_STRCASEEQ*.  When it's true, the string " (ignoring case)" will
// be inserted into the message.
AssertionResult EqFailure(const char* expected_expression,
                          const char* actual_expression,
                          const String& expected_value,
                          const String& actual_value,
                          bool ignoring_case);


AssertionResult AssertionSuccess();

// This template class represents an IEEE floating-point number
// (either single-precision or double-precision, depending on the
// template parameters).
//
// The purpose of this class is to do more sophisticated number
// comparison.  (Due to round-off error, etc, it's very unlikely that
// two floating-points will be equal exactly.  Hence a naive
// comparison by the == operation often doesn't work.)
//
// Format of IEEE floating-point:
//
//   The most-significant bit being the leftmost, an IEEE
//   floating-point looks like
//
//     sign_bit exponent_bits fraction_bits
//
//   Here, sign_bit is a single bit that designates the sign of the
//   number.
//
//   For float, there are 8 exponent bits and 23 fraction bits.
//
//   For double, there are 11 exponent bits and 52 fraction bits.
//
//   More details can be found at
//   http://en.wikipedia.org/wiki/IEEE_floating-point_standard.
//
// Template parameter:
//
//   RawType: the raw floating-point type (either float or double)
template <typename RawType>
class FloatingPoint {
 public:
  // Defines the unsigned integer type that has the same size as the
  // floating point number.
  typedef typename TypeWithSize<sizeof(RawType)>::UInt Bits;

  // Constants.

  // # of bits in a number.
  static const size_t kBitCount = 8*sizeof(RawType);

  // # of fraction bits in a number.
  static const size_t kFractionBitCount =
    std::numeric_limits<RawType>::digits - 1;

  // # of exponent bits in a number.
  static const size_t kExponentBitCount = kBitCount - 1 - kFractionBitCount;

  // The mask for the sign bit.
  static const Bits kSignBitMask = static_cast<Bits>(1) << (kBitCount - 1);

  // The mask for the fraction bits.
  static const Bits kFractionBitMask =
    ~static_cast<Bits>(0) >> (kExponentBitCount + 1);

  // The mask for the exponent bits.
  static const Bits kExponentBitMask = ~(kSignBitMask | kFractionBitMask);

  // How many ULP's (Units in the Last Place) we want to tolerate when
  // comparing two numbers.  The larger the value, the more error we
  // allow.  A 0 value means that two numbers must be exactly the same
  // to be considered equal.
  //
  // The maximum error of a single floating-point operation is 0.5
  // units in the last place.  On Intel CPU's, all floating-point
  // calculations are done with 80-bit precision, while double has 64
  // bits.  Therefore, 4 should be enough for ordinary use.
  //
  // See the following article for more details on ULP:
  // http://www.cygnus-software.com/papers/comparingfloats/comparingfloats.htm.
  static const size_t kMaxUlps = 4;

  // Constructs a FloatingPoint from a raw floating-point number.
  //
  // On an Intel CPU, passing a non-normalized NAN (Not a Number)
  // around may change its bits, although the new value is guaranteed
  // to be also a NAN.  Therefore, don't expect this constructor to
  // preserve the bits in x when x is a NAN.
  explicit FloatingPoint(const RawType& x) : value_(x) {}

  // Static methods

  // Reinterprets a bit pattern as a floating-point number.
  //
  // This function is needed to test the AlmostEquals() method.
  static RawType ReinterpretBits(const Bits bits) {
    FloatingPoint fp(0);
    fp.bits_ = bits;
    return fp.value_;
  }

  // Returns the floating-point number that represent positive infinity.
  static RawType Infinity() {
    return ReinterpretBits(kExponentBitMask);
  }

  // Non-static methods

  // Returns the bits that represents this number.
  const Bits &bits() const { return bits_; }

  // Returns the exponent bits of this number.
  Bits exponent_bits() const { return kExponentBitMask & bits_; }

  // Returns the fraction bits of this number.
  Bits fraction_bits() const { return kFractionBitMask & bits_; }

  // Returns the sign bit of this number.
  Bits sign_bit() const { return kSignBitMask & bits_; }

  // Returns true iff this is NAN (not a number).
  bool is_nan() const {
    // It's a NAN if the exponent bits are all ones and the fraction
    // bits are not entirely zeros.
    return (exponent_bits() == kExponentBitMask) && (fraction_bits() != 0);
  }

  // Returns true iff this number is at most kMaxUlps ULP's away from
  // rhs.  In particular, this function:
  //
  //   - returns false if either number is (or both are) NAN.
  //   - treats really large numbers as almost equal to infinity.
  //   - thinks +0.0 and -0.0 are 0 DLP's apart.
  bool AlmostEquals(const FloatingPoint& rhs) const {
    // The IEEE standard says that any comparison operation involving
    // a NAN must return false.
    if (is_nan() || rhs.is_nan()) return false;

    return DistanceBetweenSignAndMagnitudeNumbers(bits_, rhs.bits_) <= kMaxUlps;
  }

 private:
  // Converts an integer from the sign-and-magnitude representation to
  // the biased representation.  More precisely, let N be 2 to the
  // power of (kBitCount - 1), an integer x is represented by the
  // unsigned number x + N.
  //
  // For instance,
  //
  //   -N + 1 (the most negative number representable using
  //          sign-and-magnitude) is represented by 1;
  //   0      is represented by N; and
  //   N - 1  (the biggest number representable using
  //          sign-and-magnitude) is represented by 2N - 1.
  //
  // Read http://en.wikipedia.org/wiki/Signed_number_representations
  // for more details on signed number representations.
  static Bits SignAndMagnitudeToBiased(const Bits &sam) {
    if (kSignBitMask & sam) {
      // sam represents a negative number.
      return ~sam + 1;
    } else {
      // sam represents a positive number.
      return kSignBitMask | sam;
    }
  }

  // Given two numbers in the sign-and-magnitude representation,
  // returns the distance between them as an unsigned number.
  static Bits DistanceBetweenSignAndMagnitudeNumbers(const Bits &sam1,
                                                     const Bits &sam2) {
    const Bits biased1 = SignAndMagnitudeToBiased(sam1);
    const Bits biased2 = SignAndMagnitudeToBiased(sam2);
    return (biased1 >= biased2) ? (biased1 - biased2) : (biased2 - biased1);
  }

  union {
    RawType value_;  // The raw floating-point number.
    Bits bits_;      // The bits that represent the number.
  };
};

// Typedefs the instances of the FloatingPoint template class that we
// care to use.
typedef FloatingPoint<float> Float;
typedef FloatingPoint<double> Double;

// In order to catch the mistake of putting tests that use different
// test fixture classes in the same test case, we need to assign
// unique IDs to fixture classes and compare them.  The TypeId type is
// used to hold such IDs.  The user should treat TypeId as an opaque
// type: the only operation allowed on TypeId values is to compare
// them for equality using the == operator.
typedef void* TypeId;

// GetTypeId<T>() returns the ID of type T.  Different values will be
// returned for different types.  Calling the function twice with the
// same type argument is guaranteed to return the same ID.
template <typename T>
inline TypeId GetTypeId() {
  static bool dummy = false;
  // The compiler is required to create an instance of the static
  // variable dummy for each T used to instantiate the template.
  // Therefore, the address of dummy is guaranteed to be unique.
  return &dummy;
}

}  // namespace testing

#define GUNIT_MESSAGE(message, result_type) \
  ::testing::AssertHelper(result_type, __FILE__, __LINE__, message) = \
    ::testing::Message()

#define GUNIT_FATAL_FAILURE(message) \
  return GUNIT_MESSAGE(message, ::testing::TPRT_FATAL_FAILURE)

#define GUNIT_NONFATAL_FAILURE(message) \
  GUNIT_MESSAGE(message, ::testing::TPRT_NONFATAL_FAILURE)

#define GUNIT_SUCCESS(message) \
  GUNIT_MESSAGE(message, ::testing::TPRT_SUCCESS)

#define GUNIT_TEST_BOOLEAN(boolexpr, booltext, actual, expected, fail) \
  if (boolexpr) \
    ; \
  else \
    fail("Value of: " booltext "\n  Actual: " #actual "\nExpected: " #expected)

#define GUNIT_EXPECT_BOOLEAN(boolexpr, booltext, actual, expected) \
  GUNIT_TEST_BOOLEAN(boolexpr, booltext, actual, expected, \
                     GUNIT_NONFATAL_FAILURE)

#define GUNIT_ASSERT_BOOLEAN(boolexpr, booltext, actual, expected) \
  GUNIT_TEST_BOOLEAN(boolexpr, booltext, actual, expected, \
                     GUNIT_FATAL_FAILURE)

// Helper macro for defining tests.
#define GUNIT_TEST(test_case_name, test_name, parent_class)\
class test_case_name##_##test_name##_Test : public parent_class {\
 public:\
  test_case_name##_##test_name##_Test() {}\
  static ::testing::Test* NewTest() {\
    return new test_case_name##_##test_name##_Test;\
  }\
 private:\
  virtual void TestBody();\
  static ::testing::TestInfo* const test_info_;\
  GUNIT_DISALLOW_EVIL_CONSTRUCTORS(test_case_name##_##test_name##_Test);\
};\
\
::testing::TestInfo* const test_case_name##_##test_name##_Test::test_info_ =\
  ::testing::TestInfo::MakeAndRegisterInstance(\
    #test_case_name, \
    #test_name, \
    ::testing::GetTypeId<parent_class>(), \
    parent_class::SetUpTestCase, \
    parent_class::TearDownTestCase, \
    test_case_name##_##test_name##_Test::NewTest);\
void test_case_name##_##test_name##_Test::TestBody()


#endif  // TESTING_BASE_GUNIT_INTERNAL_H__
