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

// Utility functions and classes used by the gUnit testing framework.
//
// This file contains purely gUnit's internal implementation.  Please
// DO NOT INCLUDE IT IN A USER PROGRAM.

#ifndef TESTING_GUNIT_INTERNAL_INL_H__
#define TESTING_GUNIT_INTERNAL_INL_H__

// GUNIT_IMPLEMENTATION is defined iff the current translation unit is
// part of gUnit's implementation.
#ifndef GUNIT_IMPLEMENTATION
// A user is trying to include this from his code - just say no.
#error "gunit-internal-inl.h is part of gUnit's internal implementation."
#error "It must not be included except by gUnit itself."
#endif  // GUNIT_IMPLEMENTATION

#include <stddef.h>

#ifdef _MSC_VER  // If we are on Windows.
#include <windows.h>
#endif  // _MSC_VER

#include "gunit.h"

namespace testing {

// Integer types of given sizes.
typedef TypeWithSize<4>::Int Int32;
typedef TypeWithSize<4>::UInt UInt32;
typedef TypeWithSize<8>::UInt UInt64;

// Converts a Unicode code-point to its UTF-8 encoding.
String ToUtf8String(wchar_t wchar);

// Declares the flag variables.
//
// We don't want the users to modify these flags in the code, but want
// gUnit's own unit tests to be able to access them.  Therefore we
// declare them here as opposed to in gunit.h.
extern bool FLAGS_gunit_break_on_failure;
extern bool FLAGS_gunit_catch_exceptions;
extern String FLAGS_gunit_filter;
extern bool FLAGS_gunit_list_tests;
extern String FLAGS_gunit_output;

// This class saves the values of all gUnit flags in its c'tor, and
// restores them in its d'tor.
class GUnitFlagSaver {
 public:
  // The c'tor.
  GUnitFlagSaver() {
    break_on_failure_ = FLAGS_gunit_break_on_failure;
    catch_exceptions_ = FLAGS_gunit_catch_exceptions;
    filter_ = FLAGS_gunit_filter;
    list_tests_ = FLAGS_gunit_list_tests;
    output_ = FLAGS_gunit_output;
  }

  // The d'tor is not virtual.  DO NOT INHERIT FROM THIS CLASS.
  ~GUnitFlagSaver() {
    FLAGS_gunit_break_on_failure = break_on_failure_;
    FLAGS_gunit_catch_exceptions = catch_exceptions_;
    FLAGS_gunit_filter = filter_;
    FLAGS_gunit_list_tests = list_tests_;
    FLAGS_gunit_output = output_;
  }

 private:
  // Fields for saving the original values of flags.
  bool break_on_failure_;
  bool catch_exceptions_;
  String filter_;
  bool list_tests_;
  String output_;
} GUNIT_ATTRIBUTE_UNUSED;

// List is a simple singly-linked list container.
//
// We cannot use std::list as Microsoft's implementation of STL has
// problems when exception is disabled.  There is a hack to work
// around this, but we've seen cases where the hack fails to work.
//
// TODO(wan): switch to std::list when we have a reliable fix for the
// STL problem, e.g. when we upgrade to the next version of Visual
// C++, or (more likely) switch to STLport.
//
// The element type must support copy constructor.

// Forward declare List
template <typename E>  // E is the element type.
class List;

// ListNode is a node in a singly-linked list.  It consists of an
// element and a pointer to the next node.  The last node in the list
// has a NULL value for its next pointer.
template <typename E>  // E is the element type.
class ListNode {
  friend class List<E>;

 private:

  E element_;
  ListNode * next_;

  // The c'tor is private s.t. only in the ListNode class and in its
  // friend class List we can create a ListNode object.
  //
  // Creates a node with a given element value.  The next pointer is
  // set to NULL.
  //
  // ListNode does NOT have a default constructor.  Always use this
  // constructor (with parameter) to create a ListNode object.
  explicit ListNode(const E & element) : element_(element), next_(NULL) {}

  // We disallow copying ListNode
  GUNIT_DISALLOW_EVIL_CONSTRUCTORS(ListNode);

 public:

  // Gets the element in this node.
  const E & element() const { return element_; }

  // Gets the next node in the list.
  ListNode * next() { return next_; }
  const ListNode * next() const { return next_; }
};


// List is a simple singly-linked list container.
template <typename E>  // E is the element type.
class List {
 public:

