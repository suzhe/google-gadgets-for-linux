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

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef OS_LINUX
#include <fcntl.h>
#include <limits.h>
// Declares vsnprintf().  This header is not available on Windows.
#include <strings.h>
#include <sys/time.h>
#include <unistd.h>
#endif  // OS_LINUX

// _MSC_VER is defined iff the code is compiled by MSVC.
#ifdef _MSC_VER
#include <sys/timeb.h>
#include <windows.h>
#endif  // _MSC_VER

#include <vector>

// Indicates that this translation unit is part of gUnit's
// implementation.  It must come before gunit-internal-inl.h is
// included, or there will be a compiler error.  This trick is to
// prevent a user from accidentally including gunit-internal-inl.h in
// his code.
#define GUNIT_IMPLEMENTATION

#include "gunit-internal-inl.h"
#include "gunit.h"

// _MSC_VER is defined iff the code is compiled by MSVC.
#ifdef _MSC_VER
#define vsnprintf _vsnprintf
#endif  // _MSC_VER

namespace testing {

// Constants.

// The environment variable that controls whether a failed assertion
// should cause the debugger to break.
static const char kBreakOnFailureEnvVar[] = "GUNIT_BREAK_ON_FAILURE";

// The environment variable that controls whether gUnit should catch
// exceptions thrown by the tests.
static const char kCatchExceptionsEnvVar[] = "GUNIT_CATCH_EXCEPTIONS";

// The environment variable for specifying the test filter.
static const char kFilterEnvVar[] = "GUNIT_FILTER";

// The environment variable for specifying the death test style.
static const char kDeathTestStyleEnvVar[] = "GUNIT_DEATH_TEST_STYLE";

// A test that matches this pattern is disabled and not run.
static const char kDisableTestPattern[] = "DISABLED_*";

// A test filter that matches everything.
static const char kUniversalFilter[] = "*";

// The default death test style.
static const char kDefaultDeathTestStyle[] = "noexec";

// The environment variable for specifying alternative output.
static const char kOutputEnvVar[] = "GUNIT_OUTPUT";

// The environment variable for specifying the number of stack frames
// to be included in failure messages.
static const char kStacktraceDepthEnvVar[] = "GUNIT_STACK_TRACE_DEPTH";

// The default output file for XML output.
static const char kDefaultOutputFile[] = "test_detail.xml";

// Defines the --gunit_break_on_failure flag.
const char kGUnitBreakOnFailureFlag[] = "gunit_break_on_failure";
bool FLAGS_gunit_break_on_failure = false;

// Defines the --gunit_catch_exceptions flag.
const char kGUnitCatchExceptionsFlag[] = "gunit_catch_exceptions";
bool FLAGS_gunit_catch_exceptions = false;

// Defines the --gunit_filter flag.
const char kGUnitFilterFlag[] = "gunit_filter";
String FLAGS_gunit_filter(kUniversalFilter);

// Defines the --gunit_list_tests flag.
const char kGUnitListTestsFlag[] = "gunit_list_tests";
bool FLAGS_gunit_list_tests = false;

// Defines the --gunit_output flag.
const char kGUnitOutputFlag[] = "gunit_output";
String FLAGS_gunit_output("");

// Iterates over a list of TestCases, keeping a running sum of the
// results of calling a given int-returning method on each.
// Returns the sum.
static int SumOverTestCaseList(const List<TestCase*>& case_list,
                               int (TestCase::*method)() const) {
  int sum = 0;
  for (const ListNode<TestCase*>* node = case_list.Head();
       node != NULL;
       node = node->next()) {
    sum += (node->element()->*method)();
  }
  return sum;
}

// Returns true iff the test case passed.
static bool TestCasePassed(const TestCase* test_case) {
  return test_case->should_run() && test_case->Passed();
}

// Returns true iff the test case failed.
static bool TestCaseFailed(const TestCase* test_case) {
  return test_case->should_run() && test_case->Failed();
}

// Returns true iff test_case contains at least one test that should
// run.
static bool ShouldRunTestCase(const TestCase* test_case) {
  return test_case->should_run();
}

// Convenience function for accessing the global UnitTest
// implementation object.
static inline UnitTestImpl* GetUnitTestImpl() {
  return UnitTest::GetInstance()->impl();
}

// A wrapper for getenv() that works on Linux, Windows, and Mac OS.
static inline const char* GetEnv(const char* name) {
  // MSVC 8 deprecates getenv(), so we want to suppress warning 4996
  // (deprecated function) there.
#ifdef _MSC_VER  // We are on Windows
# pragma warning(push)          // Saves the current warning state.
# pragma warning(disable:4996)  // Temporarily disables warning 4996.
  return getenv(name);
# pragma warning(pop)           // Restores the warning state.
#else  // We are on Linux or Mac OS.
  return getenv(name);
#endif  // _MSC_VER
}

// AssertHelper constructor.
AssertHelper::AssertHelper(TestPartResultType type, const char* file,
                           int line, const char* message)
    : type_(type), file_(file), line_(line), message_(message) {
}

// Message assignment, for assertion streaming support.
void AssertHelper::operator=(const Message& message) const {
  UnitTest::GetInstance()->
    AddTestPartResult(type_, String(file_), line_,
                      AppendUserMessage(message_, message),
                      UnitTest::GetInstance()->impl()
                      ->CurrentOsStackTraceExceptTop(1)
                      // Skips the stack frame for this function itself.
                      );
}

// Class UnitTestOptions.

// Reads and returns a string environment variable; if it's not set,
// returns default_value.
const char* UnitTestOptions::ReadStringEnvVar(const char* env_var,
                                              const char* default_value) {
  const char* const value = GetEnv(env_var);
  return value == NULL ? default_value : value;
}

// Reads and returns a Boolean environment variable; if it's not set,
// returns default_value.
//
// The value is considered true iff it's not "0".
bool UnitTestOptions::ReadBoolEnvVar(const char* env_var, bool default_value) {
  const char* const string_value = GetEnv(env_var);
  return string_value == NULL ?
      default_value : strcmp(string_value, "0") != 0;
}

// Reads and returns a 32-bit integer stored in an environment
// variable; if it isn't set or doesn't represent a valid 32-bit
// integer, returns default_value.
Int32 UnitTestOptions::ReadInt32EnvVar(const char* env_var,
                                       Int32 default_value) {
  const char* const string_value = GetEnv(env_var);
  if (string_value == NULL) {
    // The environment variable is not set.
    return default_value;
  }

  // Parses the environment variable as a decimal integer.
  char* end;
  const long long_value = strtol(string_value, &end, 10);

  // Has strtol() consumed all characters in the string?
  if (*end != '\0') {
    // No - an invalid character was encountered.
    Message msg;
    msg << "WARNING: Environment variable " << env_var
        << " is expected to be a 32-bit integer, but actually"
        << " has value \"" << string_value << "\".  The default value "
        << default_value << " is used.\n";
    printf("%s", msg.GetString().c_str());
    return default_value;
  }

  // Is the parsed value in the range of an Int32?
  const Int32 result = static_cast<Int32>(long_value);
  if (long_value == LONG_MAX || long_value == LONG_MIN ||
      // The parsed value overflows as a long.  (strtol() returns
      // LONG_MAX or LONG_MIN when the input overflows.)
      result != long_value
      // The parsed value overflows as an Int32.
      ) {
    Message msg;
    msg << "WARNING: Environment variable " << env_var
        << " is expected to be a 32-bit integer, but actually"
        << " has value " << string_value << ", which overflows.  "
        << "The default value " << default_value << " is used.\n";
    printf("%s", msg.GetString().c_str());
    return default_value;
  }

  return result;
}

// Copies the values of the gUnit environment variables to the flag
// variables.
//
// This function must be called before the command line is parsed in
// main(), in order to allow command line flags to override the
// environment variables.  We call it in the constructor of
// UnitTestImpl, which is a singleton created when the first test is
// defined, before main() is reached.
void UnitTestOptions::SetFlagVarsFromEnvVars() {
  FLAGS_gunit_break_on_failure =
      ReadBoolEnvVar(kBreakOnFailureEnvVar, false);
  FLAGS_gunit_filter = ReadStringEnvVar(kFilterEnvVar, kUniversalFilter);
  FLAGS_gunit_list_tests = false;
  FLAGS_gunit_output = ReadStringEnvVar(kOutputEnvVar, "");
  FLAGS_gunit_catch_exceptions =
      ReadBoolEnvVar(kCatchExceptionsEnvVar, false);
}

// Functions for processing the gunit_output flag.

// Returns the output format, or "" for normal printed output.
String UnitTestOptions::GetOutputFormat() {
  const char* const gunit_output_flag = FLAGS_gunit_output.c_str();
  if (gunit_output_flag == NULL) return String("");

  const char* const colon = strchr(gunit_output_flag, ':');
  return (colon == NULL) ?
      String(gunit_output_flag) :
      String(gunit_output_flag, colon - gunit_output_flag);
}

// Returns the name of the requested output file, or the default if none
// was explicitly specified.
String UnitTestOptions::GetOutputFile() {
  const char* const gunit_output_flag = FLAGS_gunit_output.c_str();
  if (gunit_output_flag == NULL) return String("");

  const char* const colon = strchr(gunit_output_flag, ':');
  return (colon == NULL) ?
      String(kDefaultOutputFile) :
      String(colon + 1);
}

// Returns true iff the wildcard pattern matches the string.  The
// first ':' or '\0' character in pattern marks the end of it.
//
// This recursive algorithm isn't very efficient, but is clear and
// works well enough for matching test names, which are short.
bool UnitTestOptions::PatternMatchesString(const char *pattern,
                                           const char *str) {
  switch (*pattern) {
    case '\0':
    case ':':  // Either ':' or '\0' marks the end of the pattern.
      return *str == '\0';
    case '?':  // Matches any single character.
      return *str != '\0' && PatternMatchesString(pattern + 1, str + 1);
    case '*':  // Matches any string (possibly empty) of characters.
      return (*str != '\0' && PatternMatchesString(pattern, str + 1)) ||
          PatternMatchesString(pattern + 1, str);
    default:  // Non-special character.  Matches itself.
      return *pattern == *str &&
          PatternMatchesString(pattern + 1, str + 1);
  }
}

bool UnitTestOptions::MatchesFilter(const String& name, const char* filter) {
  const char *cur_pattern = filter;
  while (true) {
    if (PatternMatchesString(cur_pattern, name.c_str())) {
      return true;
    }

    // Finds the next pattern in the filter.
    cur_pattern = strchr(cur_pattern, ':');

    // Returns if no more pattern can be found.
    if (cur_pattern == NULL) {
      return false;
    }

    // Skips the pattern separater (the ':' character).
    cur_pattern++;
  }
}

// Returns true iff the user-specified filter matches the test case
// name and the test name.
bool UnitTestOptions::FilterMatchesTest(const String &test_case_name,
                                        const String &test_name) {
  const String& full_name = String::Format("%s.%s",
                                           test_case_name.c_str(),
                                           test_name.c_str());

  // Split --gunit_filter at '-', if there is one, to separate into
  // positive filter and negative filter portions
  const char* const p = FLAGS_gunit_filter.c_str();
  const char* const dash = strchr(p, '-');
  String positive;
  String negative;
  if (dash == NULL) {
    positive = FLAGS_gunit_filter.c_str(); // Whole string is a positive filter
    negative = String("");
  } else {
    positive.Set(p, dash - p);       // Everything up to the dash
    negative = String(dash+1);       // Everything after the dash
    if (positive.is_empty()) {
      // Treat '-test1' as the same as '*-test1'
      positive = kUniversalFilter;
    }
  }

  // A filter is a colon-separated list of patterns.  It matches a
  // test if any pattern in it matches the test.
  return (MatchesFilter(full_name, positive.c_str()) &&
          !MatchesFilter(full_name, negative.c_str()));
}

#ifdef _MSC_VER  // We are on Windows.
// Returns EXCEPTION_EXECUTE_HANDLER if gUnit should handle the
// given SEH exception, or EXCEPTION_CONTINUE_SEARCH otherwise.
// This function is useful as an __except condition.
int UnitTestOptions::GUnitShouldProcessSEH(DWORD exception_code) {
  // gUnit should handle an exception if:
  //   1. the user wants it to, AND
  //   2. this is not a breakpoint exception.
  return (FLAGS_gunit_catch_exceptions &&
          exception_code != EXCEPTION_BREAKPOINT) ?
      EXCEPTION_EXECUTE_HANDLER :
      EXCEPTION_CONTINUE_SEARCH;
}
#endif  // _MSC_VER

// The interface for printing the result of a UnitTest
class UnitTestEventListenerInterface {
 public:
  // The d'tor is pure virtual as this is an abstract class.
  virtual ~UnitTestEventListenerInterface() = 0;

