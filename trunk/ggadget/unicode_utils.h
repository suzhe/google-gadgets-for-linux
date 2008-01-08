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

#ifndef GGADGET_UNICODE_UTILS_H__
#define GGADGET_UNICODE_UTILS_H__

#include <string>
#include <ggadget/common.h>

namespace ggadget {
typedef uint16_t UTF16Char;
typedef uint32_t UTF32Char;

typedef std::basic_string<UTF16Char> UTF16String;
typedef std::basic_string<UTF32Char> UTF32String;

/**
 * Converts a utf8 char sequence to a utf32 char code.
 *
 * @param src the utf8 char sequence to be converted.
 * @param src_length length of the source utf8 char sequence.
 * @param[out] dest result utf32 char code.
 * @return how many bytes in src have been converted.
 */
size_t ConvertCharUTF8ToUTF32(const char *src, size_t src_length,
                              UTF32Char *dest);

/**
 * Converts a utf32 char code to a utf8 char sequence.
 *
 * @param src the utf32 char code be converted.
 * @param[out] dest buffer to store the result utf8 char sequence.
 * @param dest_length length of the dest utf8 char sequence buffer.
 * @return how many bytes have been written into dest.
 */
size_t ConvertCharUTF32ToUTF8(UTF32Char src, char *dest,
                              size_t dest_length);

/**
 * Converts a utf16 char sequence to a utf32 char code.
 *
 * @param src the utf16 char sequence to be converted.
 * @param src_length length of the source utf16 char sequence.
 * @param[out] dest result utf32 char code.
 * @return how many @c UTF16Char elements in src have been converted.
 */
size_t ConvertCharUTF16ToUTF32(const UTF16Char *src, size_t src_length,
                               UTF32Char *dest);

/**
 * Converts a utf32 char code to a utf16 char sequence.
 *
 * @param src the utf32 char code be converted.
 * @param[out] dest buffer to store the result utf16 char sequence.
 * @param dest_length length of the dest utf16 char sequence buffer.
 * @return how many @c UTF16Char elements have been written into dest.
 */
size_t ConvertCharUTF32ToUTF16(UTF32Char src, UTF16Char *dest,
                               size_t dest_length);

/**
 * Converts a utf8 string to a utf32 string.
 *
 * @param src the utf8 string to be converted.
 * @param src_length length of the source utf8 string.
 * @param[out] dest result utf32 string.
 * @return how many bytes in source utf8 string have been converted.
 */
size_t ConvertStringUTF8ToUTF32(const char *src, size_t src_length,
                                UTF32String *dest);

/**
 * Converts a utf8 string to a utf32 string.
 *
 * Same as above function but takes a std::string object as source.
 */
size_t ConvertStringUTF8ToUTF32(const std::string &src, UTF32String *dest);

/**
 * Converts a utf32 string to a utf8 string.
 *
 * @param src the utf32 string to be converted.
 * @param src_length length of the source utf32 string.
 * @param[out] dest result utf8 string.
 * @return how many chars in source utf32 string have been converted.
 */
size_t ConvertStringUTF32ToUTF8(const UTF32Char *src, size_t src_length,
                                std::string *dest);

/**
 * Converts a utf32 string to a utf8 string.
 *
 * Same as above function but takes a @c UTF32String object as source.
 */
size_t ConvertStringUTF32ToUTF8(const UTF32String &src, std::string *dest);

/**
 * Converts a utf8 string to a utf16 string.
 *
 * @param src the utf8 string to be converted.
 * @param src_length length of the source utf8 string.
 * @param[out] dest result utf16 string.
 * @return how many bytes in source utf8 string have been converted.
 */
size_t ConvertStringUTF8ToUTF16(const char *src, size_t src_length,
                                UTF16String *dest);

/**
 * Converts a utf8 string to a utf16 string.
 *
 * Same as above function but takes a std::string object as source.
 */
size_t ConvertStringUTF8ToUTF16(const std::string &src, UTF16String *dest);

/**
 * Converts a utf16 string to a utf8 string.
 *
 * @param src the utf16 string to be converted.
 * @param src_length length of the source utf16 string.
 * @param[out] dest result utf8 string.
 * @return how many @c UTF16Char elements in source utf16 string have been
 * converted.
 */
size_t ConvertStringUTF16ToUTF8(const UTF16Char *src, size_t src_length,
                                std::string *dest);

/**
 * Converts a utf16 string to a utf8 string.
 *
 * Same as above function but takes a @c UTF16String object as source.
 */
size_t ConvertStringUTF16ToUTF8(const UTF16String &src, std::string *dest);

/**
 * Converts a utf16 string to a utf32 string.
 *
 * @param src the utf16 string to be converted.
 * @param src_length length of the source utf16 string.
 * @param[out] dest result utf32 string.
 * @return how many @c UTF16Char elements in source utf16 string have been
 * converted.
 */
size_t ConvertStringUTF16ToUTF32(const UTF16Char *src, size_t src_length,
                                 UTF32String *dest);
/**
 * Converts a utf16 string to a utf32 string.
 *
 * Same as above function but takes a @c UTF16String object as source.
 */
size_t ConvertStringUTF16ToUTF32(const UTF16String &src, UTF32String *dest);

/**
 * Converts a utf32 string to a utf16 string.
 *
 * @param src the utf32 string to be converted.
 * @param src_length length of the source utf32 string.
 * @param[out] dest result utf16 string.
 * @return how many chars in source utf32 string have been converted.
 */
size_t ConvertStringUTF32ToUTF16(const UTF32Char *src, size_t src_length,
                                 UTF16String *dest);

/**
 * Converts a utf32 string to a utf16 string.
 *
 * Same as above function but takes a @c UTF32String object as source.
 */
size_t ConvertStringUTF32ToUTF16(const UTF32String &src, UTF16String *dest);

/**
 * Gets the length of a utf8 char sequence.
 *
 * @param src the source utf8 char sequence.
 * @return the length of a utf8 char sequence in bytes.
 */
size_t GetUTF8CharLength(const char *src);

/**
 * Checks if a utf8 char sequence valid.
 *
 * @param src the source utf8 char sequence to be checked.
 * @param length length of the source utf8 char sequence,
 *        which should be exactly equal to the length
 *        returned by @c GetUTF8CharLength function.
 * @return true if the source utf8 char sequence is valid.
 */
bool IsLegalUTF8Char(const char *src, size_t length);

/**
 * Gets the length of a utf16 char sequence.
 *
 * @param src the source utf16 char sequence.
 * @return the length of a utf16 char sequence in number of @c UTF16Char
 * elements.
 */
size_t GetUTF16CharLength(const UTF16Char *src);

/**
 * Checks if a utf16 char sequence valid.
 *
 * @param src the source utf16 char sequence to be checked.
 * @param length length of the source utf16 char sequence,
 *        which should be exactly equal to the length
 *        returned by @c GetUTF16CharLength function.
 * @return true if the source utf16 char sequence is valid.
 */
bool IsLegalUTF16Char(const UTF16Char *src, size_t length);

/**
 * Checks if a string is a valid UTF8 string.
 *
 * @param src the string to be checked.
 * @param length length of the source string.
 * @return true if the source string is a valid UTF8 string.
 */
bool IsLegalUTF8String(const char *src, size_t length);

/**
 * Checks if a string is a valid UTF8 string.
 *
 * Same as above function but takes a @c std::string object as source.
 */
bool IsLegalUTF8String(const std::string &src);

/**
 * Checks if a string is a valid UTF16 string.
 *
 * @param src the string to be checked.
 * @param length length of the source string.
 * @return true if the source string is a valid UTF16 string.
 */
bool IsLegalUTF16String(const UTF16Char *src, size_t length);

/**
 * Checks if a string is a valid UTF8 string.
 *
 * Same as above function but takes a @c std::string object as source.
 */
bool IsLegalUTF16String(const UTF16String &src);

const UTF32Char kUnicodeReplacementChar = 0xFFFD;
const UTF32Char kUnicodeMaxLegalChar = 0x10FFFF;
const UTF32Char kUnicodeMaxBMPChar = 0xFFFF;

/** Various BOMs. Note: they are not zero-ended strings, but char arrays. */
const char kUTF8BOM[] = { '\xEF', '\xBB', '\xBF' };
const char kUTF16LEBOM[] = { '\xFF', '\xFE' };
const char kUTF16BEBOM[] = { '\xFE', '\xFF' };
const char kUTF32LEBOM[] = { '\xFF', '\xFE', 0, 0 };
const char kUTF32BEBOM[] = { 0, 0, '\xFE', '\xFF' };

/**
 * Detects the encoding of a byte stream if it has BOM.
 * @param stream the input byte stream.
 * @param[out] encoding (optional) name of the encoding detected from BOM.
 *     Possible values are "UTF-8", "UTF-16LE", "UTF-16BE", "UTF-32LE"
 *     and "UTF-32BE".
 * @return @c true if the input stream has valid BOM.
 */
bool DetectEncodingFromBOM(const std::string &stream, std::string *encoding);

/**
 * Converts a byte stream into UTF8 if the stream has BOM.
 * @param stream the input byte stream.
 * @param[out] result the converted result.
 * @param[out] encoding (optional)the encoding detected from BOM.
 * @return the number of bytes successfully converted.
 */
size_t ConvertStreamToUTF8ByBOM(const std::string &stream, std::string *result,
                                std::string *encoding);

} // namespace ggadget

#endif // GGADGET_UNICODE_UTILS_H__