  // Creates an empty list.
  List() : head_(NULL), last_(NULL), size_(0) {}

  // D'tor.
  virtual ~List();

  // Clears the list.
  void Clear() {
    if ( size_ > 0 ) {
      // 1. Deletes every node.
      ListNode<E> * node = head_;
      ListNode<E> * next = node->next();
      for ( ; ; ) {
        delete node;
        node = next;
        if ( node == NULL ) break;
        next = node->next();
      }

      // 2. Resets the member variables.
      head_ = last_ = NULL;
      size_ = 0;
    }
  }

  // Gets the number of elements.
  int size() const { return size_; }

  // Gets the first element of the list, or NULL if the list is empty.
  ListNode<E> * Head() { return head_; }
  const ListNode<E> * Head() const { return head_; }

  // Gets the last element of the list, or NULL if the list is empty.
  ListNode<E> * Last() { return last_; }
  const ListNode<E> * Last() const { return last_; }

  // Adds an element to the end of the list.  A copy of the element is
  // created using the copy constructor, and then stored in the list.
  // Changes made to the element in the list doesn't affect the source
  // object, and vice versa.
  void PushBack(const E & element) {
    ListNode<E> * new_node = new ListNode<E>(element);

    if ( size_ == 0 ) {
      head_ = last_ = new_node;
      size_ = 1;
    } else {
      last_->next_ = new_node;
      last_ = new_node;
      size_++;
    }
  }

  // Adds an element to the beginning of this list.
  void PushFront(const E& element) {
    ListNode<E>* const new_node = new ListNode<E>(element);

    if ( size_ == 0 ) {
      head_ = last_ = new_node;
      size_ = 1;
    } else {
      new_node->next_ = head_;
      head_ = new_node;
      size_++;
    }
  }

  // Removes an element from the beginning of this list.  If the
  // result argument is not NULL, the removed element is stored in the
  // memory it points to.  Otherwise the element is thrown away.
  // Returns true iff the list wasn't empty before the operation.
  bool PopFront(E* result) {
    if (size_ == 0) return false;

    if (result != NULL) {
      *result = head_->element_;
    }

    ListNode<E>* const old_head = head_;
    size_--;
    if (size_ == 0) {
      head_ = last_ = NULL;
    } else {
      head_ = head_->next_;
    }
    delete old_head;

    return true;
  }

  // Inserts an element after a given node in the list.  It's the
  // caller's responsibility to ensure that the given node is in the
  // list.  If the given node is NULL, inserts the element at the
  // front of the list.
  ListNode<E>* InsertAfter(ListNode<E>* node, const E& element) {
    if (node == NULL) {
      PushFront(element);
      return Head();
    }

    ListNode<E>* const new_node = new ListNode<E>(element);
    new_node->next_ = node->next_;
    node->next_ = new_node;
    size_++;
    if (node == last_) {
      last_ = new_node;
    }

    return new_node;
  }

  // Returns the number of elements that satisfy a given predicate.
  // The parameter 'predicate' is a Boolean function or functor that
  // accepts a 'const E &', where E is the element type.
  template <typename P>  // P is the type of the predicate function/functor
  int CountIf(P predicate) const {
    int count = 0;
    for ( const ListNode<E> * node = Head();
          node != NULL;
          node = node->next() ) {
      if ( predicate(node->element()) ) {
        count++;
      }
    }

    return count;
  }

  // Applies a function/functor to each element in the list.  The
  // parameter 'functor' is a function/functor that accepts a 'const
  // E &', where E is the element type.  This method does not change
  // the elements.
  template <typename F>  // F is the type of the function/functor
  void ForEach(F functor) const {
    for ( const ListNode<E> * node = Head();
          node != NULL;
          node = node->next() ) {
      functor(node->element());
    }
  }

  // Returns the first node whose element satisfies a given predicate,
  // or NULL if none is found.  The parameter 'predicate' is a
  // function/functor that accepts a 'const E &', where E is the
  // element type.  This method does not change the elements.
  template <typename P>  // P is the type of the predicate function/functor.
  const ListNode<E> * FindIf(P predicate) const {
    for ( const ListNode<E> * node = Head();
          node != NULL;
          node = node->next() ) {
      if ( predicate(node->element()) ) {
        return node;
      }
    }

    return NULL;
  }

