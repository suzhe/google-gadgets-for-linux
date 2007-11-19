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

// gUnit definitions useful in production code.
//
// This header defines the FRIEND_TEST macro for testing private or
// protected members of a class.
//
// Here's an example:
//
// - The foo.h file:
//
//   #include "unittest/gunit_prod.h"
//   ...
//   class Foo {
//    public:
//     FRIEND_TEST(FooTest, Bar);
//   ...
//

#ifndef TESTING_GUNIT_PROD_H__
#define TESTING_GUNIT_PROD_H__

// When you need to test the private or protected members of a class,
// use the FRIEND_TEST macro to declare your tests as friends of the
// class.
#define FRIEND_TEST(test_case_name, test_name)\
friend class test_case_name##_##test_name##_Test

#endif  // TESTING_GUNIT_PROD_H__
