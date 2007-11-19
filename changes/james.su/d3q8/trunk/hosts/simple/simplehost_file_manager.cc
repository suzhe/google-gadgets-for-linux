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
#include <string.h>

#include <ggadget/gadget_consts.h>
#include <third_party/unzip/unzip.h>
#include "simplehost_file_manager.h"
#include "resources/resources.cc"

SimpleHostFileManager::SimpleHostFileManager() {

}

SimpleHostFileManager::~SimpleHostFileManager() {

}

bool SimpleHostFileManager::Init(const char *base_path) {
  ASSERT(!base_path);
  return true;
}

static inline bool ResourceNameCompare(const Resource &r1,
                                       const Resource &r2) {
  return strcmp(r1.filename, r2.filename) < 0;
}

static const Resource *FindResource(const char *file) {
  int res_prefix_len = strlen(ggadget::kGlobalResourcePrefix);
  if (0 == strncmp(file, ggadget::kGlobalResourcePrefix, res_prefix_len)) {
    // This is a resource file, so look up resources array.
    const char *res_name = file + res_prefix_len;
    //DLOG("global fm lookup: %s, %s", file, res_name);
    Resource r = {res_name, 0, NULL};
    const Resource *pos = std::lower_bound(
        kResourceList, kResourceList + arraysize(kResourceList), r,
        ResourceNameCompare);
    if (pos && pos < kResourceList + arraysize(kResourceList) &&
        0 == strcmp(res_name, pos->filename)) {
      return pos;
    }
  }
  return NULL;
}

bool SimpleHostFileManager::GetFileContents(const char *file,
                                            std::string *data,
                                            std::string *path) {
  ASSERT(data);
  data->clear();
  const Resource *pos = FindResource(file);
  if (pos) {
    data->append(pos->data, pos->data_size);    
    *path = file;
    return true;
  }

  // TODO: check security?
  // Try to read from the file system.
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

bool SimpleHostFileManager::GetXMLFileContents(const char *file,
                                               std::string *data,
                                               std::string *path) {
  return false; // not implemented
}

bool SimpleHostFileManager::ExtractFile(const char *file, 
                                        std::string *into_file) {
  return false; // not implemented
}

ggadget::GadgetStringMap *SimpleHostFileManager::GetStringTable() {
  return NULL; // not implemented
}

bool SimpleHostFileManager::FileExists(const char *file) {
  if (FindResource(file))
    return true;
  return (access(file, F_OK) == 0);
}
