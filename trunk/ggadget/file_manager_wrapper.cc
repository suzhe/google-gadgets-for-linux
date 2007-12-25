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

#include <vector>

#include "file_manager_wrapper.h"
#include "logger.h"

namespace ggadget {

class FileManagerWrapper::Impl {
 public:
  Impl() { 
  }

  ~Impl() {
  }

  FileManagerInterface *GetNextMatchingFM(const char *path, 
      size_t *index, std::string *lookup_path) {
    ASSERT(lookup_path);

    if (*index == prefixmap_.size() + 1) {
      return NULL;
    }

    while (*index < prefixmap_.size()) {
      const std::string &prefix = prefixmap_[*index].first;
      FileManagerInterface *fm = prefixmap_[*index].second;
      ++*index;

      if (0 == GadgetStrNCmp(prefix.c_str(), path, prefix.size())) {
        ASSERT(strlen(path) >= prefix.size());
        *lookup_path = std::string(path + prefix.size());
        return fm; 
      }      
    }

    // *index == prefixmap_.size() here, return &default_;
    ++*index;
    *lookup_path = path;
    return &default_;    
  }

  typedef std::vector<std::pair<std::string, FileManagerInterface *> >  
      FileManagerPrefixMap;
  FileManager default_;
  FileManagerPrefixMap prefixmap_;
};

FileManagerWrapper::FileManagerWrapper() : impl_(new FileManagerWrapper::Impl) {
}

FileManagerWrapper::~FileManagerWrapper() {
  delete impl_;
  impl_ = NULL;
}

bool FileManagerWrapper::GetFileContents(const char *file,
                                         std::string *data,
                                         std::string *path) {
  size_t index = 0;
  std::string lookup_path;
  bool result = false;
  FileManagerInterface *fm = NULL;
  while ((fm = impl_->GetNextMatchingFM(file, &index, &lookup_path)) 
         && !result) {
    result = fm->GetFileContents(lookup_path.c_str(), data, path);
  }

  return result;
}

bool FileManagerWrapper::GetXMLFileContents(const char *file,
                                            std::string *data,
                                            std::string *path) {
  size_t index = 0;
  std::string lookup_path;
  bool result = false;
  FileManagerInterface *fm = NULL;
  while ((fm = impl_->GetNextMatchingFM(file, &index, &lookup_path)) 
         && !result) {
    result = fm->GetXMLFileContents(lookup_path.c_str(), data, path);
  }

  return result;
}

bool FileManagerWrapper::ExtractFile(const char *file, std::string *into_file) {
  size_t index = 0;
  std::string lookup_path;
  bool result = false;
  FileManagerInterface *fm = NULL;
  while ((fm = impl_->GetNextMatchingFM(file, &index, &lookup_path)) 
         && !result) {
    result = fm->ExtractFile(lookup_path.c_str(), into_file);
  }

  return result;
}

bool FileManagerWrapper::FileExists(const char *file) {
  size_t index = 0;
  std::string lookup_path;
  bool result = false;
  FileManagerInterface *fm = NULL;
  while ((fm = impl_->GetNextMatchingFM(file, &index, &lookup_path)) 
          && !result) {
    result = fm->FileExists(lookup_path.c_str());
  }

  return result;
}

bool FileManagerWrapper::Init(const char *base_path) {
  return impl_->default_.Init(base_path);
}

GadgetStringMap *FileManagerWrapper::GetStringTable() {
  // Just return the string table for the default FM.
  return impl_->default_.GetStringTable();
}

bool FileManagerWrapper::RegisterFileManager(const char *prefix, 
                                             FileManagerInterface *fm) {
  if (!prefix || !*prefix) {
    return false;
  }

  impl_->prefixmap_.push_back(std::make_pair(std::string(prefix), fm)); 
  // check if fm is already added?

  return true;
}

} // namespace ggadget
