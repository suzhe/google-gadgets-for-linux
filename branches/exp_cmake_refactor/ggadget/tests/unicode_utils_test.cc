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
#include "ggadget/unicode_utils.h"
#include "unittest/gunit.h"

using namespace ggadget;

static const UTF32Char utf32_string[] = {
  0x0061, 0x0080, 0x07ff, 0x0800, 0x1fff, 0x2000, 0xd7ff,
  0xe000, 0xffff, 0x10000, 0x22000, 0xeffff,
  0xf0000, 0x10aaff, 0
};

static const size_t utf8_length[] = {
  1, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 0
};

static const char utf8_string[] = {
  0x61,0xc2,0x80,0xdf,0xbf,0xe0,0xa0,0x80,0xe1,0xbf,0xbf,0xe2,0x80,0x80,
  0xed,0x9f,0xbf,0xee,0x80,0x80,0xef,0xbf,0xbf,0xf0,0x90,0x80,0x80,
  0xf0,0xa2,0x80,0x80,0xf3,0xaf,0xbf,0xbf,0xf3,0xb0,0x80,0x80,
  0xf4,0x8a,0xab,0xbf, 0
};

static const size_t utf16_length[] = {
  1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 0
};

static const UTF16Char utf16_string[] = {
  0x0061, 0x0080, 0x07ff, 0x0800, 0x1fff, 0x2000, 0xd7ff,
  0xe000, 0xffff, 0xd800, 0xdc00, 0xd848, 0xdc00, 0xdb7f,
  0xdfff, 0xdb80, 0xdc00, 0xdbea, 0xdeff, 0
};

static const size_t invalid_utf8_length = 8;
static const char invalid_utf8_string[] = {
  //---------------------------------------v
  0x61,0xc2,0x80,0xdf,0xbf,0xe0,0xa0,0x80,0xb1,0xbf,0xbf,0xe2,0x80,0x80, 0
};

static const size_t invalid_utf16_length = 9;
static const UTF16Char invalid_utf16_string[] = {
  0x0061, 0x0080, 0x07ff, 0x0800, 0x1fff, 0x2000, 0xd7ff,
  0xe000, 0xffff, 0xd800, 0xc200, 0xd848, 0xdc00, 0xdb7f,
  //-------------------------^
  0
};

static const size_t invalid_utf32_length = 7;
static const UTF32Char invalid_utf32_string[] = {
  0x0061, 0x0080, 0x07ff, 0x0800, 0x1fff, 0x2000, 0xd7ff,
  0xd820, 0xffff, 0
};


TEST(UnicodeUtils, ConvertChar) {
  const char *utf8_ptr = utf8_string;
  const UTF16Char *utf16_ptr = utf16_string;
  const UTF32Char *utf32_ptr = utf32_string;
  UTF32Char utf32;
  char utf8[6];
  UTF16Char utf16[2];
  for (size_t i = 0; utf8_length[i]; ++i) {
    EXPECT_EQ(utf8_length[i],
              ConvertCharUTF8ToUTF32(utf8_ptr, utf8_length[i], &utf32));
    EXPECT_EQ(*utf32_ptr, utf32);
    EXPECT_EQ(utf8_length[i],
              ConvertCharUTF32ToUTF8(utf32, utf8, 6));
    EXPECT_EQ(0,
              strncmp(utf8, utf8_ptr, utf8_length[i]));
    EXPECT_EQ(utf16_length[i],
              ConvertCharUTF16ToUTF32(utf16_ptr, utf16_length[i], &utf32));
    EXPECT_EQ(*utf32_ptr, utf32);
    EXPECT_EQ(utf16_length[i],
              ConvertCharUTF32ToUTF16(utf32, utf16, 2));
    EXPECT_EQ(0, memcmp(utf16, utf16_ptr, sizeof(UTF16Char) * utf16_length[i]));
    utf8_ptr += utf8_length[i];
    utf16_ptr += utf16_length[i];
    utf32_ptr ++;
  }
}

TEST(UnicodeUtils, ConvertString) {
  std::string utf8;
  std::string orig_utf8(utf8_string);
  UTF16String utf16;
  UTF16String orig_utf16(utf16_string);
  UTF32String utf32;
  UTF32String orig_utf32(utf32_string);

  EXPECT_EQ(orig_utf8.length(), ConvertStringUTF8ToUTF32(orig_utf8, &utf32));
  EXPECT_TRUE(utf32 == orig_utf32);
  EXPECT_EQ(orig_utf32.length(), ConvertStringUTF32ToUTF8(orig_utf32, &utf8));
  EXPECT_TRUE(utf8 == orig_utf8);
  EXPECT_EQ(orig_utf16.length(), ConvertStringUTF16ToUTF32(orig_utf16, &utf32));
  EXPECT_TRUE(utf32 == orig_utf32);
  EXPECT_EQ(orig_utf32.length(), ConvertStringUTF32ToUTF16(orig_utf32, &utf16));
  EXPECT_TRUE(utf16 == orig_utf16);
  EXPECT_EQ(orig_utf8.length(), ConvertStringUTF8ToUTF16(orig_utf8, &utf16));
  EXPECT_TRUE(utf16 == orig_utf16);
  EXPECT_EQ(orig_utf16.length(), ConvertStringUTF16ToUTF8(orig_utf16, &utf8));
  EXPECT_TRUE(utf8 == orig_utf8);
}

TEST(UnicodeUtils, Invalid) {
  std::string utf8;
  std::string orig_utf8(invalid_utf8_string);
  UTF16String utf16;
  UTF16String orig_utf16(invalid_utf16_string);
  UTF32String utf32;
  UTF32String orig_utf32(invalid_utf32_string);

  EXPECT_EQ(invalid_utf8_length, ConvertStringUTF8ToUTF32(orig_utf8, &utf32));
  EXPECT_EQ(invalid_utf32_length, ConvertStringUTF32ToUTF8(orig_utf32, &utf8));
  EXPECT_EQ(invalid_utf16_length, ConvertStringUTF16ToUTF32(orig_utf16, &utf32));
  EXPECT_EQ(invalid_utf32_length, ConvertStringUTF32ToUTF16(orig_utf32, &utf16));
  EXPECT_EQ(invalid_utf8_length, ConvertStringUTF8ToUTF16(orig_utf8, &utf16));
  EXPECT_EQ(invalid_utf16_length, ConvertStringUTF16ToUTF8(orig_utf16, &utf8));
}

int main(int argc, char **argv) {
  testing::ParseGUnitFlags(&argc, argv);

  return RUN_ALL_TESTS();
}
