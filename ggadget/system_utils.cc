/*
  Copyright 2008 Google Inc.

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

#include <dirent.h>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <locale.h>
#include <pwd.h>
#include <ggadget/file_manager_factory.h>
#include "system_utils.h"
#include "string_utils.h"
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
  if (!path || !*path) {
    LOG("Can't create empty path.");
    return false;
  }

  struct stat stat_value;
  memset(&stat_value, 0, sizeof(stat_value));
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
    //DLOG("Failed to open file: %s: %s", path, strerror(errno));
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
    LOG("Error when reading file: %s: %s", path, strerror(errno));
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

bool WriteFileContents(const char *path, const std::string &content) {
  if (!path || !*path)
    return false;

  FILE *out_fp = fopen(path, "w");
  if (!out_fp) {
    DLOG("Can't open file %s for writing: %s", path, strerror(errno));
    return false;
  }

  bool result = true;
  if (fwrite(content.c_str(), content.size(), 1, out_fp) != 1) {
    result = false;
    LOG("Error when writing to file %s: %s", path, strerror(errno));
  }
  // fclose() is placed first to ensure it's always called.
  result = (fclose(out_fp) == 0 && result);

  if (!result)
    unlink(path);
  return result;
}

std::string NormalizeFilePath(const char *path) {
  if (!path || !*path)
    return std::string("");

  std::string working_path(path);

  // Replace '\\' with '/' on non-Windows platforms.
#ifndef GGL_HOST_WINDOWS
  for (std::string::iterator it = working_path.begin();
       it != working_path.end(); ++it)
    if (*it == '\\') *it = kDirSeparator;
#endif

  // Remove all "." and ".." components, and consecutive '/'s.
  std::string result;
  size_t start = 0;

  while(start < working_path.length()) {
    size_t end = working_path.find(kDirSeparator, start);
    bool omit_part = false;
    if (end == std::string::npos)
      end = working_path.length();

    size_t part_length = end - start;
    switch (part_length) {
      case 0:
        // Omit consecutive '/'s.
        omit_part = true;
        break;
      case 1:
        // Omit part in /./
        omit_part = (working_path[start] == '.');
        break;
      case 2:
        // Omit part in /../, and remove the last part in result.
        if (working_path[start] == '.' && working_path[start + 1] == '.') {
          omit_part = true;
          size_t last_sep_pos = result.find_last_of(kDirSeparator);
          if (last_sep_pos == std::string::npos)
            // No separator in the result, remove all.
            result.clear();
          else
            result.erase(last_sep_pos);
        }
        break;
      default:
        break;
    }

    if (!omit_part) {
      if (result.length() || working_path[0] == kDirSeparator)
        result += kDirSeparator;
      result += working_path.substr(start, part_length);
    }

    start = end + 1;
  }

  // Handle special case: path is pointed to root.
  if (result.empty() && *path == kDirSeparator)
    result += kDirSeparator;

  return result;
}

std::string GetCurrentDirectory() {
  char buf[4096];
  if (::getcwd(buf, 1024) == buf) {
    // it's fit.
    return std::string(buf);
  } else {
    std::string result;
    size_t length = sizeof(buf);
    while(true) {
      length *= 2;
      char *tmp = new char[length];
      if (::getcwd(tmp, length) == tmp) {
        // it's fit.
        result = std::string(tmp);
        delete[] tmp;
        break;
      }
      delete[] tmp;
      // Other error occurred, stop trying.
      if (errno != ERANGE)
        break;
    }
    return result;
  }
}

std::string GetHomeDirectory() {
  const char * home = 0;
  struct passwd *pw;

  setpwent ();
  pw = getpwuid(getuid());
  endpwent ();

  if (pw)
    home = pw->pw_dir;

  if (!home)
    home = getenv("HOME");

  // If failed to get home directory, then use current directory.
  return home ? std::string(home) : GetCurrentDirectory();
}

std::string GetAbsolutePath(const char *path) {
  if (!path || !*path)
    return "";

  // Normalizes the file path.
  std::string result = path;
  // Not using kDirSeparator because Windows version should have more things
  // to do than simply replace the path separator.
  if (result[0] != '/') {
    result = GetCurrentDirectory() + "/" + result;
  }
  result = NormalizeFilePath(result.c_str());
  return result;
}

bool IsAbsolutePath(const char *path) {
  // Other system may use other method.
  return path && *path == '/';
}

bool CreateTempDirectory(const char *prefix, std::string *path) {
  ASSERT(path);
  bool result = false;
  size_t len = (prefix ? strlen(prefix) : 0) + 20;
  char *buf = new char[len];

#ifdef HAVE_MKDTEMP
  snprintf(buf, len, "/tmp/%s-XXXXXX", (prefix ? prefix : ""));

  result = (::mkdtemp(buf) == buf);
  if (result && path)
    *path = std::string(buf);
#else
  while(true) {
    snprintf(buf, len, "/tmp/%s-%06X", (prefix ? prefix : ""),
             ::rand() & 0xFFFFFF);
    if (::access(buf, F_OK) == 0)
      continue;

    if (errno == ENOENT) {
      // The temp name is available.
      result = (mkdir(buf, 0700) == 0);
      if (result && path)
        *path = std::string(buf);
    }

    break;
  }
#endif

  delete[] buf;
  return result;
}

bool RemoveDirectory(const char *path) {
  if (!path || !*path)
    return false;

  std::string dir_path = NormalizeFilePath(path);

  if (dir_path == kDirSeparatorStr) {
    DLOG("Can't remove the whole root directory.");
    return false;
  }

  DIR *pdir = opendir(dir_path.c_str());
  if (!pdir) {
    DLOG("Can't read directory: %s", path);
    return false;
  }

  struct dirent *pfile = NULL;
  while ((pfile = readdir(pdir)) != NULL) {
    if (strcmp(pfile->d_name, ".") != 0 &&
        strcmp(pfile->d_name, "..") != 0) {
      std::string file_path =
          BuildFilePath(dir_path.c_str(), pfile->d_name, NULL);
      struct stat file_stat;
      bool result = false;
      // Don't use dirent.d_type, it's a non-standard field.
      if (lstat(file_path.c_str(), &file_stat) == 0) {
        if (S_ISDIR(file_stat.st_mode))
          result = RemoveDirectory(file_path.c_str());
        else
          result = (::unlink(file_path.c_str()) == 0);
      }
      if (!result) {
        closedir(pdir);
        return false;
      }
    }
  }

  closedir(pdir);
  return ::rmdir(dir_path.c_str()) == 0;
}

bool GetSystemLocaleInfo(std::string *language, std::string *territory) {
  char *locale = setlocale(LC_MESSAGES, NULL);
  if (!locale || !*locale) return false;

  // We don't want to support these standard locales.
  if (strcmp(locale, "C") == 0 || strcmp(locale, "POSIX") == 0) {
    DLOG("Probably setlocale() was not called at beginning of the program.");
    return false;
  }

  std::string locale_str(locale);

  // Remove encoding and variant part.
  std::string::size_type pos = locale_str.find('.');
  if (pos != std::string::npos)
    locale_str.erase(pos);

  pos = locale_str.find('_');
  if (language)
    language->assign(locale_str, 0, pos);

  if (territory) {
    if (pos != std::string::npos)
      territory->assign(locale_str, pos + 1, std::string::npos);
    else
      territory->clear();
  }
  return true;
}

void Daemonize() {
  // FIXME: How about other systems?
#ifdef GGL_HOST_LINUX
  daemon(0, 0);
#endif
}

bool CopyFile(const char *src, const char *dest) {
  ASSERT(src && dest);
  if (!src || !dest)
    return false;

  FILE *in_fp = fopen(src, "r");
  if (!in_fp) {
    LOG("Can't open file %s for reading.", src);
    return false;
  }

  FILE *out_fp = fopen(dest, "w");
  if (!out_fp) {
    LOG("Can't open file %s for writing.", dest);
    fclose(in_fp);
    return false;
  }

  const size_t kChunkSize = 8192;
  char buffer[kChunkSize];
  bool result = true;
  while(true) {
    size_t read_size = fread(buffer, 1, kChunkSize, in_fp);
    if (read_size) {
      if (fwrite(buffer, read_size, 1, out_fp) != 1) {
        LOG("Error when writing to file %s", dest);
        result = false;
        break;
      }
    }
    if (read_size < kChunkSize)
      break;
  }

  if (ferror(in_fp)) {
    LOG("Error when reading file: %s", src);
    result = false;
  }

  fclose(in_fp);
  result = (fclose(out_fp) == 0 && result);

  if (!result)
    unlink(dest);

  return result;
}

std::string GetFullPathOfSystemCommand(const char *command) {
  if (IsAbsolutePath(command))
    return std::string(command);

  const char *env_path_value = getenv("PATH");
  if (env_path_value == NULL)
    return "";

  StringVector paths;
  SplitStringList(env_path_value, ":", &paths);
  for (StringVector::iterator i = paths.begin();
       i != paths.end(); ++i) {
    std::string path = BuildFilePath(i->c_str(), command, NULL);
    if (access(path.c_str(), X_OK) == 0)
      return path;
  }
  return "";
}

static std::string GetSystemGadgetPathInResourceDir(const char *resource_dir,
                                                    const char *basename) {
  std::string path;
  FileManagerInterface *file_manager = GetGlobalFileManager();
  path = BuildFilePath(resource_dir, basename, NULL) + kGadgetFileSuffix;
  if (file_manager->FileExists(path.c_str(), NULL) &&
      file_manager->IsDirectlyAccessible(path.c_str(), NULL))
    return file_manager->GetFullPath(path.c_str());

  path = BuildFilePath(resource_dir, basename, NULL);
  if (file_manager->FileExists(path.c_str(), NULL) &&
      file_manager->IsDirectlyAccessible(path.c_str(), NULL))
    return file_manager->GetFullPath(path.c_str());

  return std::string();
}

std::string GetSystemGadgetPath(const char *basename) {
  std::string result;
#ifdef _DEBUG
  // Try current directory first in debug mode, to ease in place build/debug.
  result = GetSystemGadgetPathInResourceDir(".", basename);
  if (!result.empty())
    return result;
#endif
#ifdef GGL_RESOURCE_DIR
  result = GetSystemGadgetPathInResourceDir(GGL_RESOURCE_DIR, basename);
#endif
  return result;
}

}  // namespace ggadget
