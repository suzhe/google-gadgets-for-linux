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

#ifndef GGADGET_STRING_UTILS_H__
#define GGADGET_STRING_UTILS_H__

#include <map>
#include <string>
#include <stdint.h>         // Integer types and macros.
#include <ggadget/common.h>
#include <ggadget/unicode_utils.h>

namespace ggadget {

/**
 * Use this function to compare strings in gadget library code.
 * These strings include property names, file names, XML element and attribute
 * names, etc., but not include gadget element names.
 * Define @c GADGET_CASE_SENSITIVE to make the comparison case sensitive.
 * Don't define @c GADGET_CASE_SENSITIVE if compatibility with the Windows
 * version is required.
 */
int GadgetStrCmp(const char *s1, const char *s2);

/**
 * A comparison functor for <code>const char *</code> parameters.
 * It's useful when using <code>const char *</code> as the key of a map.
 */
struct GadgetCharPtrComparator {
  bool operator()(const char* s1, const char* s2) const {
    return GadgetStrCmp(s1, s2) < 0;
  }
};

struct GadgetStringComparator {
  bool operator()(const std::string &s1, const std::string &s2) const {
    return GadgetStrCmp(s1.c_str(), s2.c_str()) < 0;
  }
};

typedef std::map<std::string, std::string,
                 GadgetStringComparator> GadgetStringMap;

struct CaseInsensitiveCharPtrComparator {
  bool operator()(const char* s1, const char* s2) const {
    return strcasecmp(s1, s2) < 0;
  }
};

struct CaseInsensitiveStringComparator {
  bool operator()(const std::string &s1, const std::string &s2) const {
    return strcasecmp(s1.c_str(), s2.c_str()) < 0;
  }
};

typedef std::map<std::string, std::string,
                 CaseInsensitiveStringComparator> CaseInsensitiveStringMap;

/**
 * Assigns a <code>const char *</code> string to a std::string if they are
 * different (compared using GadgetStrCmp).
 * @param source source string. If it is @c NULL, dest will be cleared to blank.
 * @param dest destination string.
 * @return @c true if assignment occurs.
 */
bool AssignIfDiffer(const char *source, std::string *dest);

/**
 * Removes the starting and ending spaces from a string.
 */
std::string TrimString(const std::string &s);

/**
 * Converts a string into lowercase.
 * Note: Only converts ASCII chars.
 */
std::string ToLower(const std::string &s);

/**
 * Converts a string into uppercase.
 * Note: Only converts ASCII chars.
 */
std::string ToUpper(const std::string &s);

/**
 * Format data into a C++ string.
 */
std::string StringPrintf(const char *format, ...)
  // Tell the compiler to do printf format string checking.
  PRINTF_ATTRIBUTE(1,2);

/** URL-encode the source string. */
std::string EncodeURL(const std::string &source);

/** Returns whether the given character is valid in a URL. See RFC2396. */
bool IsValidURLChar(unsigned char c);

/**
 * Encode a string into a JavaScript string literal (without the begining and
 * ending quotes), by escaping special characters in the source string.
 */
std::string EncodeJavaScriptString(const UTF16Char *source);

/**
 * Splits a string into two parts. 
 * @param source the source string to split.
 * @param separator the source string will be splitted at the first occurance
 *     of the separator.
 * @param[out] result_left the part before the separator. If separator is not
 *     found, result_left will be set to source. Can be @c NULL if the caller
 *     doesn't need it.
 * @param[out] result_right the part after the separator. If seperator is not
 *     found, result_right will be cleared. Can be @c NULL if the caller doesn't
 *     need it.
 * @return @c true if separator found.
 */
bool SplitString(const std::string &source, const std::string &separator,
                 std::string *result_left, std::string *result_right);

} // namespace ggadget

#endif // GGADGET_STRING_UTILS_H__