  template <typename P>
  ListNode<E> * FindIf(P predicate) {
    for ( ListNode<E> * node = Head();
          node != NULL;
          node = node->next() ) {
      if ( predicate(node->element() ) ) {
        return node;
      }
    }

    return NULL;
  }

 private:
  ListNode<E>* head_;  // The first node of the list.
  ListNode<E>* last_;  // The last node of the list.
  int size_;           // The number of elements in the list.

  // We disallow copying List.
  GUNIT_DISALLOW_EVIL_CONSTRUCTORS(List);
};

// The virtual destructor of List.
template <typename E>
List<E>::~List() {
  Clear();
}

// A function for deleting an object.  Handy for being used as a
// functor.
template <typename T>
static void Delete(T * x) {
  delete x;
}


// An immutable object representing the result of a test part.
//
// TestPartResult is copyable.
//
// Don't inherit from TestPartResult as its destructor is not virtual.
class TestPartResult {
 private:

  TestPartResultType type_;
  String file_name_;  // The name of the source file where the test
                      // part took place, or NULL if the source file
                      // is unknown.
  int line_number_;   // The line in the source file where the test
                      // part took place, or -1 if the line number is
                      // unknown.
  String message_;

 public:

  // C'tor.  TestPartResult does NOT have a default constructor.
  // Always use this constructor (with parameters) to create a
  // TestPartResult object.
  TestPartResult(TestPartResultType type,
                 const String & file_name,
                 int line_number,
                 const String & message)
      : type_(type),
        file_name_(file_name),
        line_number_(line_number),
        message_(message) {
  }

  // TestPartResult doesn't have a user-defined destructor --
  // the default one works fine.

  // Gets the outcome of the test part.
  TestPartResultType type() const  { return type_; }

  // Gets the name of the source file where the test part took place, or
  // NULL if it's unknown.
  const String & file_name() const { return file_name_; }

  // Gets the line in the source file where the test part took place,
  // or -1 if it's unknown.
  int line_number() const { return line_number_; }

  // Gets the message associated with the test part.
  const String & message() const { return message_; }

  // Returns true iff the test part passed.
  bool Passed() const { return type_ == TPRT_SUCCESS; }

  // Returns true iff the test part failed.
  bool Failed() const { return type_ != TPRT_SUCCESS; }

  // Returns true iff the test part fatally failed.
  bool FatallyFailed() const { return type_ == TPRT_FATAL_FAILURE; }
};


// The result of a single Test.  This is essentially a list of
// TestPartResults.
//
// TestResult is not copyable.
class TestResult {
 public:
  // Creates an empty TestResult.
  TestResult();

  // D'tor.  Do not inherit from TestResult.
  ~TestResult();

  // Gets the list of TestPartResults.
  const List<TestPartResult> & test_part_results() const {
    return test_part_results_;
  }

  // Gets the number of successful test parts.
  int successful_part_count() const;

  // Gets the number of failed test parts.
  int failed_part_count() const;

  // Gets the number of all test parts.  This is the sum of the number
  // of successful test parts and the number of failed test parts.
  int total_part_count() const;

  // Returns true iff the test passed (i.e. no test part failed).
  bool Passed() const { return !Failed(); }

  // Returns true iff the test failed.
  bool Failed() const { return failed_part_count() > 0; }

  // Returns true iff the test fatally failed.
  bool HasFatalFailure() const;

  // Returns the elapsed time, in milliseconds.
  TimeInMillis elapsed_time() const { return elapsed_time_; }

  // Sets the elapsed time.
  void set_elapsed_time(TimeInMillis elapsed) { elapsed_time_ = elapsed; }

  // Adds a test part result to the list.
  void AddTestPartResult(const TestPartResult& test_part_result);

  // Clears the object.
  void Clear();

 private:
  // The list of TestPartResults
  List<TestPartResult> test_part_results_;

  // The elapsed time, in milliseconds.
  TimeInMillis elapsed_time_;

  // We disallow copying TestResult.
  GUNIT_DISALLOW_EVIL_CONSTRUCTORS(TestResult);
};


class TestInfoImpl {
 public:
  TestInfoImpl(TestInfo* parent, const char* test_case_name,
               const char* name, TypeId fixture_class_id,
               TestMaker maker);
  ~TestInfoImpl();

  // Returns true if this test should run.
  bool should_run() const { return should_run_; }

  // Sets the should_run member.
  void set_should_run(bool should) { should_run_ = should; }

