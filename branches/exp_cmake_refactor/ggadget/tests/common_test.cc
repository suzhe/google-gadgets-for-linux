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

#include <cstdio>
#include "ggadget/common.h"
#include "unittest/gunit.h"

using namespace ggadget;

TEST(Common, AS_STRING_MACRO) {
#define STRING_A aaa bbb ccc
  ASSERT_STREQ("aaa bbb ccc", AS_STRING(STRING_A));
}

TEST(Common, LOG_MACRO) {
  LOG("%d", 100);
  DLOG("%d", 200);
}

TEST(Common, ASSERT_MACRO) {
  ASSERT(true);
  ASSERT_M(true, ("Some message: %d", 100));
  VERIFY(true);
  VERIFY_M(true, ("Some message: %d", 200));
  VERIFY(false);
  VERIFY_M(false, ("Some message: %d", 300));
  // Should fail at runtime
  // ASSERT(false);
  // ASSERT_M(false, ("message: %d", 400));
}

TEST(Common, COMPILE_ASSERT_MACRO) {
  COMPILE_ASSERT(true, True_Msg);
  COMPILE_ASSERT(sizeof(char) == 1, True_Msg1);
  // Should fail at compile time:
  // COMPILE_ASSERT(false, False_Msg);
}

class A {
};

class B : public A {
};

class C : public A {
};

class D {
};

TEST(Common, IsDerived) {
  ASSERT_TRUE((IsDerived<A, B>::value));
  ASSERT_FALSE((IsDerived<B, A>::value));
  ASSERT_TRUE((IsDerived<A, C>::value));
  ASSERT_TRUE((IsDerived<A, A>::value));
  ASSERT_FALSE((IsDerived<B, C>::value));
  ASSERT_FALSE((IsDerived<A, D>::value));
  ASSERT_FALSE((IsDerived<D, A>::value));

  // Make sure IsDerived is compile time.
  char p[IsDerived<A, B>::value ? 10 : 20];
  ASSERT_EQ(10U, sizeof(p));
  char p1[IsDerived<B, A>::value ? 10 : 20];
  ASSERT_EQ(20U, sizeof(p1));
}

struct S {
  int x;
  double y;
};

TEST(Common, arraysize_MACRO) {
  int array[20];
  ASSERT_EQ(20U, arraysize(array));
  S array1[arraysize(array)];
  ASSERT_EQ(20U, arraysize(array1));
  // Ensure arraysize is computed at compile time.
  ASSERT_EQ(20U * sizeof(array1[0]), sizeof(array1));
  // Should fail at compile time.
  // int *p = array;
  // arraysize(p);
}

TEST(Common, IsDerived_COMPILE_ASSERT) {
  COMPILE_ASSERT((IsDerived<A, B>::value), Yes);
  // Should fail at compile time.
  // COMPILE_ASSERT((IsDerived<B, A>::value), No);
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