  // Called before the unit test starts.
  virtual void OnUnitTestStart(const UnitTest * unit_test) {}

  // Called after the unit test ends.
  virtual void OnUnitTestEnd(const UnitTest * unit_test) {}

  // Called before the test case starts.
  virtual void OnTestCaseStart(const TestCase * test_case) {}

  // Called after the test case ends.
  virtual void OnTestCaseEnd(const TestCase * test_case) {}

  // Called before the test starts.
  virtual void OnTestStart(const TestInfo * test_info) {}

  // Called after the test ends.
  virtual void OnTestEnd(const TestInfo * test_info) {}

  // Called after an assertion.
  virtual void OnNewTestPartResult(const TestPartResult * result) {}
};

// Gets the number of successful test cases.
int UnitTestImpl::successful_test_case_count() const {
  return test_cases_.CountIf(TestCasePassed);
}

// Gets the number of failed test cases.
int UnitTestImpl::failed_test_case_count() const {
  return test_cases_.CountIf(TestCaseFailed);
}

// Gets the number of all test cases.
int UnitTestImpl::total_test_case_count() const {
  return test_cases_.size();
}

// Gets the number of all test cases that contain at least one test
// that should run.
int UnitTestImpl::test_case_to_run_count() const {
  return test_cases_.CountIf(ShouldRunTestCase);
}

// Gets the number of successful tests.
int UnitTestImpl::successful_test_count() const {
  return SumOverTestCaseList(test_cases_, &TestCase::successful_test_count);
}

// Gets the number of failed tests.
int UnitTestImpl::failed_test_count() const {
  return SumOverTestCaseList(test_cases_, &TestCase::failed_test_count);
}

// Gets the number of disabled tests.
int UnitTestImpl::disabled_test_count() const {
  return SumOverTestCaseList(test_cases_, &TestCase::disabled_test_count);
}

// Gets the number of all tests.
int UnitTestImpl::total_test_count() const {
  return SumOverTestCaseList(test_cases_, &TestCase::total_test_count);
}

// Gets the number of tests that should run.
int UnitTestImpl::test_to_run_count() const {
  return SumOverTestCaseList(test_cases_, &TestCase::test_to_run_count);
}

// Returns the current OS stack trace as a String.
//
// The maximum number of stack frames to be included is specified by
// the gunit_stack_trace_depth flag.  The skip_count parameter
// specifies the number of top frames to be skipped, which doesn't
// count against the number of frames to be included.
//
// For example, if Foo() calls Bar(), which in turn calls
// CurrentOsStackTraceExceptTop(1), Foo() will be included in the
// trace but Bar() and CurrentOsStackTraceExceptTop() won't.
String UnitTestImpl::CurrentOsStackTraceExceptTop(int skip_count) {
#ifdef GUNIT_LINUX_GOOGLE3_MODE
  return os_stack_trace_getter()->CurrentStackTrace(
      static_cast<int>(FLAGS_gunit_stack_trace_depth),
      skip_count + 1
      // Skips the user-specified number of frames plus this function
      // itself.
      );
#else
  return String("");
#endif  // GUNIT_LINUX_GOOGLE3_MODE
}


static TimeInMillis GetTimeInMillis() {
#ifdef _MSC_VER  // We are on Windows.
  __timeb64 now;
  // MSVC 8 deprecates _ftime64(), so we want to suppress warning 4996
  // (deprecated function) there.
# pragma warning(push)          // Saves the current warning state.
# pragma warning(disable:4996)  // Temporarily disables warning 4996.
  _ftime64(&now);
# pragma warning(pop)           // Restores the warning state.
  return static_cast<TimeInMillis>(now.time) * 1000 + now.millitm;
#else  // We are on Linux or Mac OS.
  struct timeval now;
  gettimeofday(&now, NULL);
  return static_cast<TimeInMillis>(now.tv_sec) * 1000 + now.tv_usec / 1000;
#endif  // _MSC_VER
}



// Utilities

// class String

// Returns the input enclosed in double quotes if it's not NULL;
// otherwise returns "(null)".  For example, "\"Hello\"" is returned
// for input "Hello".
//
// This is useful for printing a C string in the syntax of a literal.
//
// Known issue: escape sequences are not handled yet.
String String::ShowCStringQuoted(const char* c_str) {
  return c_str ? String::Format("\"%s\"", c_str) : String("(null)");
}

// Copies at most length characters from str into a newly-allocated
// piece of memory of size length+1.  The memory is allocated with new[].
// A terminating null byte is written to the memory, and a pointer to it
// is returned.  If str is NULL, NULL is returned.
static char* CloneString(const char* str, size_t length) {
  if (str == NULL) {
    return NULL;
  } else {
    char* const clone = new char[length + 1];
    // MSVC 8 deprecates strncpy(), so we want to suppress warning
    // 4996 (deprecated function) there.
#ifdef _MSC_VER  // We are on Windows.
# pragma warning(push)          // Saves the current warning state.
# pragma warning(disable:4996)  // Temporarily disables warning 4996.
    strncpy(clone, str, length);
# pragma warning(pop)           // Restores the warning state.
#else  // We are on Linux or Mac OS.
    strncpy(clone, str, length);
#endif  // _MSC_VER
    clone[length] = '\0';
    return clone;
  }
}

// Clones a 0-terminated C string, allocating memory using new.  The
// caller is responsible for deleting[] the return value.  Returns the
// cloned string, or NULL if the input is NULL.
const char * String::CloneCString(const char* c_str) {
  return (c_str == NULL) ?
                    NULL : CloneString(c_str, strlen(c_str));
}

// Compares two C strings.  Returns true iff they have the same content.
//
// Unlike strcmp(), this function can handle NULL argument(s).  A NULL
// C string is considered different to any non-NULL C string,
// including the empty string.
bool String::CStringEquals(const char * lhs, const char * rhs) {
  if ( lhs == NULL ) return rhs == NULL;

  if ( rhs == NULL ) return false;

  return strcmp(lhs, rhs) == 0;
}

// Converts an array of wide chars to a narrow string using the UTF-8
// encoding, and streams the result to the given Message object.
static void StreamWideCharsToMessage(const wchar_t* wstr, size_t len,
                                     Message* msg) {
  for (size_t i = 0; i != len; i++) {
    // TODO(wan): consider allowing a testing::String object to
    // contain '\0'.  This will make it behave more like std::string,
    // and will allow ToUtf8String() to return the correct encoding
    // for '\0' s.t. we can get rid of the conditional here (and in
    // several other places).
    if (wstr[i]) {
      *msg << ToUtf8String(wstr[i]);
    } else {
      *msg << '\0';
    }
  }
}

#if GUNIT_HAS_STD_STRING
// Converts the given wide string to a narrow string using the UTF-8
// encoding, and streams the result to this Message object.
Message& Message::operator <<(const ::std::wstring& wstr) {
  StreamWideCharsToMessage(wstr.c_str(), wstr.length(), this);
  return *this;
}
#endif  // GUNIT_HAS_STD_STRING

#if GUNIT_HAS_GLOBAL_STRING
// Converts the given wide string to a narrow string using the UTF-8
// encoding, and streams the result to this Message object.
Message& Message::operator <<(const ::wstring& wstr) {
  StreamWideCharsToMessage(wstr.c_str(), wstr.length(), this);
  return *this;
}
#endif  // GUNIT_HAS_GLOBAL_STRING

// Formats a value to be used in a failure message.

// For a char value, we print it as a C++ char literal and as an
// unsigned integer (both in decimal and in hexadecimal).
String FormatForFailureMessage(char ch) {
  const unsigned int ch_as_uint = ch;
  // A String object cannot contain '\0', so we print "\\0" when ch is
  // '\0'.
  return String::Format("'%s' (%u, 0x%X)",
                        ch ? String::Format("%c", ch).c_str() : "\\0",
                        ch_as_uint, ch_as_uint);
}

// For a wchar_t value, we print it as a C++ wchar_t literal and as an
// unsigned integer (both in decimal and in hexidecimal).
String FormatForFailureMessage(wchar_t wchar) {
  // The C++ standard doesn't specify the exact size of the wchar_t
  // type.  It just says that it shall have the same size as another
  // integral type, called its underlying type.
  //
  // Therefore, in order to print a wchar_t value in the numeric form,
  // we first convert it to the largest integral type (UInt64) and
  // then print the converted value.
  //
  // We use streaming to print the value as "%llu" doesn't work
  // correctly with MSVC 7.1.
  const UInt64 wchar_as_uint64 = wchar;
  Message msg;
  // A String object cannot contain '\0', so we print "\\0" when wchar is
  // L'\0'.
  msg << "L'" << (wchar ? ToUtf8String(wchar).c_str() : "\\0") << "' ("
      << wchar_as_uint64 << ", 0x" << ::std::setbase(16)
      << wchar_as_uint64 << ")";
  return msg.GetString();
}


// AssertionResult constructor.
AssertionResult::AssertionResult(const String& failure_message)
    : failure_message_(failure_message) {
}


// Makes a successful assertion result.
AssertionResult AssertionSuccess() {
  return AssertionResult();
}


// Makes a failed assertion result with the given failure message.
AssertionResult AssertionFailure(const Message& message) {
  return AssertionResult(message.GetString());
}


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
                          bool ignoring_case) {
  Message msg;
  msg << "Value of: " << actual_expression;
  if (!actual_value.Equals(actual_expression)) {
    msg << "\n  Actual: " << actual_value;
  }

  msg << "\nExpected: " << expected_expression;
  if (ignoring_case) {
    msg << " (ignoring case)";
  }
  if (!expected_value.Equals(expected_expression)) {
    msg << "\nWhich is: " << expected_value;
  }

  return AssertionFailure(msg);
}


// Helper function for implementing ASSERT_NEAR.
AssertionResult DoubleNearPredFormat(const char* expr1,
                                     const char* expr2,
                                     const char* abs_error_expr,
                                     double val1,
                                     double val2,
                                     double abs_error) {
  const double diff = fabs(val1 - val2);
  if (diff <= abs_error) return AssertionSuccess();

  // TODO(wan): do not print the value of an expression if it's
  // already a literal.
  Message msg;
  msg << "The difference between " << expr1 << " and " << expr2
      << " is " << diff << ", which exceeds " << abs_error_expr << ", where\n"
      << expr1 << " evaluates to " << val1 << ",\n"
      << expr2 << " evaluates to " << val2 << ", and\n"
      << abs_error_expr << " evaluates to " << abs_error << ".";
  return AssertionFailure(msg);
}