  // Returns true if this test is disabled. Disabled tests are not run.
  bool is_disabled() const { return is_disabled_; }

  // Sets the is_disabled member.
  void set_is_disabled(bool is) { is_disabled_ = is; }

  // Returns the test case name.
  const char* test_case_name() const { return test_case_name_.c_str(); }

  // Returns the test name.
  const char* name() const { return name_.c_str(); }

  // Returns the ID of the test fixture class.
  TypeId fixture_class_id() const { return fixture_class_id_; }

  // Returns the test result.
  TestResult* result() { return &result_; }
  const TestResult* result() const { return &result_; }

  // Creates the test object, runs it, records its result, and then
  // deletes it.
  void Run();

  // Calls the given TestInfo object's Run() method.
  static void RunTest(TestInfo * test_info) {
    test_info->impl()->Run();
  }

  // Clears the test result.
  void ClearResult() { result_.Clear(); }

  // Clears the test result in the given TestInfo object.
  static void ClearTestResult(TestInfo * test_info) {
    test_info->impl()->ClearResult();
  }

 private:
  TestInfo* const parent_;         // The owner of this object
  const String test_case_name_;    // Test case name
  const String name_;              // Test name
  const TypeId fixture_class_id_;  // ID of the test fixture class
  bool should_run_;                // True iff this test should run
  bool is_disabled_;               // True iff this test is disabled
  const TestMaker maker_;          // The function that creates the test object
  TestResult result_;              // Test result

  GUNIT_DISALLOW_EVIL_CONSTRUCTORS(TestInfoImpl);
};

// A test case, which consists of a list of TestInfos.
//
// TestCase is not copyable.
class TestCase {
 public:
  // Creates a TestCase with the given name.
  //
  // TestCase does NOT have a default constructor.  Always use this
  // constructor to create a TestCase object.
  //
  // Arguments:
  //
  //   name:         name of the test case
  //   set_up_tc:    pointer to the function that sets up the test case
  //   tear_down_tc: pointer to the function that tears down the test case
  TestCase(const String& name,
           Test::SetUpTestCaseFunc set_up_tc,
           Test::TearDownTestCaseFunc tear_down_tc);

  // Destructor of TestCase.
  virtual ~TestCase();

  // Gets the name of the TestCase.
  const String & name() const { return name_; }

  // Returns true if any test in this test case should run.
  bool should_run() const { return should_run_; }

  // Sets the should_run member.
  void set_should_run(bool should) { should_run_ = should; }

  // Gets the (mutable) list of TestInfos in this TestCase.
  List<TestInfo*>& test_info_list() { return *test_info_list_; }

  // Gets the (immutable) list of TestInfos in this TestCase.
  const List<TestInfo *> & test_info_list() const { return *test_info_list_; }

  // Gets the number of successful tests in this test case.
  int successful_test_count() const;

  // Gets the number of failed tests in this test case.
  int failed_test_count() const;

  // Gets the number of disabled tests in this test case.
  int disabled_test_count() const;

  // Get the number of tests in this test case that should run.
  int test_to_run_count() const;

  // Gets the number of all tests in this test case.
  int total_test_count() const;

  // Returns true iff the test case passed.
  bool Passed() const { return !Failed(); }

  // Returns true iff the test case failed.
  bool Failed() const { return failed_test_count() > 0; }

  // Returns the elapsed time, in milliseconds.
  TimeInMillis elapsed_time() const { return elapsed_time_; }

  // Adds a TestInfo to this test case.  Will delete the TestInfo upon
  // destruction of the TestCase object.
  void AddTestInfo(TestInfo * test_info);

  // Finds and returns a TestInfo with the given name.  If one doesn't
  // exist, returns NULL.
  TestInfo * GetTestInfo(const String & test_name);

  // Clears the results of all tests in this test case.
  void ClearResult();

  // Clears the results of all tests in the given test case.
  static void ClearTestCaseResult(TestCase* test_case) {
    test_case->ClearResult();
  }

  // Runs every test in this TestCase.
  void Run();

  // Runs every test in the given TestCase.
  static void RunTestCase(TestCase * test_case) { test_case->Run(); }

  // Returns true iff test passed.
  static bool TestPassed(const TestInfo * test_info) {
    const TestInfoImpl* const impl = test_info->impl();
    return impl->should_run() && impl->result()->Passed();
  }

