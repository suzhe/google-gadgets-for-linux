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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>

#include "global_file_manager.h"
#include "gadget_consts.h"
#include "system_utils.h"

namespace ggadget {

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
  std::string filename = BuildFilePath(base_path_.c_str(), file);
  if (path)
    path->assign(filename);
  return ReadFileContents(filename.c_str(), data);
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

const GadgetStringMap *GlobalFileManager::GetStringTable() const {
  return NULL; // not implemented
}

bool GlobalFileManager::FileExists(const char *file, std::string *path) {
  std::string filename = BuildFilePath(base_path_.c_str(), file);
  if (path)
    path->assign(filename);
  return (access(filename.c_str(), F_OK) == 0);
}

} // namespace ggadget