// Helper template for implementing FloatLE() and DoubleLE().
template <typename RawType>
AssertionResult FloatingPointLE(const char* expr1,
                                const char* expr2,
                                RawType val1,
                                RawType val2) {
  // Returns success if val1 is less than val2,
  if (val1 < val2) {
    return AssertionSuccess();
  }

  // or if val1 is almost equal to val2.
  const FloatingPoint<RawType> lhs(val1), rhs(val2);
  if (lhs.AlmostEquals(rhs)) {
    return AssertionSuccess();
  }

  // Note that the above two checks will both fail if either val1 or
  // val2 is NaN, as the IEEE floating-point standard requires that
  // any predicate involving a NaN must return false.

  StrStream val1_ss;
  val1_ss << std::setprecision(std::numeric_limits<RawType>::digits10 + 2)
          << val1;

  StrStream val2_ss;
  val2_ss << std::setprecision(std::numeric_limits<RawType>::digits10 + 2)
          << val2;

  Message msg;
  msg << "Expected: (" << expr1 << ") <= (" << expr2 << ")\n"
      << "  Actual: " << StrStreamToString(&val1_ss) << " vs "
      << StrStreamToString(&val2_ss);

  return AssertionFailure(msg);
}

// Asserts that val1 is less than, or almost equal to, val2.  Fails
// otherwise.  In particular, it fails if either val1 or val2 is NaN.
AssertionResult FloatLE(const char* expr1, const char* expr2,
                        float val1, float val2) {
  return FloatingPointLE<float>(expr1, expr2, val1, val2);
}

// Asserts that val1 is less than, or almost equal to, val2.  Fails
// otherwise.  In particular, it fails if either val1 or val2 is NaN.
AssertionResult DoubleLE(const char* expr1, const char* expr2,
                         double val1, double val2) {
  return FloatingPointLE<double>(expr1, expr2, val1, val2);
}

// The helper function for {ASSERT|EXPECT}_EQ with int or enum
// arguments.
AssertionResult CmpHelperEQ(const char* expected_expression,
                            const char* actual_expression,
                            int expected,
                            int actual) {
  if (expected == actual) {
    return AssertionSuccess();
  }

  return EqFailure(expected_expression,
                   actual_expression,
                   FormatForComparisonFailureMessage(expected, actual),
                   FormatForComparisonFailureMessage(actual, expected),
                   false);
}


// A macro for implementing the helper functions needed to implement
// ASSERT_?? and EXPECT_?? with int or enum arguments.  It is here
// just to avoid copy-and-paste of similar code.
#define GUNIT_IMPL_CMP_HELPER(op_name, op)\
AssertionResult CmpHelper##op_name(const char* expr1, const char* expr2, \
                                   int val1, int val2) {\
  if (val1 op val2) {\
    return AssertionSuccess();\
  } else {\
    Message msg;\
    msg << "Expected: (" << expr1 << ") " #op " (" << expr2\
        << "), actual: " << FormatForComparisonFailureMessage(val1, val2)\
        << " vs " << FormatForComparisonFailureMessage(val2, val1);\
    return AssertionFailure(msg);\
  }\
}

// Implements the helper function for {ASSERT|EXPECT}_NE with int or
// enum arguments.
GUNIT_IMPL_CMP_HELPER(NE, !=)
// Implements the helper function for {ASSERT|EXPECT}_LE with int or
// enum arguments.
GUNIT_IMPL_CMP_HELPER(LE, <=)
// Implements the helper function for {ASSERT|EXPECT}_LT with int or
// enum arguments.
GUNIT_IMPL_CMP_HELPER(LT, < )
// Implements the helper function for {ASSERT|EXPECT}_GE with int or
// enum arguments.
GUNIT_IMPL_CMP_HELPER(GE, >=)
// Implements the helper function for {ASSERT|EXPECT}_GT with int or
// enum arguments.
GUNIT_IMPL_CMP_HELPER(GT, > )

#undef GUNIT_IMPL_CMP_HELPER


// The helper function for {ASSERT|EXPECT}_STREQ.
AssertionResult CmpHelperSTREQ(const char* expected_expression,
                               const char* actual_expression,
                               const char* expected,
                               const char* actual) {
  if (String::CStringEquals(expected, actual)) {
    return AssertionSuccess();
  }

  return EqFailure(expected_expression,
                   actual_expression,
                   String::ShowCStringQuoted(expected),
                   String::ShowCStringQuoted(actual),
                   false);
}

// The helper function for {ASSERT|EXPECT}_STRCASEEQ.
AssertionResult CmpHelperSTRCASEEQ(const char* expected_expression,
                                   const char* actual_expression,
                                   const char* expected,
                                   const char* actual) {
  if (String::CaseInsensitiveCStringEquals(expected, actual)) {
    return AssertionSuccess();
  }

  return EqFailure(expected_expression,
                   actual_expression,
                   String::ShowCStringQuoted(expected),
                   String::ShowCStringQuoted(actual),
                   true);
}

// The helper function for {ASSERT|EXPECT}_STRNE.
AssertionResult CmpHelperSTRNE(const char* s1_expression,
                               const char* s2_expression,
                               const char* s1,
                               const char* s2) {
  if (!String::CStringEquals(s1, s2)) {
    return AssertionSuccess();
  } else {
    Message msg;
    msg << "Expected: (" << s1_expression << ") != ("
        << s2_expression << "), actual: \""
        << s1 << "\" vs \"" << s2 << "\"";
    return AssertionFailure(msg);
  }
}

// The helper function for {ASSERT|EXPECT}_STRCASENE.
AssertionResult CmpHelperSTRCASENE(const char* s1_expression,
                                   const char* s2_expression,
                                   const char* s1,
                                   const char* s2) {
  if (!String::CaseInsensitiveCStringEquals(s1, s2)) {
    return AssertionSuccess();
  } else {
    Message msg;
    msg << "Expected: (" << s1_expression << ") != ("
        << s2_expression << ") (ignoring case), actual: \""
        << s1 << "\" vs \"" << s2 << "\"";
    return AssertionFailure(msg);
  }
}

namespace {

// Helper functions for implementing IsSubString() and IsNotSubstring().

// This group of overloaded functions return true iff needle is a
// substring of haystack.  NULL is considered a substring of itself
// only.

bool IsSubstringPred(const char* needle, const char* haystack) {
  if (needle == NULL || haystack == NULL)
    return needle == haystack;

  return strstr(haystack, needle) != NULL;
}

bool IsSubstringPred(const wchar_t* needle, const wchar_t* haystack) {
  if (needle == NULL || haystack == NULL)
    return needle == haystack;

  return wcsstr(haystack, needle) != NULL;
}

// StringType here can be either ::std::string or ::std::wstring.
template <typename StringType>
bool IsSubstringPred(const StringType& needle,
                     const StringType& haystack) {
  return haystack.find(needle) != StringType::npos;
}

// This function implements either IsSubstring() or IsNotSubstring(),
// depending on the value of the expected_to_be_substring parameter.
// StringType here can be const char*, const wchar_t*, ::std::string,
// or ::std::wstring.
template <typename StringType>
AssertionResult IsSubstringImpl(
    bool expected_to_be_substring,
    const char* needle_expr, const char* haystack_expr,
    const StringType& needle, const StringType& haystack) {
  if (IsSubstringPred(needle, haystack) == expected_to_be_substring)
    return AssertionSuccess();

  const bool is_wide_string = sizeof(needle[0]) > 1;
  const char* const begin_string_quote = is_wide_string ? "L\"" : "\"";
  return AssertionFailure(
      Message()
      << "Value of: " << needle_expr << "\n"
      << "  Actual: " << begin_string_quote << needle << "\"\n"
      << "Expected: " << (expected_to_be_substring ? "" : "not ")
      << "a substring of " << haystack_expr << "\n"
      << "Which is: " << begin_string_quote << haystack << "\"");
}

}  // namespace

// IsSubstring() and IsNotSubstring() check whether needle is a
// substring of haystack (NULL is considered a substring of itself
// only), and return an appropriate error message when they fail.

AssertionResult IsSubstring(
    const char* needle_expr, const char* haystack_expr,
    const char* needle, const char* haystack) {
  return IsSubstringImpl(true, needle_expr, haystack_expr, needle, haystack);
}

AssertionResult IsSubstring(
    const char* needle_expr, const char* haystack_expr,
    const wchar_t* needle, const wchar_t* haystack) {
  return IsSubstringImpl(true, needle_expr, haystack_expr, needle, haystack);
}

AssertionResult IsNotSubstring(
    const char* needle_expr, const char* haystack_expr,
    const char* needle, const char* haystack) {
  return IsSubstringImpl(false, needle_expr, haystack_expr, needle, haystack);
}

AssertionResult IsNotSubstring(
    const char* needle_expr, const char* haystack_expr,
    const wchar_t* needle, const wchar_t* haystack) {
  return IsSubstringImpl(false, needle_expr, haystack_expr, needle, haystack);
}

#if GUNIT_HAS_STD_STRING
AssertionResult IsSubstring(
    const char* needle_expr, const char* haystack_expr,
    const ::std::string& needle, const ::std::string& haystack) {
  return IsSubstringImpl(true, needle_expr, haystack_expr, needle, haystack);
}

AssertionResult IsSubstring(
    const char* needle_expr, const char* haystack_expr,
    const ::std::wstring& needle, const ::std::wstring& haystack) {
  return IsSubstringImpl(true, needle_expr, haystack_expr, needle, haystack);
}

AssertionResult IsNotSubstring(
    const char* needle_expr, const char* haystack_expr,
    const ::std::string& needle, const ::std::string& haystack) {
  return IsSubstringImpl(false, needle_expr, haystack_expr, needle, haystack);
}

AssertionResult IsNotSubstring(
    const char* needle_expr, const char* haystack_expr,
    const ::std::wstring& needle, const ::std::wstring& haystack) {
  return IsSubstringImpl(false, needle_expr, haystack_expr, needle, haystack);
}
#endif  // GUNIT_HAS_STD_STRING

// Utility functions for encoding Unicode text (wide strings) in
// UTF-8.

// A Unicode code-point can have upto 21 bits, and is encoded in UTF-8
// like this:
//
// Code-point length   Encoding
//   0 -  7 bits       0xxxxxxx
//   8 - 11 bits       110xxxxx 10xxxxxx
//  12 - 16 bits       1110xxxx 10xxxxxx 10xxxxxx
//  17 - 21 bits       11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

// The maximum code-point a one-byte UTF-8 sequence can represent.
const UInt32 kMaxCodePoint1 = (static_cast<UInt32>(1) <<  7) - 1;

// The maximum code-point a two-byte UTF-8 sequence can represent.
const UInt32 kMaxCodePoint2 = (static_cast<UInt32>(1) << (5 + 6)) - 1;

// The maximum code-point a three-byte UTF-8 sequence can represent.
const UInt32 kMaxCodePoint3 = (static_cast<UInt32>(1) << (4 + 2*6)) - 1;

// The maximum code-point a four-byte UTF-8 sequence can represent.
const UInt32 kMaxCodePoint4 = (static_cast<UInt32>(1) << (3 + 3*6)) - 1;

// Chops off the n lowest bits from a bit pattern.  Returns the n
// lowest bits.  As a side effect, the original bit pattern will be
// shifted to the right by n bits.
inline UInt32 ChopLowBits(UInt32* bits, int n) {
  const UInt32 low_bits = *bits & ((static_cast<UInt32>(1) << n) - 1);
  *bits >>= n;
  return low_bits;
}

// Converts a Unicode code-point to its UTF-8 encoding.
String ToUtf8String(wchar_t wchar) {
  char str[5] = {};  // Initializes str to all '\0' characters.

  UInt32 code = static_cast<UInt32>(wchar);
  if (code <= kMaxCodePoint1) {
    str[0] = code;                          // 0xxxxxxx
  } else if (code <= kMaxCodePoint2) {
    str[1] = 0x80 | ChopLowBits(&code, 6);  // 10xxxxxx
    str[0] = 0xC0 | code;                   // 110xxxxx
  } else if (code <= kMaxCodePoint3) {
    str[2] = 0x80 | ChopLowBits(&code, 6);  // 10xxxxxx
    str[1] = 0x80 | ChopLowBits(&code, 6);  // 10xxxxxx
    str[0] = 0xE0 | code;                   // 1110xxxx
  } else if (code <= kMaxCodePoint4) {
    str[3] = 0x80 | ChopLowBits(&code, 6);  // 10xxxxxx
    str[2] = 0x80 | ChopLowBits(&code, 6);  // 10xxxxxx
    str[1] = 0x80 | ChopLowBits(&code, 6);  // 10xxxxxx
    str[0] = 0xF0 | code;                   // 11110xxx
  } else {
    return String::Format("(Invalid Unicode 0x%llX)",
                          static_cast<UInt64>(wchar));
  }

  return String(str);
}

