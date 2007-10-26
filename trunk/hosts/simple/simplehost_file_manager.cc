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

#include "ggadget/gadget_consts.h"
#include "third_party/unzip/unzip.h"
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

bool SimpleHostFileManager::GetFileContents(const char *file,
                                            std::string *data,
                                            std::string *path) {
  data->clear();

  int res_prefix_len = strlen(ggadget::kGlobalResourcePrefix);
  if (0 == strncmp(file, ggadget::kGlobalResourcePrefix, res_prefix_len)) {
    // This is a resource file, so look up resources array.
    const char *res_name = file + res_prefix_len;
    //DLOG("global fm lookup: %s, %s", file, res_name);
    Resource r = {res_name, 0, NULL};    
    const Resource *pos = std::lower_bound(
        kResourceList, kResourceList + arraysize(kResourceList), r,
        ResourceNameCompare);
    // This lookup should never fail since resource names are statically coded.
    ASSERT(pos && pos < kResourceList + arraysize(kResourceList) &&
        0 == strcmp(res_name, pos->filename));

    data->append(pos->data, pos->data_size);    
    *path = file;
    return true;
  }

  // TODO: add localized lookup support?

  return false;
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
