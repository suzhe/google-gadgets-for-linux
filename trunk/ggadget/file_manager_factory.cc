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

#include "common.h"
#include "logger.h"
#include "file_manager_factory.h"
#include "dir_file_manager.h"
#include "zip_file_manager.h"

namespace ggadget {

typedef FileManagerInterface *(*FileManagerFactory)(const char *, bool);

static FileManagerFactory g_factories_[] = {
  &ZipFileManager::Create,
  &DirFileManager::Create,
  NULL
};

static FileManagerInterface *g_global_file_manager = NULL;

FileManagerInterface *
CreateFileManager(const char *base_path) {
  ASSERT(base_path && *base_path);

  FileManagerInterface *fm;
  // Try load the base_path with a file manager.
  for (size_t i = 0; g_factories_[i]; ++i) {
    fm = g_factories_[i](base_path, false);
    if (fm) return fm;
  }

  return NULL;
}

bool SetGlobalFileManager(FileManagerInterface *manager) {
  ASSERT(!g_global_file_manager && manager);
  if (!g_global_file_manager && manager) {
    g_global_file_manager = manager;
    return true;
  }
  return false;
}

FileManagerInterface *GetGlobalFileManager() {
  EXPECT_M(g_global_file_manager,
           ("The global FileManager has not been set yet."));
  return g_global_file_manager;
}

} // namespace ggadget
