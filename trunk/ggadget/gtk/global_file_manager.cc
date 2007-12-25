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
#include <ggadget/logger.h>
#include "global_file_manager.h"

namespace ggadget {
namespace gtk {

GlobalFileManager::GlobalFileManager() {
}

GlobalFileManager::~GlobalFileManager() {
}

bool GlobalFileManager::Init(const char *base_path) {
  if (!base_path || !*base_path) {
    return false;
  }
  base_path_ = base_path;
  return true;
}

bool GlobalFileManager::GetFileContents(const char *file,
                                        std::string *data, std::string *path) {
  ASSERT(data);
  data->clear();

  std::string filename = base_path_ + file;

  // TODO: check security?
  // Try to read from the file system.
  path->assign(filename);

  FILE *datafile = fopen(filename.c_str(), "r");
  if (!datafile) {
    LOG("Failed to open file: %s", filename.c_str());
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
    LOG("Error when reading file: %s", filename.c_str());
    data->clear();
    fclose(datafile);
    return false;
  }

  if (data->length() > kMaxFileSize) {
    LOG("File is too big (> %zu): %s", kMaxFileSize, filename.c_str());
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
  return (access(file, F_OK) == 0);
}

} // namespace gtk
} // namespace ggadget