  // Returns true iff test failed.
  static bool TestFailed(const TestInfo * test_info) {
    const TestInfoImpl* const impl = test_info->impl();
    return impl->should_run() && impl->result()->Failed();
  }

  // Returns true iff test is disabled.
  static bool TestDisabled(const TestInfo * test_info) {
    return test_info->impl()->is_disabled();
  }

  // Returns true if the given test should run.
  static bool ShouldRunTest(const TestInfo *test_info) {
    return test_info->impl()->should_run();
  }


 private:
  // Name of the test case.
  String name_;
  // List of TestInfos.
  List<TestInfo*>* test_info_list_;
  // Pointer to the function that sets up the test case.
  Test::SetUpTestCaseFunc set_up_tc_;
  // Pointer to the function that tears down the test case.
  Test::TearDownTestCaseFunc tear_down_tc_;
  // True iff any test in this test case should run.
  bool should_run_;
  // Elapsed time, in milliseconds.
  TimeInMillis elapsed_time_;

  // We disallow copying TestCases.
  GUNIT_DISALLOW_EVIL_CONSTRUCTORS(TestCase);
};

// Class UnitTestOptions.
//
// This class contains functions for processing options the user
// specifies when running the tests.  It has only static members.
//
// In most cases, the user can specify an option using either an
// environment variable or a command line flag.  E.g. you can set the
// test filter using either GUNIT_FILTER or --gunit_filter.  If both
// the variable and the flag are present, the latter overrides the
// former.
class UnitTestOptions {
 public:
  // Functions for reading environment variables.

  // Reads and returns a string environment variable; if it's not set,
  // returns default_value.
  static const char* ReadStringEnvVar(const char* env_var,
                                      const char* default_value);

  // Reads and returns a Boolean environment variable; if it's not set,
  // returns default_value.
  //
  // The value is considered true iff it's not "0".
  static bool ReadBoolEnvVar(const char* env_var, bool default_value);

  // Reads and returns a 32-bit integer stored in an environment
  // variable; if it isn't set or doesn't represent a valid 32-bit
  // integer, returns default_value.
  static Int32 ReadInt32EnvVar(const char* env_var, Int32 default_value);

  // Copies the values of the gUnit environment variables to the flag
  // variables.
  //
  // This function must be called before the command line is parsed in
  // main(), in order to allow command line flags to override the
  // environment variables.  We call it in the constructor of
  // UnitTestImpl, which is a singleton created when the first test is
  // defined, before main() is reached.
  static void SetFlagVarsFromEnvVars();

  // Functions for processing the gunit_output flag.

  // Returns the output format, or "" for normal printed output.
  static String GetOutputFormat();

  // Returns the name of the requested output file, or the default if none
  // was explicitly specified.
  static String GetOutputFile();

  // Functions for processing the gunit_filter flag.

  // Returns true iff the wildcard pattern matches the string.  The
  // first ':' or '\0' character in pattern marks the end of it.
  //
  // This recursive algorithm isn't very efficient, but is clear and
  // works well enough for matching test names, which are short.
  static bool PatternMatchesString(const char *pattern, const char *str);

  // Returns true iff the user-specified filter matches the test case
  // name and the test name.
  static bool FilterMatchesTest(const String &test_case_name,
                                const String &test_name);

#ifdef _MSC_VER  // We are on Windows.
  // Function for supporting the gunit_catch_exception flag.

  // Returns EXCEPTION_EXECUTE_HANDLER if gUnit should handle the
  // given SEH exception, or EXCEPTION_CONTINUE_SEARCH otherwise.
  // This function is useful as an __except condition.
  static int GUnitShouldProcessSEH(DWORD exception_code);
#endif  // _MSC_VER
 private:
  // Returns true if "name" matches the ':' separated list of glob-style
  // filters in "filter".
  static bool MatchesFilter(const String& name, const char* filter);

};

class UnitTestEventListenerInterface;

// The role interface for getting the OS stack trace as a string.
class OsStackTraceGetterInterface {
 public:
  OsStackTraceGetterInterface() {}
  virtual ~OsStackTraceGetterInterface() {}

  // Returns the current OS stack trace as a String.  Parameters:
  //
  //   max_depth  - the maximum number of stack frames to be included
  //                in the trace.
  //   skip_count - the number of top frames to be skipped; doesn't count
  //                against max_depth.
  virtual String CurrentStackTrace(int max_depth, int skip_count) = 0;