// Converts a wide C string to a String using the UTF-8 encoding.
// NULL will be converted to "(null)".
String String::ShowWideCString(const wchar_t * wide_c_str) {
  if (wide_c_str == NULL) return String("(null)");

  StrStream ss;
  while (*wide_c_str) {
    ss << ToUtf8String(*wide_c_str++);
  }

  return StrStreamToString(&ss);
}

// Similar to ShowWideCString(), except that this function encloses
// the converted string in double quotes.
String String::ShowWideCStringQuoted(const wchar_t* wide_c_str) {
  if (wide_c_str == NULL) return String("(null)");

  return String::Format("L\"%s\"",
                        String::ShowWideCString(wide_c_str).c_str());
}

// Compares two wide C strings.  Returns true iff they have the same
// content.
//
// Unlike wcscmp(), this function can handle NULL argument(s).  A NULL
// C string is considered different to any non-NULL C string,
// including the empty string.
bool String::WideCStringEquals(const wchar_t * lhs, const wchar_t * rhs) {
  if (lhs == NULL) return rhs == NULL;

  if (rhs == NULL) return false;

  return wcscmp(lhs, rhs) == 0;
}

// Helper function for *_STREQ on wide strings.
AssertionResult CmpHelperSTREQ(const char* expected_expression,
                               const char* actual_expression,
                               const wchar_t* expected,
                               const wchar_t* actual) {
  if (String::WideCStringEquals(expected, actual)) {
    return AssertionSuccess();
  }

  return EqFailure(expected_expression,
                   actual_expression,
                   String::ShowWideCStringQuoted(expected),
                   String::ShowWideCStringQuoted(actual),
                   false);
}

// Helper function for *_STRNE on wide strings.
AssertionResult CmpHelperSTRNE(const char* s1_expression,
                               const char* s2_expression,
                               const wchar_t* s1,
                               const wchar_t* s2) {
  if (!String::WideCStringEquals(s1, s2)) {
    return AssertionSuccess();
  }

  Message msg;
  msg << "Expected: (" << s1_expression << ") != ("
      << s2_expression << "), actual: "
      << String::ShowWideCStringQuoted(s1)
      << " vs " << String::ShowWideCStringQuoted(s2);
  return AssertionFailure(msg);
}

// Compares two C strings, ignoring case.  Returns true iff they have
// the same content.
//
// Unlike strcasecmp(), this function can handle NULL argument(s).  A
// NULL C string is considered different to any non-NULL C string,
// including the empty string.
bool String::CaseInsensitiveCStringEquals(const char * lhs, const char * rhs) {
  if ( lhs == NULL ) return rhs == NULL;

  if ( rhs == NULL ) return false;

#ifdef _MSC_VER
  return _stricmp(lhs, rhs) == 0;
#else  // _MSC_VER
  return strcasecmp(lhs, rhs) == 0;
#endif  // _MSC_VER
}

// Constructs a String by copying a given number of chars from a
// buffer.  E.g. String("hello", 3) will create the string "hel".
String::String(const char * buffer, size_t len) {
  char * const temp = new char[ len + 1 ];
  memcpy(temp, buffer, len);
  temp[ len ] = '\0';
  c_str_ = temp;
}

// Compares this with another String.
// Returns < 0 if this is less than rhs, 0 if this is equal to rhs, or > 0
// if this is greater than rhs.
int String::Compare(const String & rhs) const {
  if ( c_str_ == NULL ) {
    return rhs.c_str_ == NULL ? 0 : -1;  // NULL < anything except NULL
  }

  return rhs.c_str_ == NULL ? 1 : strcmp(c_str_, rhs.c_str_);
}

// Returns true iff this String ends with the given suffix.  *Any*
// String is considered to end with a NULL or empty suffix.
bool String::EndsWith(const char* suffix) const {
  if (suffix == NULL || CStringEquals(suffix, "")) return true;

  if (c_str_ == NULL) return false;

  const size_t this_len = strlen(c_str_);
  const size_t suffix_len = strlen(suffix);
  return (this_len >= suffix_len) &&
         CStringEquals(c_str_ + this_len - suffix_len, suffix);
}

// Sets the 0-terminated C string this String object represents.  The
// old string in this object is deleted, and this object will own a
// clone of the input string.  This function copies only up to length
// bytes (plus a terminating null byte), or until the first null byte,
// whichever comes first.
//
// This function works even when the c_str parameter has the same
// value as that of the c_str_ field.
void String::Set(const char * c_str, size_t length) {
  // Makes sure this works when c_str == c_str_
  const char* const temp = CloneString(c_str, length);
  delete[] c_str_;
  c_str_ = temp;
}

// Assigns a C string to this object.  Self-assignment works.
const String& String::operator=(const char* c_str) {
  // Makes sure this works when c_str == c_str_
  if (c_str != c_str_) {
    delete[] c_str_;
    c_str_ = CloneCString(c_str);
  }
  return *this;
}

// Formats a list of arguments to a String, using the same format
// spec string as for printf.
//
// We do not use the StringPrintf class as it is not universally
// available.
//
// The result is limited to 4096 characters (including the tailing 0).
// If 4096 characters are not enough to format the input,
// "<buffer exceeded>" is returned.
String String::Format(const char * format, ...) {
  va_list args;
  va_start(args, format);

  char buffer[4096];
  // MSVC 8 deprecates vsnprintf(), so we want to suppress warning
  // 4996 (deprecated function) there.
#ifdef _MSC_VER  // We are on Windows.
# pragma warning(push)          // Saves the current warning state.
# pragma warning(disable:4996)  // Temporarily disables warning 4996.
  const int size =
    vsnprintf(buffer, sizeof(buffer)/sizeof(buffer[0]) - 1, format, args);
# pragma warning(pop)           // Restores the warning state.
#else  // We are on Linux or Mac OS.
  const int size =
    vsnprintf(buffer, sizeof(buffer)/sizeof(buffer[0]) - 1, format, args);
#endif  // _MSC_VER
  va_end(args);

  return String(size >= 0 ? buffer : "<buffer exceeded>");
}

// Converts the buffer in a StrStream to a String, converting NUL
// bytes to "\\0" along the way.
String StrStreamToString(StrStream* ss) {
#ifdef _MSC_VER
  const char* const start = ss->str();
  const char* const end = start + ss->pcount();
#else
  const std::string& str = ss->str();
  const char* const start = str.c_str();
  const char* const end = start + str.length();
#endif  // _MSC_VER

  // We need to use a helper StrStream to do this transformation
  // because String doesn't support push_back().
  StrStream helper;
  for (const char* ch = start; ch != end; ++ch) {
    if (*ch == '\0') {
      helper << "\\0";  // Replaces NUL with "\\0";
    } else {
      helper.put(*ch);
    }
  }

#ifdef _MSC_VER
  const String str(helper.str(), helper.pcount());
  helper.freeze(false);
  ss->freeze(false);
  return str;
#else
  return String(helper.str().c_str());
#endif  // _MSC_VER
}

// Appends the user-supplied message to the gUnit-generated message.
String AppendUserMessage(const String& gunit_msg,
                         const Message& user_msg) {
  // Appends the user message if it's non-empty.
  const String user_msg_string = user_msg.GetString();
  if (user_msg_string.is_empty()) {
    return gunit_msg;
  }

  Message msg;
  msg << gunit_msg << "\n" << user_msg_string;

  return msg.GetString();
}


// class TestResult

// Creates an empty TestResult.
TestResult::TestResult() : test_part_results_(), elapsed_time_(0) {
}

// D'tor.
TestResult::~TestResult() {
}

// Adds a test part result to the list.
void TestResult::AddTestPartResult(const TestPartResult& test_part_result) {
  test_part_results_.PushBack(test_part_result);
}

// Clears the object.
void TestResult::Clear() {
  test_part_results_.Clear();
}

// Returns true iff the test part passed.
static bool TestPartPassed(const TestPartResult & result) {
  return result.Passed();
}

// Gets the number of successful test parts.
int TestResult::successful_part_count() const {
  return test_part_results_.CountIf(TestPartPassed);
}

// Returns true iff the test part failed.
static bool TestPartFailed(const TestPartResult & result) {
  return result.Failed();
}

// Gets the number of failed test parts.
int TestResult::failed_part_count() const {
  return test_part_results_.CountIf(TestPartFailed);
}

// Returns true iff the test part fatally failed.
static bool TestPartFatallyFailed(const TestPartResult & result) {
  return result.FatallyFailed();
}

// Returns true iff the test fatally failed.
bool TestResult::HasFatalFailure() const {
  return test_part_results_.CountIf(TestPartFatallyFailed) > 0;
}

// Gets the number of all test parts.  This is the sum of the number
// of successful test parts and the number of failed test parts.
int TestResult::total_part_count() const {
  return test_part_results_.size();
}


// class Test

// Creates a Test object.

// The c'tor saves the values of all gUnit flags.
Test::Test()
    : gunit_flag_saver_(new GUnitFlagSaver) {
}

// The d'tor restores the values of all gUnit flags.
Test::~Test() {
  delete gunit_flag_saver_;
}

// Sets up the test fixture.
//
// A sub-class may override this.
void Test::SetUp() {
}

// Tears down the test fixture.
//
// A sub-class may override this.
void Test::TearDown() {
}

#ifdef _MSC_VER  // We are on Windows.

// Adds an "exception thrown" fatal failure to the current test.
static void AddExceptionThrownFailure(DWORD exception_code,
                                      const char* location) {
  Message message;
  message << "Exception thrown with code 0x" << std::setbase(16) <<
    exception_code << std::setbase(10) << " in " << location << ".";

  UnitTest* const unit_test = UnitTest::GetInstance();
  unit_test->AddTestPartResult(
      TPRT_FATAL_FAILURE,
      String(static_cast<const char *>(NULL)),
           // We have no info about the source file where the exception
           // occurred.
      -1,  // We have no info on which line caused the exception.
      message.GetString(),
      String(""));
}

#endif  // _MSC_VER

// gUnit requires all tests in the same test case to use the same test
// fixture class.  This function checks if the current test has the
// same fixture class as the first test in the current test case.  If
// yes, it returns true; otherwise it generates a gUnit failure and
// returns false.
static bool HasSameFixtureClass() {
  UnitTestImpl* const impl = GetUnitTestImpl();
  const TestCase* const test_case = impl->current_test_case();

  // Info about the first test in the current test case.
  const TestInfoImpl* const first_test_info =
      test_case->test_info_list().Head()->element()->impl();
  const TypeId first_fixture_id = first_test_info->fixture_class_id();
  const char* const first_test_name = first_test_info->name();

  // Info about the current test.
  const TestInfoImpl* const this_test_info = impl->current_test_info()->impl();
  const TypeId this_fixture_id = this_test_info->fixture_class_id();
  const char* const this_test_name = this_test_info->name();

  if (this_fixture_id != first_fixture_id) {
    // Is the first test defined using TEST?
    const bool first_is_TEST = first_fixture_id == GetTypeId<Test>();
    // Is this test defined using TEST?
    const bool this_is_TEST = this_fixture_id == GetTypeId<Test>();

    if (first_is_TEST || this_is_TEST) {
      // The user mixed TEST and TEST_F in this test case - we'll tell
      // him/her how to fix it.

      // Gets the name of the TEST and the name of the TEST_F.  Note
      // that first_is_TEST and this_is_TEST cannot both be true, as
      // the fixture IDs are different for the two tests.
      const char* const TEST_name =
          first_is_TEST ? first_test_name : this_test_name;
      const char* const TEST_F_name =
          first_is_TEST ? this_test_name : first_test_name;

      ADD_FAILURE()
          << "All tests in the same test case must use the same test fixture\n"
          << "class, so mixing TEST_F and TEST in the same test case is\n"
          << "illegal.  In test case " << this_test_info->test_case_name()
          << ",\n"
          << "test " << TEST_F_name << " is defined using TEST_F but\n"
          << "test " << TEST_name << " is defined using TEST.  You probably\n"
          << "want to change the TEST to TEST_F or move it to another test\n"
          << "case.";
    } else {
      // The user defined two fixture classes with the same name in
      // two namespaces - we'll tell him/her how to fix it.
      ADD_FAILURE()
          << "All tests in the same test case must use the same test fixture\n"
          << "class.  However, in test case "
          << this_test_info->test_case_name() << ",\n"
          << "you defined test " << first_test_name
          << " and test " << this_test_name << "\n"
          << "using two different test fixture classes.  This can happen if\n"
          << "the two classes are from different namespaces or translation\n"
          << "units and have the same name.  You should probably rename one\n"
          << "of the classes to put the tests into different test cases.";
    }
    return false;
  }

  return true;
}

