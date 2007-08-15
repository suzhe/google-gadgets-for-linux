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

// This code is based on ConvertUTF.c and ConvertUTF.h released by Unicode, Inc.

/*
 * Copyright 2001-2004 Unicode, Inc.
 *
 * Disclaimer
 *
 * This source code is provided as is by Unicode, Inc. No claims are
 * made as to fitness for any particular purpose. No warranties of any
 * kind are expressed or implied. The recipient agrees to determine
 * applicability of information provided. If this file has been
 * purchased on magnetic or optical media from Unicode, Inc., the
 * sole remedy for any claim will be exchange of defective media
 * within 90 days of receipt.
 *
 * Limitations on Rights to Redistribute This Code
 *
 * Unicode, Inc. hereby grants the right to freely use the information
 * supplied in this file in the creation of products supporting the
 * Unicode Standard, and to make copies of this file in any form
 * for internal or external distribution as long as this notice
 * remains attached.
 */

#include "unicode_utils.h"

namespace ggadget {

static const int kHalfShift = 10;
static const UTF32Char kHalfBase = 0x0010000UL;
static const UTF32Char kHalfMask = 0x3FFUL;

static const UTF32Char kSurrogateHighStart = 0xD800;
static const UTF32Char kSurrogateHighEnd = 0xDBFF;
static const UTF32Char kSurrogateLowStart = 0xDC00;
static const UTF32Char kSurrogateLowEnd = 0xDFFF;

static const UTF8Char kTrailingBytesForUTF8[256] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

static const UTF32Char kOffsetsFromUTF8[6] = {
  0x00000000UL, 0x00003080UL, 0x000E2080UL,
  0x03C82080UL, 0xFA082080UL, 0x82082080UL
};

static const UTF8Char kFirstByteMark[7] = {
  0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC
};

size_t ConvertCharUTF8ToUTF32(const UTF8Char *src, size_t src_length,
                              UTF32Char *dest) {
  if (!src || !*src || !src_length || !dest) {
    return 0;
  }

  size_t extra_bytes = kTrailingBytesForUTF8[*src];
  if (extra_bytes >= src_length || !IsLegalUTF8Char(src, extra_bytes + 1)) {
    *dest = 0;
    return 0;
  }

  UTF32Char result = 0;
  // The cases all fall through.
  switch (extra_bytes) {
    case 5: result += *src++; result <<= 6;
    case 4: result += *src++; result <<= 6;
    case 3: result += *src++; result <<= 6;
    case 2: result += *src++; result <<= 6;
    case 1: result += *src++; result <<= 6;
    case 0: result += *src++;
  }
  result -= kOffsetsFromUTF8[extra_bytes];
  if (result > kUnicodeMaxLegalChar ||
      (result >= kSurrogateHighStart && result <= kSurrogateLowEnd))
    result = kUnicodeReplacementChar;
  *dest = result;
  return extra_bytes + 1;
}

size_t ConvertCharUTF32ToUTF8(UTF32Char src, UTF8Char *dest,
                              size_t dest_length) {
  const UTF32Char kByteMask = 0xBF;
  const UTF32Char kByteMark = 0x80;

  if (src > kUnicodeMaxLegalChar ||
      (src >= kSurrogateHighStart && src <= kSurrogateLowEnd) ||
      !dest || !dest_length)
    return 0;

  size_t bytes_to_write = 0;
  if (src < 0x80)
    bytes_to_write = 1;
  else if (src < 0x800)
    bytes_to_write = 2;
  else if (src < 0x10000)
    bytes_to_write = 3;
  else
    bytes_to_write = 4;

  if (bytes_to_write > dest_length)
    return 0;

  dest += bytes_to_write;
  // Everything falls through.
  switch (bytes_to_write) {
    case 4:
      *--dest = static_cast<UTF8Char>((src | kByteMark) & kByteMask); src >>=6;
    case 3:
      *--dest = static_cast<UTF8Char>((src | kByteMark) & kByteMask); src >>=6;
    case 2:
      *--dest = static_cast<UTF8Char>((src | kByteMark) & kByteMask); src >>=6;
    case 1:
      *--dest = static_cast<UTF8Char>(src | kFirstByteMark[bytes_to_write]);
  }

  return bytes_to_write;
}

size_t ConvertCharUTF16ToUTF32(const UTF16Char *src, size_t src_length,
                               UTF32Char *dest) {
  if (!src || !*src || !src_length || !dest) {
    return 0;
  }

  UTF32Char high;
  high = *src;
  if (high >= kSurrogateHighStart && high <= kSurrogateHighEnd) {
    if (src_length > 1) {
      UTF32Char low = *(src+1);
      if (low >= kSurrogateLowStart && low <= kSurrogateLowEnd) {
        *dest = ((high - kSurrogateHighStart) << kHalfShift) +
                (low - kSurrogateLowStart) + kHalfBase;
        return 2;
      }
    }
    return 0;
  } else if (high >= kSurrogateLowStart && high <= kSurrogateLowEnd) {
    return 0;
  }
  *dest = high;
  return 1;
}

size_t ConvertCharUTF32ToUTF16(UTF32Char src, UTF16Char *dest,
                               size_t dest_length) {
  if (!dest || !dest_length)
    return 0;

  if (src <= kUnicodeMaxBMPChar) {
    if (src >= kSurrogateHighStart && src <= kSurrogateLowEnd)
      return 0;
    *dest = static_cast<UTF16Char>(src);
    return 1;
  } else if (src <= kUnicodeMaxLegalChar && dest_length > 1) {
    src -= kHalfBase;
    *dest++ = static_cast<UTF16Char>((src >> kHalfShift) + kSurrogateHighStart);
    *dest = static_cast<UTF16Char>((src & kHalfMask) + kSurrogateLowStart);
    return 2;
  }
  return 0;
}

size_t ConvertStringUTF8ToUTF32(const UTF8Char *src, size_t src_length,
                                UTF32String *dest) {
  if (!src || !src_length || !dest)
    return 0;

  dest->clear();
  size_t used_length = 0;
  size_t utf8_len;
  UTF32Char utf32;
  while (src_length) {
    utf8_len = ConvertCharUTF8ToUTF32(src, src_length, &utf32);
    if (!utf8_len) break;
    dest->push_back(utf32);
    used_length += utf8_len;
    src += utf8_len;
    src_length -= utf8_len;
  }
  return used_length;
}

size_t ConvertStringUTF8ToUTF32(const std::string &src, UTF32String *dest) {
  return
      ConvertStringUTF8ToUTF32(reinterpret_cast<const UTF8Char*>(src.c_str()),
                               src.length(), dest);
}

size_t ConvertStringUTF32ToUTF8(const UTF32Char *src, size_t src_length,
                                std::string *dest) {
  if (!src || !src_length || !dest)
    return 0;

  dest->clear();
  size_t used_length = 0;
  size_t utf8_len;
  UTF8Char utf8[6];
  while (src_length) {
    utf8_len = ConvertCharUTF32ToUTF8(*src, utf8, 6);
    if (!utf8_len) break;
    dest->append(reinterpret_cast<std::string::value_type*>(utf8), utf8_len);
    ++used_length;
    ++src;
    --src_length;
  }
  return used_length;
}

size_t ConvertStringUTF32ToUTF8(const UTF32String &src, std::string *dest) {
  return ConvertStringUTF32ToUTF8(src.c_str(), src.length(), dest);
}

size_t ConvertStringUTF8ToUTF16(const UTF8Char *src, size_t src_length,
                                UTF16String *dest) {
  if (!src || !src_length || !dest)
    return 0;

  dest->clear();
  size_t used_length = 0;
  size_t utf8_len;
  size_t utf16_len;
  UTF16Char utf16[2];
  UTF32Char utf32;
  while (src_length) {
    utf8_len = ConvertCharUTF8ToUTF32(src, src_length, &utf32);
    if (!utf8_len) break;
    utf16_len = ConvertCharUTF32ToUTF16(utf32, utf16, 2);
    if (!utf16_len) break;
    dest->append(utf16, utf16_len);
    used_length += utf8_len;
    src += utf8_len;
    src_length -= utf8_len;
  }
  return used_length;
}

size_t ConvertStringUTF8ToUTF16(const std::string &src, UTF16String *dest) {
  return
      ConvertStringUTF8ToUTF16(reinterpret_cast<const UTF8Char*>(src.c_str()),
                               src.length(), dest);
}

size_t ConvertStringUTF16ToUTF8(const UTF16Char *src, size_t src_length,
                                std::string *dest) {
  if (!src || !src_length || !dest)
    return 0;

  dest->clear();
  size_t used_length = 0;
  size_t utf8_len;
  size_t utf16_len;
  UTF8Char utf8[6];
  UTF32Char utf32;
  while (src_length) {
    utf16_len = ConvertCharUTF16ToUTF32(src, src_length, &utf32);
    if (!utf16_len) break;
    utf8_len = ConvertCharUTF32ToUTF8(utf32, utf8, 6);
    if (!utf8_len) break;
    dest->append(reinterpret_cast<std::string::value_type*>(utf8), utf8_len);
    used_length += utf16_len;
    src += utf16_len;
    src_length -= utf16_len;
  }
  return used_length;
}

size_t ConvertStringUTF16ToUTF8(const UTF16String &src, std::string *dest) {
  return ConvertStringUTF16ToUTF8(src.c_str(), src.length(), dest);
}

size_t ConvertStringUTF16ToUTF32(const UTF16Char *src, size_t src_length,
                                 UTF32String *dest) {
  if (!src || !src_length || !dest)
    return 0;

  dest->clear();
  size_t used_length = 0;
  size_t utf16_len;
  UTF32Char utf32;
  while (src_length) {
    utf16_len = ConvertCharUTF16ToUTF32(src, src_length, &utf32);
    if (!utf16_len) break;
    dest->push_back(utf32);
    used_length += utf16_len;
    src += utf16_len;
    src_length -= utf16_len;
  }
  return used_length;
}

size_t ConvertStringUTF16ToUTF32(const UTF16String &src, UTF32String *dest) {
  return ConvertStringUTF16ToUTF32(src.c_str(), src.length(), dest);
}

size_t ConvertStringUTF32ToUTF16(const UTF32Char *src, size_t src_length,
                                 UTF16String *dest) {
  if (!src || !src_length || !dest)
    return 0;

  dest->clear();
  size_t used_length = 0;
  size_t utf16_len;
  UTF16Char utf16[2];
  while (src_length) {
    utf16_len = ConvertCharUTF32ToUTF16(*src, utf16, 2);
    if (!utf16_len) break;
    dest->append(utf16, utf16_len);
    ++used_length;
    ++src;
    --src_length;
  }
  return used_length;
}

size_t ConvertStringUTF32ToUTF16(const UTF32String &src, UTF16String *dest) {
  return ConvertStringUTF32ToUTF16(src.c_str(), src.length(), dest);
}

size_t GetUTF8CharLength(const UTF8Char *src) {
  return src ? (kTrailingBytesForUTF8[*src] + 1) : 0;
}

bool IsLegalUTF8Char(const UTF8Char *src, size_t length) {
  if (!src || !length) return false;

  UTF8Char a;
  const UTF8Char *srcptr = src+length;
  switch (length) {
    default: return false;
    // Everything else falls through when "true"...
    case 4: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
    case 3: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
    case 2: if ((a = (*--srcptr)) > 0xBF) return false;
      switch (*src) {
        // No fall-through in this inner switch
        case 0xE0: if (a < 0xA0) return false; break;
        case 0xED: if (a > 0x9F) return false; break;
        case 0xF0: if (a < 0x90) return false; break;
        case 0xF4: if (a > 0x8F) return false; break;
        default:   if (a < 0x80) return false;
      }
    case 1: if (*src >= 0x80 && *src < 0xC2) return false;
  }
  if (*src > 0xF4) return false;
  return true;
}

size_t GetUTF16CharLength(const UTF16Char *src) {
  if (!src) return 0;
  if (*src < kSurrogateHighStart || *src > kSurrogateLowEnd)
    return 1;
  if (*src <= kSurrogateHighEnd &&
      *(src+1) >= kSurrogateLowStart && *(src+1) <= kSurrogateLowEnd)
    return 2;
  return 0;
}

bool IsLegalUTF16Char(const UTF16Char *src, size_t length) {
  return length && length == GetUTF16CharLength(src);
}


} // namespace ggadget
