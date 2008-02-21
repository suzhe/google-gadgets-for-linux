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
#include <errno.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "system_utils.h"
#include "common.h"
#include "gadget_consts.h"
#include "logger.h"

namespace ggadget {

std::string BuildPath(const char *separator, const char *element, ...) {
  std::string result;
  va_list args;
  va_start(args, element);
  result = BuildPathV(separator, element, args);
  va_end(args);
  return result;
}

std::string BuildPathV(const char *separator, const char *element, va_list ap) {
  std::string result;

  if (!separator || !*separator)
    separator = kDirSeparatorStr;

  size_t sep_len = strlen(separator);

  while(element) {
    size_t elm_len = strlen(element);

    bool has_leading_sep = false;
    // Remove leading separators in element.
    while(elm_len >= sep_len && strncmp(element, separator, sep_len) == 0) {
      element += sep_len;
      elm_len -= sep_len;
      has_leading_sep = true;
    }

    // Remove trailing separators in element.
    while(elm_len >= sep_len &&
          strncmp(element + elm_len - sep_len, separator, sep_len) == 0) {
      elm_len -= sep_len;
    }

    // If the first element has leading separator, then means that the part
    // starts from root.
    if (!result.length() && has_leading_sep) {
      result.append(separator, sep_len);
    }

    // skip empty element.
    if (elm_len) {
      if (result.length() && (result.length() < sep_len ||
          strncmp(result.c_str() + result.length() - sep_len,
                  separator, sep_len) != 0)) {
        result.append(separator, sep_len);
      }
      result.append(element, elm_len);
    }

    element = va_arg(ap, const char *);
  }

  return result;
}

std::string BuildFilePath(const char *element, ...) {
  std::string result;
  va_list args;
  va_start(args, element);
  result = BuildPathV(kDirSeparatorStr, element, args);
  va_end(args);
  return result;
}

std::string BuildFilePathV(const char *element, va_list ap) {
  return BuildPathV(kDirSeparatorStr, element, ap);
}

bool SplitFilePath(const char *path, std::string *dir, std::string *filename) {
  if (!path || !*path) return false;

  if (dir) *dir = std::string();
  if (filename) *filename = std::string();

  size_t len = strlen(path);
  size_t sep_len = strlen(kDirSeparatorStr);

  // No dir part.
  if (len < sep_len) {
    if (filename) filename->assign(path, len);
    return false;
  }

  const char *last_sep = path + len - sep_len;
  bool has_sep = false;
  for (; last_sep != path; --last_sep) {
    if (strncmp(last_sep, kDirSeparatorStr, sep_len) == 0) {
      has_sep = true;
      break;
    }
  }

  if (has_sep) {
    // If the path refers to a file in root directory, then the root directory
    // will be returned.
    if (dir) {
      const char *first_sep = last_sep;
      for (; first_sep - sep_len >= path; first_sep -= sep_len) {
        if (strncmp(first_sep - sep_len, kDirSeparatorStr, sep_len) != 0)
          break;
      }
      dir->assign(path, (first_sep == path ? sep_len : first_sep - path));
    }
    last_sep += sep_len;
  }

  if (filename && *last_sep)
    filename->assign(last_sep);

  return has_sep && *last_sep;
}

bool EnsureDirectories(const char *path) {
  struct stat stat_value;
  bzero(&stat_value, sizeof(stat_value));
  if (stat(path, &stat_value) == 0) {
    if (S_ISDIR(stat_value.st_mode))
      return true;
    LOG("Path is not a directory: '%s'", path);
    return false;
  }
  if (errno != ENOENT) {
    LOG("Failed to access directory: '%s' error: %s", path, strerror(errno));
    return false;
  }

  std::string dir, file;
  SplitFilePath(path, &dir, &file);
  if (!dir.empty() && file.empty()) {
    // Deal with the case that the path has trailing '/'.
    std::string temp(dir);
    SplitFilePath(temp.c_str(), &dir, &file);
  }
  // dir will be empty if the input path is the upmost level of a relative path.
  if (!dir.empty() && !EnsureDirectories(dir.c_str()))
    return false;

  if (mkdir(path, 0700) == 0)
    return true;

  LOG("Failed to create directory: '%s' error: %s", path, strerror(errno));
  return false;
}

bool ReadFileContents(const char *path, std::string *content) {
  ASSERT(content);
  if (!path || !*path || !content)
    return false;

  content->clear();

  FILE *datafile = fopen(path, "r");
  if (!datafile) {
    LOG("Failed to open file: %s", path);
    return false;
  }

  // The approach below doesn't really work for large files, so we limit the
  // file size. A memory-mapped file scheme might be better here.
  const size_t kMaxFileSize = 20 * 1000 * 1000;
  const size_t kChunkSize = 8192;
  char buffer[kChunkSize];
  while (true) {
    size_t read_size = fread(buffer, 1, kChunkSize, datafile);
    content->append(buffer, read_size);
    if (content->length() > kMaxFileSize || read_size < kChunkSize)
      break;
  }

  if (ferror(datafile)) {
    LOG("Error when reading file: %s", path);
    content->clear();
    fclose(datafile);
    return false;
  }

  if (content->length() > kMaxFileSize) {
    LOG("File is too big (> %zu): %s", kMaxFileSize, path);
    content->clear();
    fclose(datafile);
    return false;
  }

  fclose(datafile);
  return true;
}

}  // namespace ggadget