// Runs the test and updates the test result.
void Test::Run() {
  if (!HasSameFixtureClass()) return;

  UnitTestImpl* const impl = GetUnitTestImpl();
#ifdef _MSC_VER  // We are on Windows.
  impl->os_stack_trace_getter()->UponLeavingGUnit();
  __try {
    SetUp();
  } __except(UnitTestOptions::GUnitShouldProcessSEH(GetExceptionCode())) {
    AddExceptionThrownFailure(GetExceptionCode(), "SetUp()");
  }

  // We will run the test only if SetUp() had no fatal failure.
  if (!HasFatalFailure()) {
    impl->os_stack_trace_getter()->UponLeavingGUnit();
    __try {
      TestBody();
    } __except(UnitTestOptions::GUnitShouldProcessSEH(GetExceptionCode())) {
      AddExceptionThrownFailure(GetExceptionCode(), "the test body");
    }
  }

  // However, we want to clean up as much as possible.  Hence we will
  // always call TearDown(), even if SetUp() or the test body has
  // failed.
  impl->os_stack_trace_getter()->UponLeavingGUnit();
  __try {
    TearDown();
  } __except(UnitTestOptions::GUnitShouldProcessSEH(GetExceptionCode())) {
    AddExceptionThrownFailure(GetExceptionCode(), "TearDown()");
  }

#else  // We are on Linux or Mac - exceptions are disabled.
  impl->os_stack_trace_getter()->UponLeavingGUnit();
  SetUp();

  // We will run the test only if SetUp() was successful.
  if (!HasFatalFailure()) {
    impl->os_stack_trace_getter()->UponLeavingGUnit();
    TestBody();
  }

  // However, we want to clean up as much as possible.  Hence we will
  // always call TearDown(), even if SetUp() or the test body has
  // failed.
  impl->os_stack_trace_getter()->UponLeavingGUnit();
  TearDown();
#endif  // _MSC_VER
}


// Returns true iff the current test has a fatal failure.
bool Test::HasFatalFailure() {
  return GetUnitTestImpl()->current_test_result()->HasFatalFailure();
}


// class TestInfo

// Constructs a TestInfo object.
TestInfo::TestInfo(const char* test_case_name,
                   const char* name,
                   TypeId fixture_class_id,
                   TestMaker maker) {
  impl_ = new TestInfoImpl(this, test_case_name, name,
                           fixture_class_id, maker);
}

// Destructs a TestInfo object.
TestInfo::~TestInfo() {
  delete impl_;
}

// Creates a TestInfo object and registers it with the UnitTest
// singleton; returns the created object.
//
// Arguments:
//
//   test_case_name: name of the test case
//   name:           name of the test
//   set_up_tc:      pointer to the function that sets up the test case
//   tear_down_tc:   pointer to the function that tears down the test case
//   maker:          pointer to the function that creates a test object
TestInfo* TestInfo::MakeAndRegisterInstance(
    const char* test_case_name,
    const char* name,
    TypeId fixture_class_id,
    Test::SetUpTestCaseFunc set_up_tc,
    Test::TearDownTestCaseFunc tear_down_tc,
    TestMaker maker) {
  TestInfo* const test_info =
      new TestInfo(test_case_name, name, fixture_class_id, maker);
  GetUnitTestImpl()->AddTestInfo(set_up_tc, tear_down_tc, test_info);
  return test_info;
}

// Returns the test case name.
const char* TestInfo::test_case_name() const {
  return impl_->test_case_name();
}

// Returns the test name.
const char* TestInfo::name() const {
  return impl_->name();
}


namespace {

// A predicate that checks the test name of a TestInfo against a known
// value.
//
// This is used for implementation of the TestCase class only.  We put
// it in the anonymous namespace to prevent polluting the outer
// namespace.
//
// TestNameIs is copyable.
class TestNameIs {
 private:
  String name_;

 public:
  // Constructor.
  //
  // TestNameIs has NO default constructor.
  explicit TestNameIs(const String & name)
      : name_(name) {}

  // Returns true iff the test name of test_info matches name_.
  bool operator()(const TestInfo * test_info) const {
    return test_info && String(test_info->name()).Compare(name_) == 0;
  }
};

} // namespace

// Finds and returns a TestInfo with the given name.  If one doesn't
// exist, returns NULL.
TestInfo * TestCase::GetTestInfo(const String & test_name) {
  // Can we find a TestInfo with the given name?
  ListNode<TestInfo *> * const node = test_info_list_->FindIf(
      TestNameIs(test_name));

  // Returns the TestInfo found.
  return node ? node->element() : NULL;
}


// Creates the test object, runs it, records its result, and then
// deletes it.
void TestInfoImpl::Run() {
  if (!should_run_) return;

  // Tells UnitTest where to store test result.
  UnitTestImpl* const impl = GetUnitTestImpl();
  impl->set_current_test_info(parent_);

  // Notifies the unit test event listener that a test is about to
  // start.
  UnitTestEventListenerInterface* const result_printer =
    impl->result_printer();
  result_printer->OnTestStart(parent_);

  const TimeInMillis start = GetTimeInMillis();

  impl->os_stack_trace_getter()->UponLeavingGUnit();
#ifdef _MSC_VER  // We are on Windows.
  Test* test = NULL;

  __try {
    // Creates the test object.
    test = (*maker_)();
  } __except(UnitTestOptions::GUnitShouldProcessSEH(GetExceptionCode())) {
    AddExceptionThrownFailure(GetExceptionCode(),
                              "the test fixture's constructor");
    return;
  }
#else  // We are on Linux or Mac OS - exceptions are disabled.

  // TODO(wan): If test->Run() throws, test won't be deleted.  This is
  // not a problem now as we don't use exceptions.  If we were to
  // enable exceptions, we should revise the following to be
  // exception-safe.

  // Creates the test object.
  Test* test = (*maker_)();
#endif  // _MSC_VER

  // Runs the test only if the constructor of the test fixture didn't
  // generate a fatal failure.
  if (!Test::HasFatalFailure()) {
    test->Run();
  }

  // Deletes the test object.
  impl->os_stack_trace_getter()->UponLeavingGUnit();
  delete test;
  test = NULL;

  result_.set_elapsed_time(GetTimeInMillis() - start);

  // Notifies the unit test event listener that a test has just finished.
  result_printer->OnTestEnd(parent_);

  // Tells UnitTest to stop associating assertion results to this
  // test.
  impl->set_current_test_info(NULL);
}


// class TestCase

// Gets the number of successful tests in this test case.
int TestCase::successful_test_count() const {
  return test_info_list_->CountIf(TestPassed);
}

// Gets the number of failed tests in this test case.
int TestCase::failed_test_count() const {
  return test_info_list_->CountIf(TestFailed);
}

int TestCase::disabled_test_count() const {
  return test_info_list_->CountIf(TestDisabled);
}

// Get the number of tests in this test case that should run.
int TestCase::test_to_run_count() const {
  return test_info_list_->CountIf(ShouldRunTest);
}

// Gets the number of all tests.
int TestCase::total_test_count() const {
  return test_info_list_->size();
}

// Creates a TestCase with the given name.
//
// Arguments:
//
//   name:         name of the test case
//   set_up_tc:    pointer to the function that sets up the test case
//   tear_down_tc: pointer to the function that tears down the test case
TestCase::TestCase(const String& name,
                   Test::SetUpTestCaseFunc set_up_tc,
                   Test::TearDownTestCaseFunc tear_down_tc)
    : name_(name),
      set_up_tc_(set_up_tc),
      tear_down_tc_(tear_down_tc),
      should_run_(false),
      elapsed_time_(0) {
  test_info_list_ = new List<TestInfo *>;
}

// Destructor of TestCase.
TestCase::~TestCase() {
  // Deletes every Test in the collection.
  test_info_list_->ForEach(Delete<TestInfo>);

  // Then deletes the Test collection.
  delete test_info_list_;
  test_info_list_ = NULL;
}

// Adds a test to this test case.  Will delete the test upon
// destruction of the TestCase object.
void TestCase::AddTestInfo(TestInfo * test_info) {
  test_info_list_->PushBack(test_info);
}

// Runs every test in this TestCase.
void TestCase::Run() {
  if (!should_run_) return;

  UnitTestImpl* const impl = GetUnitTestImpl();
  impl->set_current_test_case(this);

  UnitTestEventListenerInterface * const result_printer =
      impl->result_printer();

  result_printer->OnTestCaseStart(this);
  impl->os_stack_trace_getter()->UponLeavingGUnit();
  set_up_tc_();

  const TimeInMillis start = GetTimeInMillis();
  test_info_list_->ForEach(TestInfoImpl::RunTest);
  elapsed_time_ = GetTimeInMillis() - start;

  impl->os_stack_trace_getter()->UponLeavingGUnit();
  tear_down_tc_();
  result_printer->OnTestCaseEnd(this);
  impl->set_current_test_case(NULL);
}

// Clears the results of all tests in this test case.
void TestCase::ClearResult() {
  test_info_list_->ForEach(TestInfoImpl::ClearTestResult);
}


// class UnitTestEventListenerInterface

// The virtual d'tor.
UnitTestEventListenerInterface::~UnitTestEventListenerInterface() {
}

// A result printer that never prints anything.  Used in the child process
// of an exec-style death test to avoid needless output clutter.
class NullUnitTestResultPrinter : public UnitTestEventListenerInterface { };


// class PlainTextUnitTestResultPrinter

// This class implements the UnitTestEventListenerInterface interface,
// and is used as the default printer for test results.
//
// Class PlainTextUnitTestResultPrinter is copyable.
class PlainTextUnitTestResultPrinter : public UnitTestEventListenerInterface {
 protected:

  // Prints a TestPartResult.
  static void PrintTestPartResult(const TestPartResult & test_part_result);

  // Formats a countable noun.  Depending on its quantity, either the
  // singular form or the plural form is used. e.g.
  //
  // FormatCountableNoun(1, "formula", "formuli") returns "1 formula".
  // FormatCountableNoun(5, "book", "books") returns "5 books".
  static String FormatCountableNoun(int count,
                                    const char * singular_form,
                                    const char * plural_form) {
    return String::Format("%d %s", count,
                          count == 1 ? singular_form : plural_form);
  }

  // Formats the count of tests.
  static String FormatTestCount(int test_count) {
    return FormatCountableNoun(test_count, "test", "tests");
  }

  // Formats the count of test cases.
  static String FormatTestCaseCount(int test_case_count) {
    return FormatCountableNoun(test_case_count, "test case", "test cases");
  }

  // Formats the result of a test case or test.
  // T can be either TestCase or TestResult.
  template <typename T>
  static const char * FormatResultSummary(const T& result);

 public:
  // Creates a new PlainTextUnitTestResultPrinter object.
  PlainTextUnitTestResultPrinter()
      : test_running_(false) {
  }

  // The following methods override what's in the
  // UnitTestEventListenerInterface class.

