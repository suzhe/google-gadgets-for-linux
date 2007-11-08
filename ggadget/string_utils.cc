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

#include <cstring>
#include "string_utils.h"
#include "common.h"

namespace ggadget {

static const char kSlash      = '/';
static const char kBackSlash = '\\';

int GadgetStrCmp(const char *s1, const char *s2) {
#ifdef GADGET_CASE_SENSITIVE
  return strcmp(s1, s2);
#else
  return strcasecmp(s1, s2);
#endif
}

bool AssignIfDiffer(const char *source, std::string *dest) {
  ASSERT(dest);
  bool changed = false;
  if (source && source[0]) {
    if (GadgetStrCmp(source, dest->c_str()) != 0) {
      changed = true;
      *dest = source;
    }
  } else if (!dest->empty()){
    changed = true;
    dest->clear();
  }
  return changed;
}

std::string TrimString(const std::string &s) {
  std::string::size_type start = s.find_first_not_of(" \t\r\n");
  std::string::size_type end = s.find_last_not_of(" \t\r\n");
  if (start == std::string::npos)
    return std::string("");

  ASSERT(end != std::string::npos);
  return std::string(s, start, end - start + 1);
}

std::string ToLower(const std::string &s) {
  std::string result(s);
  std::transform(result.begin(), result.end(), result.begin(), ::tolower);
  return result;
}

std::string ToUpper(const std::string &s) {
  std::string result(s);
  std::transform(result.begin(), result.end(), result.begin(), ::toupper);
  return result;
}

std::string StringPrintf(const char* format, ...) {
  std::string dst;
  // First try with a small fixed size buffer
  char space[1024];

  va_list ap;
  va_start(ap, format);
  // It's possible for methods that use a va_list to invalidate
  // the data in it upon use.  The fix is to make a copy
  // of the structure before using it and use that copy instead.
  va_list backup_ap;
  va_copy(backup_ap, ap);

  int result = vsnprintf(space, sizeof(space), format, backup_ap);
  va_end(backup_ap);

  if (result >= 0 && result < static_cast<int>(sizeof(space))) {
    // It fits.
    dst = space;
  } else {
    // Repeatedly increase buffer size until it fits
    int length = sizeof(space);
    while (true) {
      if (result < 0) {
        // Older behavior: just try doubling the buffer size.
        length *= 2;
      } else {
        // We need exactly result + 1 characters.
        length = result + 1;
      }
      char* buf = new char[length];

      // Restore the va_list before we use it again
      va_copy(backup_ap, ap);
      result = vsnprintf(buf, length, format, backup_ap);
      va_end(backup_ap);

      if ((result >= 0) && (result < length)) {
        // It fits.
        dst = buf;
        delete[] buf;
        break;
      }
      delete[] buf;
    }
  }
  va_end(ap);
  return dst;
}

bool IsValidURLChar(unsigned char c) {
  // See RFC 2396 for information: http://www.ietf.org/rfc/rfc2396.txt
  // check for INVALID character (in US-ASCII: 0-127) and consider all
  // others valid
  return !(c <= ' ' || '<'==c || '>'==c || '\"'==c || '{'==c || '}'==c ||
          // '|'==c ||     // Technically | is unadvised, but it is valid, and some URLs use it
          // '^'==c ||     // Also technically invalid but Yahoo news use it... others too
          // '`'==c ||     // Yahoo uses this
          kBackSlash==c || '['==c || ']'==c
          // Comparison below is always false for char:
          || c >= 128);  // Enable converting non-ascii chars
}

void EncodeURL(const std::string &source, std::string *dest) {
  ASSERT(dest);

  dest->clear();

  for (size_t c = 0; c < source.length(); c++) {
    unsigned char src = source[c];

    if (src == kBackSlash) {
      dest->append(1, kSlash);
      continue;
    }

    // %-encode disallowed URL chars (b/w 0-127)
    // (chars >=128 are considered valid... although technically they're not)
    if (!IsValidURLChar(src)) {
      // output the percent, followed by the hex value of the character
      // Note: we know it's a char in US-ASCII (0-127)
      //
      dest->append(1, '%');

      static const char kHexChars[] = "0123456789abcdef";
      dest->append(1, kHexChars[src >> 4]);
      dest->append(1, kHexChars[src & 0xF]);
    } else {
      // not a special char: just copy
      dest->append(1, src);
    }
  }
}

}  // namespace ggadget