  // UponLeavingGUnit() should be called immediately before gUnit calls
  // user code. It saves some information about the current stack that
  // CurrentStackTrace() will use to find and hide gUnit stack frames.
  virtual void UponLeavingGUnit() = 0;

 private:
  GUNIT_DISALLOW_EVIL_CONSTRUCTORS(OsStackTraceGetterInterface);
};

// A working implemenation of the OsStackTraceGetterInterface interface.
class OsStackTraceGetter : public OsStackTraceGetterInterface {
 public:
  OsStackTraceGetter() {}
  virtual String CurrentStackTrace(int max_depth, int skip_count);
  virtual void UponLeavingGUnit();

  // This string is inserted in place of stack frames that are part of
  // gUnit's implementation.
  static const char* const kElidedFramesMarker;

 private:
  GUNIT_DISALLOW_EVIL_CONSTRUCTORS(OsStackTraceGetter);
};

// The private implementation of the UnitTest class.
class UnitTestImpl {
 public:
  explicit UnitTestImpl(UnitTest* parent);
  ~UnitTestImpl();

  // Gets the number of successful test cases.
  int successful_test_case_count() const;

  // Gets the number of failed test cases.
  int failed_test_case_count() const;

  // Gets the number of all test cases.
  int total_test_case_count() const;

  // Gets the number of all test cases that contain at least one test
  // that should run.
  int test_case_to_run_count() const;

  // Gets the number of successful tests.
  int successful_test_count() const;

  // Gets the number of failed tests.
  int failed_test_count() const;

  // Gets the number of disabled tests.
  int disabled_test_count() const;

  // Gets the number of all tests.
  int total_test_count() const;

  // Gets the number of tests that should run.
  int test_to_run_count() const;

  // Gets the elapsed time, in milliseconds.
  TimeInMillis elapsed_time() const { return elapsed_time_; }

  // Returns true iff the unit test passed (i.e. all test cases passed).
  bool Passed() const { return !Failed(); }

  // Returns true iff the unit test failed (i.e. some test case failed
  // or something outside of all tests failed).
  bool Failed() const {
    return failed_test_case_count() > 0 || ad_hoc_test_result()->Failed();
  }

  // Returns the TestResult for the test that's currently running, or
  // the TestResult for the ad hoc test if no test is running.
  TestResult* current_test_result();

  // Returns the TestResult for the ad hoc test.
  const TestResult* ad_hoc_test_result() const { return &ad_hoc_test_result_; }

  // Sets the unit test result printer.
  //
  // Does nothing if the input and the current printer object are the
  // same; otherwise, deletes the old printer object and makes the
  // input the current printer.
  void set_result_printer(UnitTestEventListenerInterface * result_printer);

  // Returns the current unit test result printer if it is not NULL;
  // otherwise, creates an appropriate result printer, makes it the
  // current printer, and returns it.
  UnitTestEventListenerInterface * result_printer();

  // Sets the OS stack trace getter.
  //
  // Does nothing if the input and the current OS stack trace getter
  // are the same; otherwise, deletes the old getter and makes the
  // input the current getter.
  void set_os_stack_trace_getter(OsStackTraceGetterInterface* getter);

  // Returns the current OS stack trace getter if it is not NULL;
  // otherwise, creates an OsStackTraceGetter, makes it the current
  // getter, and returns it.
  OsStackTraceGetterInterface* os_stack_trace_getter();

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
  String CurrentOsStackTraceExceptTop(int skip_count);

  // Finds and returns a TestCase with the given name.  If one doesn't
  // exist, creates one and returns it.
  //
  // Arguments:
  //
  //   test_case_name: name of the test case
  //   set_up_tc:      pointer to the function that sets up the test case
  //   tear_down_tc:   pointer to the function that tears down the test case
  TestCase* GetTestCase(const String& test_case_name,
                        Test::SetUpTestCaseFunc set_up_tc,
                        Test::TearDownTestCaseFunc tear_down_tc);

  // Adds a TestInfo to the unit test.
  //
  // Arguments:
  //
  //   set_up_tc:    pointer to the function that sets up the test case
  //   tear_down_tc: pointer to the function that tears down the test case
  //   test_info:    the TestInfo object
  void AddTestInfo(Test::SetUpTestCaseFunc set_up_tc,
                   Test::TearDownTestCaseFunc tear_down_tc,
                   TestInfo * test_info) {
    GetTestCase(String(test_info->test_case_name()),
                set_up_tc,
                tear_down_tc)->AddTestInfo(test_info);
  }