  // Called before the unit test starts.
  virtual void OnUnitTestStart(const UnitTest * unit_test);

  // Called after the unit test ends.
  virtual void OnUnitTestEnd(const UnitTest * unit_test);

  // Called before the test case starts.
  virtual void OnTestCaseStart(const TestCase * test_case);

  // Called after the test case ends.
  virtual void OnTestCaseEnd(const TestCase * test_case);

  // Called before the test starts.
  virtual void OnTestStart(const TestInfo * test_info);

  // Called after the test ends.
  virtual void OnTestEnd(const TestInfo * test_info);

  // Called after an assertion.
  virtual void OnNewTestPartResult(const TestPartResult * result);

 private:
  bool test_running_;         // Is a test running?
};

// Converts a TestPartResultType enum to human-friendly string
// representation.  Both TPRT_NONFATAL_FAILURE and TPRT_FATAL_FAILURE
// are translated to "Failure", as the user usually doesn't care about
// the difference between the two when viewing the test result.
static const char * TestPartResultTypeToString(TestPartResultType type) {
  switch (type) {
    case TPRT_SUCCESS:
      return "Success";

    case TPRT_NONFATAL_FAILURE:
    case TPRT_FATAL_FAILURE:
      return "Failure";
  }

  return "Unknown result type";
}

// Prints a TestPartResult.
void PlainTextUnitTestResultPrinter::PrintTestPartResult(
    const TestPartResult & test_part_result) {
  const char * const file_name =
    test_part_result.file_name().c_str();

  printf("%s", file_name == NULL ? "unknown file" : file_name);
  if (test_part_result.line_number() >= 0) {
    printf(":%d", test_part_result.line_number());
  }
  printf(": %s\n", TestPartResultTypeToString(test_part_result.type()));
  printf("%s\n", test_part_result.message().c_str());
  fflush(stdout);
}

// Formats the result of a test case or test.
// T can be either TestCase or TestResult.
template <typename T>
const char * PlainTextUnitTestResultPrinter::FormatResultSummary(
    const T& result) {
  if (result.Failed()) {
    return "failed";
  }

  return "passed";
}

// Called before the unit test starts.
void PlainTextUnitTestResultPrinter::OnUnitTestStart(
    const UnitTest * unit_test) {
  const char * const filter = FLAGS_gunit_filter.c_str();

  // Prints the filter if it's not *.  This reminds the user that some
  // tests may be skipped.
  if (!String::CStringEquals(filter, kUniversalFilter)) {
    printf("gUnit filter = %s\n", filter);
  }

  const UnitTestImpl* const impl = unit_test->impl();
  printf("\nRunning %s from %s . . .\n",
         FormatTestCount(impl->test_to_run_count()).c_str(),
         FormatTestCaseCount(impl->test_case_to_run_count()).c_str());
}

String Repeat(int n, char ch) {
  Message msg;
  for (int i = 0; i < n; i++) {
    msg << ch;
  }
  return msg.GetString();
}

// Returns a fancy message to catch user's attention. e.g.:
//
// "##########################################"
// "#                                        #"
// "#     YOU HAVE 5 DISABLED TESTS!!!       #"
// "#                                        #"
// "##########################################"
//
// Extra work that we do here is to match ending '#' in all lines.
// Message is centered in the box with padding_length '#" inserted before
// and after.
// Empty string is returned if disabled_count is not positive.
String DisabledTestBanner(int disabled_count, int padding_length) {
  if (disabled_count <= 0) return String("");

  const String fail_message =
    String::Format("YOU HAVE %d DISABLED %s!!!",
                   disabled_count,
                   disabled_count == 1 ? "TEST" : "TESTS");
  // Number of '#' to print before and after to make message centered.
  const int line_length = static_cast<int>(strlen(fail_message.c_str())) +
      2*padding_length;
  Message banner;
  banner << Repeat(line_length + 2, '#') << '\n'
         << '#' << Repeat(line_length, ' ') << "#\n"
         << '#' << Repeat(padding_length, ' ') << fail_message
         << Repeat(padding_length, ' ') << "#\n"
         << '#' << Repeat(line_length, ' ') << "#\n"
         << Repeat(line_length + 2, '#') << '\n';
  return banner.GetString();
}

// Called after the unit test ends.
void PlainTextUnitTestResultPrinter::OnUnitTestEnd(
    const UnitTest * unit_test) {
  // Prints the summary for the entire unit test.
  const UnitTestImpl* const impl = unit_test->impl();

  printf("\nSUMMARY\n\n");

  printf("%s from %s ran.\n",
         FormatTestCount(impl->test_to_run_count()).c_str(),
         FormatTestCaseCount(impl->test_case_to_run_count()).c_str());
  printf("%d passed.\n", impl->successful_test_count());
  printf("%d failed.\n", impl->failed_test_count());
  printf("%s",
         DisabledTestBanner(impl->disabled_test_count(), 8).c_str());

  const TestResult * const ad_hoc_result = impl->ad_hoc_test_result();
  if (!ad_hoc_result->Passed()) {
    printf("The non-test part of the code %s.\n",
           FormatResultSummary(*ad_hoc_result));
  }

  printf("\n%s\n", impl->Passed() ? "PASS" : "FAIL");

  // Ensure that gUnit output is printed before, e.g., heapchecker output.
  fflush(stdout);
}

// Called before the test case starts.
void PlainTextUnitTestResultPrinter::OnTestCaseStart(
    const TestCase * test_case) {
  printf("\nRunning %s from test case %s . . .\n",
         FormatTestCount(test_case->test_to_run_count()).c_str(),
         test_case->name().c_str());
}

// Called after the test case ends.
void PlainTextUnitTestResultPrinter::OnTestCaseEnd(
    const TestCase * test_case) {
  printf("Test case %s %s.\n",
         test_case->name().c_str(),
         FormatResultSummary(*test_case));
}

// Called before the test starts.
void PlainTextUnitTestResultPrinter::OnTestStart(const TestInfo * test_info) {
  // Initializes the test status.
  test_running_ = true;

  // Prints the test name s.t. if something goes wrong, the user can
  // tell which test it is in.
  printf("  Running test %s . . .\n", test_info->name());
}

// Called after the test ends.
void PlainTextUnitTestResultPrinter::OnTestEnd(const TestInfo * test_info) {
  // Prints the test result.
  printf("  Test %s %s.\n", test_info->name(),
         test_info->impl()->result()->Passed() ? "passed" : "failed");

  // Resets the test status.
  test_running_ = false;
}

// Called after an assertion.
void PlainTextUnitTestResultPrinter::OnNewTestPartResult(
    const TestPartResult * result) {
  // If the test part succeeded, we don't need to do anything.
  if (result->type() == TPRT_SUCCESS) return;

  // Prints the failure message.
  PrintTestPartResult(*result);
  printf("\n");
}


// This class generates an XML output file in addition to the usual
// plain-text output.
class XmlUnitTestResultPrinter : public UnitTestEventListenerInterface {
 public:
  explicit XmlUnitTestResultPrinter(const char* output_file);

  virtual void OnUnitTestStart(const UnitTest* unit_test);
  virtual void OnUnitTestEnd(const UnitTest* unit_test);
  virtual void OnTestCaseStart(const TestCase* test_case);
  virtual void OnTestCaseEnd(const TestCase* test_case);
  virtual void OnTestStart(const TestInfo* test_info);
  virtual void OnTestEnd(const TestInfo* test_info);
  virtual void OnNewTestPartResult(const TestPartResult* result);

 private:
  // Is c a whitespace character that is normalized to a space character
  // when it appears in an XML attribute value?
  static bool IsNormalizableWhitespace(char c) {
    return c == 0x9 || c == 0xA || c == 0xD;
  }

  // May c appear in a well-formed XML document?
  static bool IsValidXmlCharacter(char c) {
    return IsNormalizableWhitespace(c) || c >= 0x20;
  }

  // Returns an XML-escaped copy of the input string str.  If
  // is_attribute is true, the text is meant to appear as an attribute
  // value, and normalizable whitespace is preserved by replacing it
  // with character references.
  static String EscapeXml(const String &str, bool is_attribute);

  // Convenience wrapper around EscapeXml when str is an attribute value.
  static String EscapeXmlAttribute(const String& str) {
    return EscapeXml(str, true);
  }

  // Convenience wrapper around EscapeXml when str is not an attribute value.
  static String EscapeXmlText(const String& str) {
    return EscapeXml(str, false);
  }

  // Prints an XML representation of a TestInfo object.
  static void PrintXmlTestInfo(FILE* out,
                               const String& test_case_name,
                               const TestInfo* test_info);

  // Prints an XML representation of a TestCase object
  static void PrintXmlTestCase(FILE* out, const TestCase* test_case);

  // Prints an XML summary of unit_test to output stream out.
  static void PrintXmlUnitTest(FILE* out, const UnitTest* unit_test);

  // The output file.
  const String output_file_;
  // The plain-printer this class forwards all events to.
  PlainTextUnitTestResultPrinter plain_printer_;
};

// Creates a new XmlUnitTestResultPrinter.
XmlUnitTestResultPrinter::XmlUnitTestResultPrinter(const char* output_file)
    : output_file_(output_file) {
  if (output_file_.c_str() == NULL || output_file_.is_empty()) {
    fprintf(stderr, "XML output file may not be null\n");
    exit(EXIT_FAILURE);
  }
}

// Called before the unit test starts.
void XmlUnitTestResultPrinter::OnUnitTestStart(const UnitTest* unit_test) {
  plain_printer_.OnUnitTestStart(unit_test);
}

// Called after the unit test ends.
void XmlUnitTestResultPrinter::OnUnitTestEnd(const UnitTest* unit_test) {
  plain_printer_.OnUnitTestEnd(unit_test);

  // MSVC 8 deprecates fopen(), so we want to suppress warning 4996
  // (deprecated function) there.
#ifdef _MSC_VER  // We are on Windows.
# pragma warning(push)          // Saves the current warning state.
# pragma warning(disable:4996)  // Temporarily disables warning 4996.
  FILE* const xmlout = fopen(output_file_.c_str(), "w");
# pragma warning(pop)           // Restores the warning state.
#else  // We are on Linux or Mac OS.
  FILE* const xmlout = fopen(output_file_.c_str(), "w");
#endif  // _MSC_VER
  if (xmlout == NULL) {
    // TODO: report the reason of the failure.
    //
    // We don't do it for now as:
    //
    //   1. There is no urgent need for it.
    //   2. It's a bit involved to make the errno variable thread-safe on
    //      all three operating systems (Linux, Windows, and Mac OS).
    //   3. To interpret the meaning of errno in a thread-safe way,
    //      we need the strerror_r() function, which is not available on
    //      Windows.
    fprintf(stderr,
            "Unable to open file \"%s\"\n",
            output_file_.c_str());
    exit(EXIT_FAILURE);
  }
  PrintXmlUnitTest(xmlout, unit_test);
  fclose(xmlout);
}

// Called before the test case starts.
void XmlUnitTestResultPrinter::OnTestCaseStart(const TestCase* test_case) {
  plain_printer_.OnTestCaseStart(test_case);
}

// Called after the test case ends.
void XmlUnitTestResultPrinter::OnTestCaseEnd(const TestCase* test_case) {
  plain_printer_.OnTestCaseEnd(test_case);
}

// Called before the test starts.
void XmlUnitTestResultPrinter::OnTestStart(const TestInfo* test_info) {
  plain_printer_.OnTestStart(test_info);
}

// Called after the test ends.
void XmlUnitTestResultPrinter::OnTestEnd(const TestInfo* test_info) {
  plain_printer_.OnTestEnd(test_info);
}

// Called after an assertion.
void XmlUnitTestResultPrinter::OnNewTestPartResult(
    const TestPartResult* result) {
  plain_printer_.OnNewTestPartResult(result);
}

