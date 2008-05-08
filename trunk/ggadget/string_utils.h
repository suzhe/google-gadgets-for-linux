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

#include <cstring>
#include <cstdarg>
#include <map>
#include <string>
#include <stdint.h>         // Integer types and macros.
#include <ggadget/common.h>
#include <ggadget/unicode_utils.h>

namespace ggadget {

/**
 * Use these functions to compare strings in gadget library code.
 * These strings include property names, file names, XML element and attribute
 * names, etc., but not include gadget element names.
 * Define @c GADGET_CASE_SENSITIVE to make the comparison case sensitive.
 * Don't define @c GADGET_CASE_SENSITIVE if compatibility with the Windows
 * version is required.
 */
int GadgetStrCmp(const char *s1, const char *s2);
int GadgetStrNCmp(const char *s1, const char *s2, size_t n);
int GadgetCharCmp(char c1, char c2);

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

/**
 * Default gadget string map, case sensitivity controled by
 * @c GADGET_CASE_SENSITIVE.
 */ 
typedef std::map<std::string, std::string,
                 GadgetStringComparator> GadgetStringMap;

/**
 * Case sensitive string map.
 */
typedef std::map<std::string, std::string> StringMap;

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
 * different.
 * @param source source string. If it is @c NULL, dest will be cleared to blank.
 * @param dest destination string.
 * @param comparator the comparator used to compare the strings, default is
 *     @c GadgetStrCmp().
 * @return @c true if assignment occurs.
 */
bool AssignIfDiffer(
    const char *source, std::string *dest,
    int (*comparator)(const char *s1, const char *s2) = GadgetStrCmp);

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
std::string StringVPrintf(const char *format, va_list ap);

/**
 * Append result to a supplied string
 */
void StringAppendPrintf(std::string* dst, const char* format, ...)
  PRINTF_ATTRIBUTE(2,3);
void StringAppendVPrintf(std::string* dst, const char* format, va_list ap);

/** URL-encode the source string. */
std::string EncodeURL(const std::string &source);

/** Returns whether the given character is valid in a URL. See RFC2396. */
bool IsValidURLChar(unsigned char c);

/** Returns whether the given string is a valid URL for a RSS feed. */
bool IsValidRSSURL(const char* url);

/** 
 * Returns whether the given string is a valid URL. 
 * Not a very complete check at the moment.
 */
bool IsValidURL(const char* url);

/**
 * Encode a string into a JavaScript string literal (without the begining and
 * ending quotes), by escaping special characters in the source string.
 */
std::string EncodeJavaScriptString(const UTF16Char *source);

/**
 * Splits a string into two parts. For convenience, the impl allows
 * @a result_left or @a result_right is the same object as @a source.
 *
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

/**
 * Compresses white spaces in a string using the rule like HTML formatting:
 *   - Removing leading and trailing white spaces;
 *   - Converting all consecutive white spaces into single spaces;
 * Only ASCII spaces (isspace(ch) == true) are handled.
 */
std::string CompressWhiteSpaces(const char *source);

/**
 * Removes tags and converts entities to their utf8 representation.
 * If everything is removed, this will return an empty string.
 * Whitespaces are compressed as in @c CompressWhiteSpaces().
 */
std::string ExtractTextFromHTML(const char *source);

/**
 * Guess if a string contains HTML. We basically check for common constructs
 * in html such as </, />, <br>, <hr>, <p>, etc.
 * This function only scans the 50k characters as an optimization.
 */
bool ContainsHTML(const char *s);

/**
 * Matches an XPath string against an XPath pattern, and returns whether
 * matches. This function only supports simple xpath grammar, containing only
 * element tag name, element index (not allowed in pattern) and attribute
 * names, no wildcard or other xpath expressions.
 *
 * @param xpath an xpath string returned as one of the keys in the result of
 *     XMLParserInterface::ParseXMLIntoXPathMap().
 * @param pattern an xpath pattern with only element tag names and (optional)
 *     attribute names.
 * @return true if the xpath matches the pattern. The matching rule is: first
 *     remove all [...]s in the xpath, and test if the result equals to the
 *     pattern.
 */
bool SimpleMatchXPath(const char *xpath, const char *pattern);

/**
 * Compares two versions.
 * @param version1 version string in "x.x.x.x" format where 'x' is an integer.
 * @param version2
 * @param[out] result on success: -1 if version1 < version2, 0 if
 *     version1 == version2, or 1 if version1 > version2.
 * @return @c false if @a version1 or @a version2 is invalid version string,
 *     or @c true on success.
 */
bool CompareVersion(const char *version1, const char *version2, int *result);

} // namespace ggadget

#endif // GGADGET_STRING_UTILS_H__
