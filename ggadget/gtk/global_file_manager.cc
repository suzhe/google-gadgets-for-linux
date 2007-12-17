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

#include <locale.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <cstring>

#include <ggadget/gadget_consts.h>
#include "global_file_manager.h"

#if 0
#include <hosts/simple/resources/resources.cc> // temp
#else
struct Resource {
  const char *filename;
  size_t data_size;
  const char *data;
};

static const Resource kResourceList[] = {
  {"invalid", 0, NULL}
};
#endif

namespace ggadget {
namespace gtk {

static const char kResourceZipName[] = "ggl_resources.bin";

GlobalFileManager::GlobalFileManager() {

}

GlobalFileManager::~GlobalFileManager() {

}

bool GlobalFileManager::InitLocaleStrings() {
  char *locale = setlocale(LC_MESSAGES, NULL);
  if (!locale)
    return false;

  locale_prefix_ = locale;
  std::string::size_type pos = locale_prefix_.find('.');
  if (pos != std::string::npos)
    // Remove '.' and everything after.
    locale_prefix_.erase(pos);

  locale_lang_prefix_ = locale_prefix_;
  pos = locale_lang_prefix_.find('_');
  if (pos != std::string::npos)
    locale_lang_prefix_.erase(pos);

  locale_lang_prefix_ += kPathSeparator;
  locale_prefix_ += kPathSeparator;

  return true;
}

bool GlobalFileManager::Init(const char *base_path) {
  ASSERT(!base_path);

  if (!InitLocaleStrings()) {
    return false;
  }

  // Initialize resource file. Try several directories for the file.
  std::string filename;
  unzFile zip;

  // TODO: try other dirs first

  // Try current directory.
  filename = kResourceZipName;
  zip = unzOpen(filename.c_str());
  if (zip) {
    unzClose(zip);
    goto exit;
  }

exit:
  res_zip_path_ = filename;
  if (res_zip_path_.empty()) {
    LOG("Failed to open resource file.");
    return false;
  }

  return true;
}

static inline bool ResourceNameCompare(const Resource &r1,
                                       const Resource &r2) {
  return strcmp(r1.filename, r2.filename) < 0;
}

static const Resource *FindResource(const char *res_name) {
  //DLOG("global fm lookup: %s, %s", file, res_name);
  Resource r = {res_name, 0, NULL};
  const Resource *pos = std::lower_bound(
      kResourceList, kResourceList + arraysize(kResourceList), r,
      ResourceNameCompare);
  if (pos && pos < kResourceList + arraysize(kResourceList) &&
      0 == strcmp(res_name, pos->filename)) {
    return pos;
  }
  return NULL;
}

bool GlobalFileManager::SeekToFile(unzFile zip, const char *file,
                                   std::string *path) {
  ASSERT(path);
  path->clear();

  bool status = true;
  std::string filename = file;
  // Use case-sensitive comparisons.
  if (unzLocateFile(zip, filename.c_str(), 1) == UNZ_OK) {
    goto exit;
  }

  // Try lang_TERRITORY/file, e.g. zh_CN/myfile.
  filename = locale_prefix_ + file;
  if (unzLocateFile(zip, filename.c_str(), 1) == UNZ_OK) {
    goto exit;
  }

  // Try lang/file, e.g. zh/myfile.
  filename = locale_lang_prefix_ + file;
  if (unzLocateFile(zip, filename.c_str(), 1) == UNZ_OK) {
    goto exit;
  }

  // Try default en_US and en locale.
  filename = std::string("en_US/") + file;
  if (unzLocateFile(zip, filename.c_str(), 1) == UNZ_OK) {
    goto exit;
  }

  filename = std::string("en/") + file;
  if (unzLocateFile(zip, filename.c_str(), 1) == UNZ_OK) {
    goto exit;
  }

  // No reason to support Windows locale IDs in this lookup.
  status = false;

exit:
  if (status) {
    *path = filename;
  }

  return status;
}

bool GlobalFileManager::GetZipFileContents(const char *filename,
                                           std::string *data,
                                           std::string *path) {
  unzFile zip = unzOpen(res_zip_path_.c_str());
  if (!zip) {
    LOG("Failed to open resource file: %s", res_zip_path_.c_str());
    return false;
  }

  bool status;
  status = SeekToFile(zip, filename, path);
  if (!status) {
    LOG("Unable to locate file: %s in resource file: %s",
        filename, res_zip_path_.c_str());
  }

  if (unzOpenCurrentFile(zip) == UNZ_OK) {
    const int kChunkSize = 2048;
    char buffer[kChunkSize];
    while (true) {
      int read_size = unzReadCurrentFile(zip, buffer, kChunkSize);
      if (read_size > 0)
        data->append(buffer, read_size);
      else if (read_size < 0) {
        LOG("Error reading file: %s in resource file: %s",
            filename, res_zip_path_.c_str());
        data->clear();
        status = false;
        break;
      } else {
        break;
      }
    }
  } else {
    LOG("Failed to open file: %s in resource file: %s",
        filename, res_zip_path_.c_str());
    status = false;
  }

  if (unzCloseCurrentFile(zip) != UNZ_OK) {
    LOG("CRC error in file: %s in resource file: %s",
        filename, res_zip_path_.c_str());
    data->clear();
    status = false;
  }
  unzClose(zip);
  return status;
}

bool GlobalFileManager::GetFileContents(const char *file,
                                        std::string *data, std::string *path) {
  ASSERT(data);
  data->clear();

  int res_prefix_len = strlen(ggadget::kGlobalResourcePrefix);
  if (0 == strncmp(file, ggadget::kGlobalResourcePrefix, res_prefix_len)) {
    // This is a resource file. First check compiled resources.
    const char *res_name = file + res_prefix_len;
    const Resource *pos = FindResource(res_name);
    if (pos) {
      data->append(pos->data, pos->data_size);
      *path = file;
      return true;
    }

    // Not compiled, check associated resource zip file.
    bool result = GetZipFileContents(res_name, data, path);
    if (result) {
      // The returned path starts with the resource prefix.
      *path = ggadget::kGlobalResourcePrefix + *path;
      return true;
    }

    // TODO: check dir?

    return false;
  }

  // TODO: check security?
  // Not a resource, try to read from the file system.
  path->assign(file);

  FILE *datafile = fopen(file, "r");
  if (!datafile) {
    LOG("Failed to open file: %s", file);
    return false;
  }

  const size_t kChunkSize = 2048;
  const size_t kMaxFileSize = 4 * 1024 * 1024;
  char buffer[kChunkSize];
  while (true) {
    size_t read_size = fread(buffer, 1, kChunkSize, datafile);
    data->append(buffer, read_size);
    if (data->length() > kMaxFileSize || read_size < kChunkSize)
      break;
  }

  if (ferror(datafile)) {
    LOG("Error when reading file: %s", file);
    data->clear();
    fclose(datafile);
    return false;
  }

  if (data->length() > kMaxFileSize) {
    LOG("File is too big (> %zu): %s", kMaxFileSize, file);
    data->clear();
    fclose(datafile);
    return false;
  }

  fclose(datafile);
  return true;
}

bool GlobalFileManager::GetXMLFileContents(const char *file,
                                               std::string *data,
                                               std::string *path) {
  return false; // not implemented
}

bool GlobalFileManager::ExtractFile(const char *file,
                                        std::string *into_file) {
  return false; // not implemented
}

ggadget::GadgetStringMap *GlobalFileManager::GetStringTable() {
  return NULL; // not implemented
}

bool GlobalFileManager::FileExists(const char *file) {
  if (FindResource(file))
    return true;
  return (access(file, F_OK) == 0);
}

} // namespace gtk
} // namespace ggadget