// Returns an XML-escaped copy of the input string str.  If is_attribute
// is true, the text is meant to appear as an attribute value, and
// normalizable whitespace is preserved by replacing it with character
// references.
//
// Invalid XML characters in str, if any, are stripped from the output.
// It is expected that most, if not all, of the text processed by this
// module will consist of ordinary English text.
// If this module is ever modified to produce version 1.1 XML output,
// most invalid characters can be retained using character references.
// TODO: It might be nice to have a minimally invasive, human-readable
// escaping scheme for invalid characters, rather than dropping them.
String XmlUnitTestResultPrinter::EscapeXml(const String &str,
                                           bool is_attribute) {
  Message m;

  if (str.c_str() != NULL) {
    for (const char* src = str.c_str(); *src; ++src) {
      switch (*src) {
        case '<':
          m << "&lt;";
          break;
        case '>':
          m << "&gt;";
          break;
        case '&':
          m << "&amp;";
          break;
        case '\'':
          if (is_attribute)
            m << "&apos;";
          else
            m << '\'';
          break;
        case '"':
          if (is_attribute)
            m << "&quot;";
          else
            m << '"';
          break;
        default:
          if (IsValidXmlCharacter(*src)) {
            if (is_attribute && IsNormalizableWhitespace(*src))
              m << String::Format("&#x%02X;", unsigned(*src));
            else
              m << *src;
          }
          break;
      }
    }
  }

  return m.GetString();
}


// The following routines generate an XML representation of a UnitTest
// object.
// Prints an XML representation of a TestInfo object.
void XmlUnitTestResultPrinter::PrintXmlTestInfo(FILE* out,
                                                const String& test_case_name,
                                                const TestInfo* test_info) {
  const TestInfoImpl* const impl = test_info->impl();
  const TestResult * const result = impl->result();
  const List<TestPartResult> &results = result->test_part_results();
  fprintf(out,
          "    <testcase name=\"%s\" status=\"%s\" time=\"%s\" "
          "classname=\"%s\"",
          EscapeXmlAttribute(String(test_info->name())).c_str(),
          impl->should_run() ? "run" : "notrun",
          StreamableToString(result->elapsed_time()).c_str(),
          EscapeXmlAttribute(test_case_name).c_str());

  int failures = 0;
  for (const ListNode<TestPartResult>* part_node = results.Head();
       part_node != NULL;
       part_node = part_node->next()) {
    const TestPartResult& part = part_node->element();
    if (part.Failed()) {
      const String message = String::Format("%s:%d\n%s",
                                            part.file_name().c_str(),
                                            part.line_number(),
                                            part.message().c_str());
      if (++failures == 1)
        fprintf(out, ">\n");
      fprintf(out,
              "      <failure message=\"%s\" type=\"\"/>\n",
              EscapeXmlAttribute(message).c_str());
    }
  }

  if (failures == 0)
    fprintf(out, " />\n");
  else
    fprintf(out, "    </testcase>\n");
}

// Prints an XML representation of a TestCase object
void XmlUnitTestResultPrinter::PrintXmlTestCase(FILE* out,
                                                const TestCase* test_case) {
  fprintf(out,
          "  <testsuite name=\"%s\" tests=\"%d\" failures=\"%d\" "
          "disabled=\"%d\" ",
          EscapeXmlAttribute(test_case->name()).c_str(),
          test_case->total_test_count(),
          test_case->failed_test_count(),
          test_case->disabled_test_count());
  fprintf(out,
          "errors=\"0\" time=\"%s\">\n",
          StreamableToString(test_case->elapsed_time()).c_str());
  for (const ListNode<TestInfo*>* info_node =
         test_case->test_info_list().Head();
       info_node != NULL;
       info_node = info_node->next()) {
    PrintXmlTestInfo(out, test_case->name(), info_node->element());
  }
  fprintf(out, "  </testsuite>\n");
}