  // Sets the TestCase object for the test that's currently running.
  void set_current_test_case(TestCase* current_test_case) {
    current_test_case_ = current_test_case;
  }

  // Sets the TestInfo object for the test that's currently running.  If
  // current_test_info is NULL, the assertion results will be stored in
  // ad_hoc_test_result_.
  void set_current_test_info(TestInfo* current_test_info) {
    current_test_info_ = current_test_info;
  }

  // Runs all tests in this UnitTest object, prints the result, and
  // returns 0 if all tests are successful, or 1 otherwise.  If any
  // exception is thrown during a test on Windows, this test is
  // considered to be failed, but the rest of the tests will still be
  // run.  (We disable exceptions on Linux and Mac OS X, so the issue
  // doesn't apply there.)
  int RunAllTests();

  // Clears the results of all tests, including the ad hoc test.
  void ClearResult() {
    test_cases_.ForEach(TestCase::ClearTestCaseResult);
    ad_hoc_test_result_.Clear();
  }

  // Matches the full name of each test against the user-specified
  // filter to decide whether the test should run, then records the
  // result in each TestCase and TestInfo object.
  void FilterTests();

  // Lists all the tests by name.
  void ListAllTests();

  const TestCase* current_test_case() const { return current_test_case_; }
  TestInfo* current_test_info() { return current_test_info_; }
  const TestInfo* current_test_info() const { return current_test_info_; }

  List<TestCase*>* test_cases() { return &test_cases_; }
  const List<TestCase*>* test_cases() const { return &test_cases_; }

  List<TraceInfo>* gunit_trace_stack() { return gunit_trace_stack_; }
  const List<TraceInfo>* gunit_trace_stack() const {
    return gunit_trace_stack_;
  }

 private:
  // The UnitTest object that owns this implementation object.
  UnitTest* const parent_;

  List<TestCase*> test_cases_;  // The list of TestCases.

  // Points to the last death test case registered.  Initially NULL.
  ListNode<TestCase*>* last_death_test_case_;

  // This points to the TestCase for the currently running test.  It
  // changes as gUnit goes through one test case after another.  When
  // no test is running, this is set to NULL and gUnit stores
  // assertion results in ad_hoc_test_result_.  Initally NULL.
  TestCase* current_test_case_;

  // This points to the TestInfo for the currently running test.  It
  // changes as gUnit goes through one test after another.  When no test
  // is running, this is set to NULL and gUnit stores assertion results
  // in ad_hoc_test_result_.  Initally NULL.
  TestInfo* current_test_info_;

  // Normally, a user only writes assertions inside a TEST or TEST_F,
  // or inside a function called by a TEST or TEST_F.  Since gUnit
  // keeps track of which test is current running, it can associate
  // such an assertion with the test it belongs to.
  //
  // If an assertion is encountered when no TEST or TEST_F is running,
  // gUnit attributes the assertion result to an imaginary "ad hoc"
  // test, and records the result in ad_hoc_test_result_.
  TestResult ad_hoc_test_result_;

  // The unit test result printer.  Will be deleted when the UnitTest
  // object is destructed.  By default, a plain text printer is used,
  // but the user can set this field to use a custom printer if that
  // is desired.
  UnitTestEventListenerInterface* result_printer_;

  // The OS stack trace getter.  Will be deleted when the UnitTest
  // object is destructed.  By default, an OsStackTraceGetter is used,
  // but the user can set this field to use a custom getter if that is
  // desired.
  OsStackTraceGetterInterface* os_stack_trace_getter_;

  // How long the test took to run, in milliseconds.
  TimeInMillis elapsed_time_;

  // A stack of traces created by the SCOPED_TRACE() macro.
  List<TraceInfo>* const gunit_trace_stack_;

  GUNIT_DISALLOW_EVIL_CONSTRUCTORS(UnitTestImpl);
};

// Returns string with character 'ch' repeated 'n' times.
String Repeat(int n, char ch);

// Returns banner used for printing information about disabled tests.
// Padding length is used to center message in a nicely formatted ASCII
// box.
String DisabledTestBanner(int disabled_count, int padding_length);

} // namespace testing

#endif  // TESTING_GUNIT_INTERNAL_INL_H__
