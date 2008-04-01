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
#include "unittest/gtest.h"

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

TEST(StringUtils, EncodeURL) {
  // Valid url chars, no conversion
  char src1[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890`-=;',./~!@#$%^&*()_+|:?";

  // Invalid url chars, will be converted
  char src2[] = " []{}<>\"";

  // Back slash, will be converted to '/'
  char src3[] = "\\";

  // Valid but invisible chars, will be converted
  char src4[] = "\a\b\f\n\r\t\v\007\013\xb\x7";

  // Non-ascii chars(except \x7f), will be converted
  char src5[] = "\x7f\x80\x81 asd\x8f 3\x9a\xaa\xfe\xff";

  std::string dest;

  dest = EncodeURL(src1);
  EXPECT_STREQ(src1, dest.c_str());

  dest = EncodeURL(src2);
  EXPECT_STREQ("%20%5b%5d%7b%7d%3c%3e%22", dest.c_str());

  dest = EncodeURL(src3);
  EXPECT_STREQ("/", dest.c_str());

  dest = EncodeURL(src4);
  EXPECT_STREQ("%07%08%0c%0a%0d%09%0b%07%0b%0b%07", dest.c_str());

  dest = EncodeURL(src5);
  EXPECT_STREQ("\x7f%80%81%20asd%8f%203%9a%aa%fe%ff", dest.c_str());
}

TEST(StringUtils, EncodeJavaScriptString) {
  UTF16Char src1[] = { '\"', '\\', 'a', 'b', 1, 0x1f, 0xfff, 0 };
  std::string dest = EncodeJavaScriptString(src1);
  EXPECT_STREQ("\\\"\\\\ab\\u0001\\u001F\\u0FFF", dest.c_str());
}

TEST(StringUtils, SplitString) {
  std::string left, right;
  EXPECT_TRUE(SplitString("", "", &left, &right));
  EXPECT_STREQ("", left.c_str());
  EXPECT_STREQ("", right.c_str());
  EXPECT_TRUE(SplitString("abcde", "", &left, &right));
  EXPECT_STREQ("", left.c_str());
  EXPECT_STREQ("abcde", right.c_str());
  EXPECT_TRUE(SplitString("abcde", "c", &left, &right));
  EXPECT_STREQ("ab", left.c_str());
  EXPECT_STREQ("de", right.c_str());
  EXPECT_TRUE(SplitString("abcde", "abcde", &left, &right));
  EXPECT_STREQ("", left.c_str());
  EXPECT_STREQ("", right.c_str());
  EXPECT_TRUE(SplitString("abcdeabcde", "a", &left, &right));
  EXPECT_STREQ("", left.c_str());
  EXPECT_STREQ("bcdeabcde", right.c_str());
  EXPECT_TRUE(SplitString("abcdeabcde", "d", &left, &right));
  EXPECT_STREQ("abc", left.c_str());
  EXPECT_STREQ("eabcde", right.c_str());
  EXPECT_FALSE(SplitString("abcde", "cb", &left, &right));
  EXPECT_STREQ("abcde", left.c_str());
  EXPECT_STREQ("", right.c_str());
}

TEST(StringUtils, CompressWhiteSpaces) {
  EXPECT_STREQ("", CompressWhiteSpaces("").c_str());
  EXPECT_STREQ("", CompressWhiteSpaces(" \n\r\t  ").c_str());
  EXPECT_STREQ("A", CompressWhiteSpaces("A").c_str());
  EXPECT_STREQ("A", CompressWhiteSpaces(" A ").c_str());
  EXPECT_STREQ("A", CompressWhiteSpaces("   A   ").c_str());
  EXPECT_STREQ("AB", CompressWhiteSpaces("AB").c_str());
  EXPECT_STREQ("AB", CompressWhiteSpaces(" AB ").c_str());
  EXPECT_STREQ("AB", CompressWhiteSpaces("  AB  ").c_str());
  EXPECT_STREQ("A AB ABC", CompressWhiteSpaces("  A     AB     ABC ").c_str());
}

TEST(StringUtils, ExtractTextFromHTML) {
  EXPECT_STREQ("", ExtractTextFromHTML("").c_str());
  EXPECT_STREQ("< > &' \" \xc2\xa9 \xc2\xae<< &unknown;0\xf4\x81\x84\x91\xe2\x80\x89 Text",
    ExtractTextFromHTML(
      " <script language=\"javascript\"> some script and may be <tags>\n"
      " </script>\n"
      " <!-- some comments <tags> <script> -->\n"
      " <style>style</style>\n"
      " <input type='button' value='<tag>'>\n"
      " &lt; &gt &amp&apos; &nbsp; &nbsp; &quot;<b>&copy;</b>&reg;&lt&lt\n"
      " &#32;&#x&#&unknown;&#x30;&#x101111;&#x2009;\n\r\t Text ").c_str());
}

TEST(StringUtils, SimpleMatchXPath) {
  EXPECT_TRUE(SimpleMatchXPath("", ""));
  EXPECT_TRUE(SimpleMatchXPath("a[1]", "a"));
  // Invalid pattern: no '[' or ']' is allowed.
  EXPECT_FALSE(SimpleMatchXPath("a[1]", "a[1]"));
  EXPECT_TRUE(SimpleMatchXPath("a[1]/b[9999]/c[10000]@d", "a/b/c@d"));
  EXPECT_FALSE(SimpleMatchXPath("a[1]/b[9999]/c[10000]@d", "a/b/c@f"));
  // Missing closing ']'.
  EXPECT_FALSE(SimpleMatchXPath("a[1]/b[9999]/c[10000@d", "a/b/c@d"));
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