// Prints an XML summary of unit_test to output stream out.
void XmlUnitTestResultPrinter::PrintXmlUnitTest(FILE* out,
                                                const UnitTest* unit_test) {
  const UnitTestImpl* const impl = unit_test->impl();
  fprintf(out, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  fprintf(out,
          "<testsuite tests=\"%d\" failures=\"%d\" disabled=\"%d\" "
          "errors=\"0\" time=\"%s\" ",
          impl->total_test_count(),
          impl->failed_test_count(),
          impl->disabled_test_count(),
          StreamableToString(impl->elapsed_time()).c_str());
  fprintf(out, "name=\"\">\n");
  for (const ListNode<TestCase*>* case_node = impl->test_cases()->Head();
       case_node != NULL;
       case_node = case_node->next()) {
    PrintXmlTestCase(out, case_node->element());
  }
  fprintf(out, "</testsuite>\n");
}


// Information about a trace point.
struct TraceInfo {
  const char* file;
  int line;
  String message;
};


// Class ScopedTrace

// Pushes the given source file location and message onto a trace
// stack maintained by gUnit.
ScopedTrace::ScopedTrace(const char* file, int line, const Message& message) {
  TraceInfo trace;
  trace.file = file;
  trace.line = line;
  trace.message = message.GetString();

  GetUnitTestImpl()->gunit_trace_stack()->PushFront(trace);
}

// Pops the info pushed by the c'tor.
ScopedTrace::~ScopedTrace() {
  GetUnitTestImpl()->gunit_trace_stack()->PopFront(NULL);
}


// class OsStackTraceGetter

// Returns the current OS stack trace as a String.  Parameters:
//
//   max_depth  - the maximum number of stack frames to be included
//                in the trace.
//   skip_count - the number of top frames to be skipped; doesn't count
//                against max_depth.
//
// L < mutex_
String OsStackTraceGetter::CurrentStackTrace(int max_depth, int skip_count) {
#ifdef GUNIT_LINUX_GOOGLE3_MODE
  if (max_depth == 0) {
    return String("");
  }

  if (max_depth < 0 || max_depth > kMaxStackTraceDepth) {
    max_depth = kMaxStackTraceDepth;
  }

  std::vector<string> stack;
  // Skips whatever number of frames the caller requested, plus this
  // function and GetSymbolizedStackTrace().
  util::GetSymbolizedStackTrace(&stack, max_depth, skip_count + 2);

  scoped_array<void*> raw_stack(new void*[max_depth]);
  // Skips the frames requested by the caller, plus this function.
  const int raw_stack_size = GetStackTrace(raw_stack.get(),
                                           max_depth,
                                           skip_count + 1);

  mutex_.Lock();
  void* const caller_frame = caller_frame_;
  mutex_.Unlock();

  string result;
  for (int i = 0; i < stack.size() && i < raw_stack_size; i++) {
    if (raw_stack[i] == caller_frame &&
        !FLAGS_gunit_show_internal_stack_frames) {
      // Add a marker to the trace and stop adding frames.
      result += OsStackTraceGetter::kElidedFramesMarker;
      result += "\n";
      break;
    }

    result += stack[i] + "\n";
  }

  return String(result.c_str());
#else
  return String("");
#endif  // GUNIT_LINUX_GOOGLE3_MODE
}

// L < mutex_
void OsStackTraceGetter::UponLeavingGUnit() {
#ifdef GUNIT_LINUX_GOOGLE3_MODE
  // Skips UponLeavingGUnit, GetSymbolizedStackTrace, and the calling
  // frame, since only the address below the caller stays constant.
  void* caller_frame;
  if (GetStackTrace(&caller_frame, 1, 3) <= 0)
    caller_frame = NULL;

  MutexLock l(&mutex_);
  caller_frame_ = caller_frame;
#endif  // GUNIT_LINUX_GOOGLE3_MODE
}

const char* const
OsStackTraceGetter::kElidedFramesMarker = "... gUnit internal frames ...";


// class UnitTest

// Gets the singleton UnitTest object.  The first time this method is
// called, a UnitTest object is constructed and returned.  Consecutive
// calls will return the same object.
UnitTest * UnitTest::GetInstance() {
  static UnitTest * instance = new UnitTest();
  return instance;
}

// Adds a TestPartResult to the test result.
void UnitTest::AddTestPartResult(TestPartResultType result_type,
                                 const String& file_name,
                                 int line_number,
                                 const String& message,
                                 const String& os_stack_trace) {
  Message msg;
  msg << message;

  if (impl_->gunit_trace_stack()->size() > 0) {
    msg << "\ngUnit trace:";

    for (ListNode<TraceInfo>* node = impl_->gunit_trace_stack()->Head();
         node != NULL;
         node = node->next()) {
      const TraceInfo& trace = node->element();
      msg << "\n" << trace.file << ":" << trace.line << ": " << trace.message;
    }
  }

  if (os_stack_trace.c_str() != NULL && !os_stack_trace.is_empty()) {
    msg << "\nStack trace:\n" << os_stack_trace;
  }

  const TestPartResult result =
    TestPartResult(result_type, file_name, line_number, msg.GetString());
  impl_->current_test_result()->AddTestPartResult(result);
  impl_->result_printer()->OnNewTestPartResult(&result);

  // If this is a failure and the user wants the debugger to break on
  // failures ...
  if (result_type != TPRT_SUCCESS && FLAGS_gunit_break_on_failure) {
    // ... then we generate a seg fault.
    *static_cast<int*>(NULL) = 1;
  }
}

// Runs all tests in this UnitTest object and prints the result.
// Returns 0 if successful, or 1 otherwise.
int UnitTest::Run() {
#if _MSC_VER  // We are on Windows.

  if (FLAGS_gunit_catch_exceptions) {
    // The user wants gUnit to catch exceptions thrown by the tests.

    // This lets fatal errors be handled by us, instead of causing pop-ups.
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOALIGNMENTFAULTEXCEPT |
                 SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
  }

  __try {
    return impl_->RunAllTests();
  } __except(UnitTestOptions::GUnitShouldProcessSEH(GetExceptionCode())) {
    printf("Exception thrown with code 0x%x.\nFAIL\n", GetExceptionCode());
    return 1;
  }

#else
  // We are on Linux or Mac OS.  There is no exception of any kind.

  return impl_->RunAllTests();
#endif  // _MSC_VER
}

const TestCase* UnitTest::current_test_case() const {
  return impl_->current_test_case();
}

const TestInfo* UnitTest::current_test_info() const {
  return impl_->current_test_info();
}

// Creates an empty UnitTest.
UnitTest::UnitTest() {
  impl_ = new UnitTestImpl(this);
}

// Destructor of UnitTest.
UnitTest::~UnitTest() {
  delete impl_;
}

UnitTestImpl::UnitTestImpl(UnitTest* parent)
    : parent_(parent),
      test_cases_(),
      last_death_test_case_(NULL),
      current_test_case_(NULL),
      current_test_info_(NULL),
      ad_hoc_test_result_(),
      result_printer_(NULL),
      os_stack_trace_getter_(NULL),
      elapsed_time_(0),
      gunit_trace_stack_(new List<TraceInfo>) {
  UnitTestOptions::SetFlagVarsFromEnvVars();
}

UnitTestImpl::~UnitTestImpl() {
  // Deletes every TestCase.
  test_cases_.ForEach(Delete<TestCase>);

  // Deletes the current test result printer.
  delete result_printer_;

  delete os_stack_trace_getter_;

  // Deletes the gUnit trace stack.
  delete gunit_trace_stack_;
}

namespace {

// A predicate that checks the name of a TestCase against a known
// value.
//
// This is used for implementation of the UnitTest class only.  We put
// it in the anonymous namespace to prevent polluting the outer
// namespace.
//
// TestCaseNameIs is copyable.
class TestCaseNameIs {
 private:
  String name_;

 public:
  // Constructor.
  //
  // TestCaseNameIs has NO default constructor.
  explicit TestCaseNameIs(const String & name)
      : name_(name) {}

  // Returns true iff the name of test_case matches name_.
  bool operator()(const TestCase * test_case) const {
    return test_case != NULL && test_case->name().Compare(name_) == 0;
  }
};

} // namespace

// Finds and returns a TestCase with the given name.  If one doesn't
// exist, creates one and returns it.
//
// Arguments:
//
//   test_case_name: name of the test case
//   set_up_tc:      pointer to the function that sets up the test case
//   tear_down_tc:   pointer to the function that tears down the test case
TestCase* UnitTestImpl::GetTestCase(const String& test_case_name,
                                    Test::SetUpTestCaseFunc set_up_tc,
                                    Test::TearDownTestCaseFunc tear_down_tc) {
  // Can we find a TestCase with the given name?
  ListNode<TestCase*>* node = test_cases_.FindIf(
      TestCaseNameIs(test_case_name));

  if (node == NULL) {
    // No.  Let's create one.
    TestCase* const test_case =
      new TestCase(test_case_name, set_up_tc, tear_down_tc);

    // Is this a death test case?
    if (test_case_name.EndsWith("DeathTest")) {
      // Yes.  Inserts the test case after the last death test case
      // defined so far.
      node = test_cases_.InsertAfter(last_death_test_case_, test_case);
      last_death_test_case_ = node;
    } else {
      // No.  Appends to the end of the list.
      test_cases_.PushBack(test_case);
      node = test_cases_.Last();
    }
  }

  // Returns the TestCase found.
  return node->element();
}

// Runs all tests in this UnitTest object, prints the result, and
// returns 0 if all tests are successful, or 1 otherwise.  If any
// exception is thrown during a test on Windows, this test is
// considered to be failed, but the rest of the tests will still be
// run.  (We disable exceptions on Linux and Mac OS X, so the issue
// doesn't apply there.)
int UnitTestImpl::RunAllTests() {
  // Lists all the tests and exits if the --gunit_list_tests
  // flag was specified.
  if (FLAGS_gunit_list_tests) {
    ListAllTests();
    return 0;
  }

  UnitTestEventListenerInterface * const printer = result_printer();

  // Compares the full test names with the filter to decide which
  // tests to run.
  FilterTests();

  // Tells the unit test event listener that the tests are about to
  // start.
  printer->OnUnitTestStart(parent_);

  const TimeInMillis start = GetTimeInMillis();

  // Runs each test case.
  test_cases_.ForEach(TestCase::RunTestCase);

  elapsed_time_ = GetTimeInMillis() - start;

  // Tells the unit test event listener that the tests have just
  // finished.
  printer->OnUnitTestEnd(parent_);

  // Gets the result and clears it.
  const bool passed = Passed();
  ClearResult();

  // Returns 0 if all tests passed, or 1 other wise.
  return passed ? 0 : 1;
}

// Compares the name of each test with the user-specified filter to
// decide whether the test should be run, then records the result in
// each TestCase and TestInfo object.
void UnitTestImpl::FilterTests() {
  for (const ListNode<TestCase *> *test_case_node = test_cases_.Head();
       test_case_node != NULL;
       test_case_node = test_case_node->next()) {
    TestCase * const test_case = test_case_node->element();
    const String &test_case_name = test_case->name();
    test_case->set_should_run(false);

    for (const ListNode<TestInfo *> *test_info_node =
           test_case->test_info_list().Head();
         test_info_node != NULL;
         test_info_node = test_info_node->next()) {
      TestInfo * const test_info = test_info_node->element();
      const String test_name(test_info->name());
      // A test is disabled if test case name or test name matches
      // kDisableTestPattern.
      const bool is_disabled =
        UnitTestOptions::PatternMatchesString(kDisableTestPattern,
                                              test_case_name.c_str()) ||
        UnitTestOptions::PatternMatchesString(kDisableTestPattern,
                                              test_name.c_str());
      test_info->impl()->set_is_disabled(is_disabled);

      const bool should_run =
        !is_disabled && UnitTestOptions::FilterMatchesTest(test_case_name,
                                                           test_name);
      test_info->impl()->set_should_run(should_run);
      test_case->set_should_run(test_case->should_run() || should_run);
    }
  }
}

// Lists all tests by name.
void UnitTestImpl::ListAllTests() {
  for (const ListNode<TestCase*>* test_case_node = test_cases_.Head();
       test_case_node != NULL;
       test_case_node = test_case_node->next()) {
    const TestCase* const test_case = test_case_node->element();

    // Prints the test case name following by an indented list of test nodes.
    printf("%s.\n", test_case->name().c_str());

    for (const ListNode<TestInfo*>* test_info_node =
         test_case->test_info_list().Head();
         test_info_node != NULL;
         test_info_node = test_info_node->next()) {
      const TestInfo* const test_info = test_info_node->element();

      printf("  %s\n", test_info->name());
    }
  }
}

// Sets the unit test result printer.
//
// Does nothing if the input and the current printer object are the
// same; otherwise, deletes the old printer object and makes the
// input the current printer.
void UnitTestImpl::set_result_printer(
    UnitTestEventListenerInterface* result_printer) {
  if (result_printer_ != result_printer) {
    delete result_printer_;
    result_printer_ = result_printer;
  }
}

// Returns the current unit test result printer if it is not NULL;
// otherwise, creates an appropriate result printer, makes it the
// current printer, and returns it.
UnitTestEventListenerInterface* UnitTestImpl::result_printer() {
  if (result_printer_ == NULL) {
    const String& output_format = UnitTestOptions::GetOutputFormat();
    if (output_format.Equals("xml")) {
      result_printer_ = new XmlUnitTestResultPrinter(
          UnitTestOptions::GetOutputFile().c_str());
    } else {
      if (!output_format.Equals("")) {
        printf("WARNING: unrecognized output format \"%s\" ignored.\n",
               output_format.c_str());
      }
      result_printer_ = new PlainTextUnitTestResultPrinter();
    }
  }
  return result_printer_;
}

// Sets the OS stack trace getter.
//
// Does nothing if the input and the current OS stack trace getter are
// the same; otherwise, deletes the old getter and makes the input the
// current getter.
void UnitTestImpl::set_os_stack_trace_getter(
    OsStackTraceGetterInterface* getter) {
  if (os_stack_trace_getter_ != getter) {
    delete os_stack_trace_getter_;
    os_stack_trace_getter_ = getter;
  }
}

// Returns the current OS stack trace getter if it is not NULL;
// otherwise, creates an OsStackTraceGetter, makes it the current
// getter, and returns it.
OsStackTraceGetterInterface* UnitTestImpl::os_stack_trace_getter() {
  if (os_stack_trace_getter_ == NULL) {
    os_stack_trace_getter_ = new OsStackTraceGetter;
  }

  return os_stack_trace_getter_;
}

// Returns the TestResult for the test that's currently running, or
// the TestResult for the ad hoc test if no test is running.
TestResult* UnitTestImpl::current_test_result() {
  return current_test_info_ ?
    current_test_info_->impl()->result() : &ad_hoc_test_result_;
}

// This ensures that an instance of UnitTest is created before main()
// is invoked.  This is necessary for making the "command line flags
// override environment variables" behavior work.
static const UnitTest* const unit_test_instance = UnitTest::GetInstance();

// TestInfoImpl constructor.
TestInfoImpl::TestInfoImpl(TestInfo* parent,
                           const char* test_case_name,
                           const char* name,
                           TypeId fixture_class_id,
                           TestMaker maker)
    : parent_(parent),
      test_case_name_(String(test_case_name)),
      name_(String(name)),
      fixture_class_id_(fixture_class_id),
      should_run_(false),
      is_disabled_(false),
      maker_(maker) {
}

// TestInfoImpl destructor.
TestInfoImpl::~TestInfoImpl() {
}

namespace {

// Parses a string as a command line flag.  The string should have
// the format "--flag=value".  When def_optional is true, the "=value"
// part can be omitted.
//
// Returns the value of the flag, or NULL if the parsing failed.
const char* ParseFlagValue(const char* str,
                           const char* flag,
                           bool def_optional) {
  // str and flag must not be NULL.
  if (str == NULL || flag == NULL) return NULL;

  // str must start with "--".
  if (str[0] != '-' || str[1] != '-') return NULL;

  // Skips the "--".
  const char* const flag_start = str + 2;

  // The flag name must come after "--".
  const size_t flag_len = strlen(flag);
  if (strncmp(flag_start, flag, flag_len) != 0) return NULL;

  // Skips the flag name.
  const char* flag_end = flag_start + flag_len;

  // When def_optional is true, it's OK to not have a "=value" part.
  if (def_optional && (flag_end[0] == '\0')) {
    return flag_end;
  }

  // If def_optional is true and there are more characters after the
  // flag name, or if def_optional is false, there must be a '=' after
  // the flag name.
  if (flag_end[0] != '=') return NULL;

  // Returns the string after "=".
  return flag_end + 1;
}

// Parses a string for a bool flag, in the form of either
// "--flag=value" or "--flag".
//
// In the former case, the value is taken as true as long as it does
// not start with '0', 'f', or 'F'.
//
// In the latter case, the value is taken as true.
//
// On success, stores the value of the flag in *value, and returns
// true.  On failure, returns false without changing *value.
bool ParseBoolFlag(const char* str, const char* flag, bool* value) {
  // Gets the value of the flag as a string.
  const char* const value_str = ParseFlagValue(str, flag, true);

  // Aborts if the parsing failed.
  if (value_str == NULL) return false;

  // Converts the string value to a bool.
  *value = !(*value_str == '0' || *value_str == 'f' || *value_str == 'F');
  return true;
}

// Parses a string for a string flag, in the form of
// "--flag=value".
//
// On success, stores the value of the flag in *value, and returns
// true.  On failure, returns false without changing *value.
bool ParseStringFlag(const char* str, const char* flag, String* value) {
  // Gets the value of the flag as a string.
  const char* const value_str = ParseFlagValue(str, flag, false);

  // Aborts if the parsing failed.
  if (value_str == NULL) return false;

  // Sets *value to the value of the flag.
  *value = value_str;
  return true;
}

}  // Unnamed namespace.

// The internal implementation of ParseGUnitFlags().
//
// The type parameter CharType can be instantiated to either char or
// wchar_t.
template <typename CharType>
void ParseGUnitFlagsImpl(int* argc, CharType** argv) {
  if (*argc <= 0) return;

  for (int i = 1; i != *argc; i++) {
    const String arg_string = StreamableToString(argv[i]);
    const char* const arg = arg_string.c_str();

    // Do we see a gUnit flag?
    if (ParseBoolFlag(arg,
                      kGUnitBreakOnFailureFlag,
                      &FLAGS_gunit_break_on_failure) ||
        ParseBoolFlag(arg,
                      kGUnitCatchExceptionsFlag,
                      &FLAGS_gunit_catch_exceptions) ||
        ParseStringFlag(arg,
                        kGUnitFilterFlag,
                        &FLAGS_gunit_filter) ||
        ParseBoolFlag(arg,
                      kGUnitListTestsFlag,
                      &FLAGS_gunit_list_tests) ||
        ParseStringFlag(arg,
                        kGUnitOutputFlag,
                        &FLAGS_gunit_output)) {
      // Yes.  Shift the remainder of the argv list left by one.  Note
      // that argv has (*argc + 1) elements, the last one always being
      // NULL.  The following loop moves the trailing NULL element as
      // well.
      for (int j = i; j != *argc; j++) {
        argv[j] = argv[j + 1];
      }

      // Decrements the argument count.
      (*argc)--;

      // We also need to decrement the iterator as we just removed
      // an element.
      i--;
    }
  }
}

// Parses a command line for the flags that gUnit recognizes.
// Whenever a gUnit flag is seen, it is removed from argv, and *argc
// is decremented.
//
// No value is returned.  Instead, the FLAGS_gunit_* variables are
// updated.
void ParseGUnitFlags(int* argc, char** argv) {
  return ParseGUnitFlagsImpl(argc, argv);
}

// This overloaded version can be used in Windows programs compiled in
// UNICODE mode.
#ifdef  _MSC_VER
void ParseGUnitFlags(int* argc, wchar_t** argv) {
  return ParseGUnitFlagsImpl(argc, argv);
}
#endif  // _MSC_VER

} // namespace testing
