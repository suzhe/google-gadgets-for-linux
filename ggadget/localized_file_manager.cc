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
#include <vector>
#include <string>
#include <cstdio>

#include "common.h"
#include "logger.h"
#include "localized_file_manager.h"
#include "windows_locales.h"
#include "gadget_consts.h"
#include "system_utils.h"

namespace ggadget {

class LocalizedFileManager::Impl {
 public:
  Impl(FileManagerInterface *file_manager)
    : file_manager_(file_manager) {
    std::string language, territory;
    if (GetSystemLocaleInfo(&language, &territory) && language.length()) {
      std::string locale = language;
      if (territory.length()) {
        locale.append("_");
        locale.append(territory);
        prefixes_.push_back(locale);
      }
      prefixes_.push_back(language);

      std::string locale_id;
      if (GetLocaleIDString(locale.c_str(), &locale_id))
        prefixes_.push_back(locale_id);
    }
    prefixes_.push_back("en_US");
    prefixes_.push_back("en");
    prefixes_.push_back("1033"); // for windows compatibility.
  }

  ~Impl() {
    delete file_manager_;
    file_manager_ = NULL;
  }

  std::vector<std::string> prefixes_;
  FileManagerInterface *file_manager_;
};


LocalizedFileManager::LocalizedFileManager()
  : impl_(new Impl(NULL)){
}

LocalizedFileManager::~LocalizedFileManager() {
  delete impl_;
}

LocalizedFileManager::LocalizedFileManager(FileManagerInterface *file_manager)
  : impl_(new Impl(file_manager)){
}

bool LocalizedFileManager::Attach(FileManagerInterface *file_manager) {
  if (file_manager) {
    delete impl_->file_manager_;
    impl_->file_manager_ = file_manager;
    return true;
  }
  return false;
}

bool LocalizedFileManager::IsValid() {
  return impl_->file_manager_ && impl_->file_manager_->IsValid();
}

bool LocalizedFileManager::Init(const char *base_path, bool create) {
  return impl_->file_manager_ ?
         impl_->file_manager_->Init(base_path, create) :
         false;
}

bool LocalizedFileManager::ReadFile(const char *file, std::string *data) {
  ASSERT(file && data);

  if (!file || !*file || !data)
    return false;

  if (impl_->file_manager_) {
    // Try localized file first.
    for (std::vector<std::string>::iterator it = impl_->prefixes_.begin();
         it != impl_->prefixes_.end(); ++it) {
      std::string path = BuildFilePath(it->c_str(), file, NULL);
      if (impl_->file_manager_->ReadFile(path.c_str(), data))
        return true;
    }
    return impl_->file_manager_->ReadFile(file, data);
  }
  return false;
}

bool LocalizedFileManager::WriteFile(const char *file, const std::string &data,
                                     bool overwrite) {
  // It makes no sense to support writting to localized file.
  return impl_->file_manager_ ?
         impl_->file_manager_->WriteFile(file, data, overwrite) :
         false;
}

bool LocalizedFileManager::RemoveFile(const char *file) {
  ASSERT(file);

  if (!file || !*file)
    return false;

  bool result = false;
  if (impl_->file_manager_) {
    // Remove all localized and non-localized versions.
    for (std::vector<std::string>::iterator it = impl_->prefixes_.begin();
         it != impl_->prefixes_.end(); ++it) {
      std::string path = BuildFilePath(it->c_str(), file, NULL);
      if (impl_->file_manager_->RemoveFile(path.c_str()))
        result = true;
    }
    if (impl_->file_manager_->RemoveFile(file))
      result = true;
  }
  return result;
}

bool LocalizedFileManager::ExtractFile(const char *file, std::string *into_file) {
  ASSERT(file && into_file);

  if (!file || !*file || !into_file)
    return false;

  if (impl_->file_manager_) {
    // Try localized file first.
    for (std::vector<std::string>::iterator it = impl_->prefixes_.begin();
         it != impl_->prefixes_.end(); ++it) {
      std::string path = BuildFilePath(it->c_str(), file, NULL);
      if (impl_->file_manager_->ExtractFile(path.c_str(), into_file))
        return true;
    }
    return impl_->file_manager_->ExtractFile(file, into_file);
  }
  return false;
}

bool LocalizedFileManager::FileExists(const char *file, std::string *path) {
  ASSERT(file);

  if (!file || !*file)
    return false;

  if (impl_->file_manager_) {
    // Try non-localized file first, so that we can get the non-localized path.
    if (impl_->file_manager_->FileExists(file, path))
      return true;

    for (std::vector<std::string>::iterator it = impl_->prefixes_.begin();
         it != impl_->prefixes_.end(); ++it) {
      std::string path = BuildFilePath(it->c_str(), file, NULL);
      if (impl_->file_manager_->FileExists(path.c_str(), NULL))
        return true;
    }
  }
  return false;
}

bool LocalizedFileManager::IsDirectlyAccessible(const char *file,
                                          std::string *path) {
  // This file manager doesn't support localized feature of this function.
  return impl_->file_manager_ ?
         impl_->file_manager_->IsDirectlyAccessible(file, path) :
         false;
}

std::string LocalizedFileManager::GetFullPath(const char *file) {
  // This file manager doesn't support localized feature of this function.
  if (impl_->file_manager_)
    return impl_->file_manager_->GetFullPath(file);
  return std::string("");
}

uint64_t LocalizedFileManager::GetLastModifiedTime(const char *file) {
  // This file manager doesn't support localized feature of this function.
  return impl_->file_manager_ ?
         impl_->file_manager_->GetLastModifiedTime(file) : 0;
}

} // namespace ggadget
