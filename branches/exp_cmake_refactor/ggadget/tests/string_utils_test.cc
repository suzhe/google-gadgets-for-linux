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
#include "ggadget/string_utils.h"
#include "unittest/gunit.h"

using namespace ggadget;

TEST(StringUtils, AssignIfDiffer) {
  std::string s;
  ASSERT_FALSE(AssignIfDiffer(NULL, &s));
  ASSERT_STREQ("", s.c_str());
  ASSERT_FALSE(AssignIfDiffer("", &s));
  ASSERT_STREQ("", s.c_str());
  ASSERT_TRUE(AssignIfDiffer("abcd", &s));
  ASSERT_STREQ("abcd", s.c_str());
  ASSERT_FALSE(AssignIfDiffer("abcd", &s));
  ASSERT_STREQ("abcd", s.c_str());
  ASSERT_TRUE(AssignIfDiffer("1234", &s));
  ASSERT_STREQ("1234", s.c_str());
  ASSERT_TRUE(AssignIfDiffer("", &s));
  ASSERT_STREQ("", s.c_str());
  s = "qwer";
  ASSERT_TRUE(AssignIfDiffer(NULL, &s));
  ASSERT_STREQ("", s.c_str());
}
  
TEST(StringUtils, TrimString) {
  EXPECT_STREQ("", TrimString("").c_str());
  EXPECT_STREQ("", TrimString("  \n \r \t ").c_str());
  EXPECT_STREQ("a b\r c", TrimString(" a b\r c \r\t ").c_str());
  EXPECT_STREQ("a b c", TrimString("a b c  ").c_str());
  EXPECT_STREQ("a b c", TrimString("  a b c").c_str());
  EXPECT_STREQ("a b c", TrimString("a b c").c_str());
  EXPECT_STREQ("abc", TrimString("abc").c_str());
}

TEST(StringUtils, ToUpper) {
  EXPECT_STREQ("", ToUpper("").c_str());
  EXPECT_STREQ("ABCABC123", ToUpper("abcABC123").c_str());
}

TEST(StringUtils, ToLower) {
  EXPECT_STREQ("", ToLower("").c_str());
  EXPECT_STREQ("abcabc123", ToLower("abcABC123").c_str());
}

TEST(StringUtils, StringPrintf) {
  EXPECT_STREQ("123", StringPrintf("%d", 123).c_str());
  char *buf = new char[100000];
  for (int i = 0; i < 100000; i++)
    buf[i] = (i % 50) + '0';
  buf[99999] = 0;
  EXPECT_STREQ(buf, StringPrintf("%s", buf).c_str());
  delete buf;
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
