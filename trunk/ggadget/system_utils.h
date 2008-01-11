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

/**
 * This file includes common utils related to system level functionalities,
 * such as path, directory and file operations.
 */

#ifndef GGADGET_SYSTEM_UTILS_H__
#define GGADGET_SYSTEM_UTILS_H__

#include <cstdarg>
#include <string>
#include <stdint.h>
#include <ggadget/sysdeps.h>

namespace ggadget {

/**
 * Build a path using specified separator.
 *
 * @param separator The separator to be used, or if it's NULL, the system
 * default directory separator will be used.
 * @param element The first path element followed by one or more elements,
 * terminated by NULL.
 */
std::string BuildPath(const char *separator, const char *element, ...);

/**
 * Same as BuildPath, but accepting a va_list.
 */
std::string BuildPathV(const char *separator, const char *element, va_list ap);

/**
 * Build a file path using system default dir separator.
 * On Unix systems, it's identical to BuildPath(kDirSeparatorStr, ...);
 */
std::string BuildFilePath(const char *element, ...);

/**
 * Same as BuildFilePath, but accepting a va_list.
 */
std::string BuildFilePathV(const char *element, va_list ap);

/**
 * Split file path into directory and filename.
 *
 * @param[in] path The full file path to be splitted.
 * @param[out] dir The directory part of the specified path will be stored in
 * it, can be NULL to discard this part.
 * @param[out] filename The filename part of the specified path will be stored
 * in it, can be NULL to discard this part.
 * @return true if both directory and filename are available.
 */
bool SplitFilePath(const char *path, std::string *dir, std::string *filename);

} // namespace ggadget

#endif // GGADGET_SYSTEM_UTILS_H__
