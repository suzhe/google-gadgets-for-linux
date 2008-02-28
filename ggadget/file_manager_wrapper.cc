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
#include "string_utils.h"
#include "logger.h"

namespace ggadget {

class FileManagerWrapper::Impl {
 public:
  Impl() : default_(NULL) {
  }

  ~Impl() {
    delete default_;
    default_ = NULL;

    for (size_t i = 0; i < file_managers_.size(); ++i)
      delete file_managers_[i].second;

    file_managers_.clear();
  }

  bool RegisterFileManager(const char *prefix, FileManagerInterface *fm) {
    // The default FileManager
    if (!prefix || !*prefix) {
      if (!fm) {
        LOG("A NULL FileManager is specified, the default "
            "FileManager instance will be removed.");
      }
      delete default_;
      default_ = fm;
      return true;
    }

    if (!fm || !fm->IsValid()) {
      LOG("An invalid FileManager instance is specified for prefix %s",
          prefix);
      return false;
    }

    file_managers_.push_back(std::make_pair(std::string(prefix), fm));
    return true;
  }

  bool IsValid() {
    if (default_ && default_->IsValid())
      return true;

    for (FileManagerPrefixMap::iterator it = file_managers_.begin();
         it != file_managers_.end(); ++it) {
      if (it->second->IsValid())
        return true;
    }

    return false;
  }

  bool Init(const char *base_path, bool create) {
    if (default_)
      return default_->Init(base_path, create);
    return false;
  }

  bool ReadFile(const char *file, std::string *data) {
    size_t index = 0;
    FileManagerInterface *fm = NULL;
    std::string path;
    bool matched = false;
    while ((fm = GetNextMatching(file, &index, &path)) != NULL) {
      matched = true;
      if (fm->ReadFile(path.c_str(), data))
        return true;
    }

    if (default_ && !matched)
      return default_->ReadFile(file, data);

    return false;
  }

  bool WriteFile(const char *file, const std::string &data) {
    size_t index = 0;
    FileManagerInterface *fm = NULL;
    std::string path;
    bool matched = false;
    while ((fm = GetNextMatching(file, &index, &path)) != NULL) {
      matched = true;
      // Only writes the file to the first matched FileManager,
      // unless it fails.
      if (fm->WriteFile(path.c_str(), data))
        return true;
    }

    if (default_ && !matched)
      return default_->WriteFile(file, data);

    return false;
  }

  bool RemoveFile(const char *file) {
    size_t index = 0;
    FileManagerInterface *fm = NULL;
    std::string path;
    bool result = false;
    bool matched = false;
    while ((fm = GetNextMatching(file, &index, &path)) != NULL) {
      matched = true;
      // Removes the file in all matched FileManager instance.
      if (fm->RemoveFile(path.c_str()))
        result = true;
    }

    if (default_ && !matched)
      result = default_->RemoveFile(file);

    return result;
  }

  bool ExtractFile(const char *file, std::string *into_file) {
    size_t index = 0;
    FileManagerInterface *fm = NULL;
    std::string path;
    bool matched = false;
    while ((fm = GetNextMatching(file, &index, &path)) != NULL) {
      matched = true;
      if (fm->ExtractFile(path.c_str(), into_file))
        return true;
    }

    if (default_ && !matched)
      return default_->ExtractFile(file, into_file);

    return false;
  }

  bool FileExists(const char *file, std::string *path) {
    size_t index = 0;
    FileManagerInterface *fm = NULL;
    std::string lookup_path;
    bool matched = false;
    while ((fm = GetNextMatching(file, &index, &lookup_path)) != NULL) {
      matched = true;
      if (fm->FileExists(lookup_path.c_str(), path))
        return true;
    }

    if (default_ && !matched)
      return default_->FileExists(file, path);

    return false;
  }

  bool IsDirectlyAccessible(const char *file, std::string *path) {
    size_t index = 0;
    FileManagerInterface *fm = NULL;
    std::string lookup_path;
    bool matched = false;
    while ((fm = GetNextMatching(file, &index, &lookup_path)) != NULL) {
      matched = true;
      if (fm->IsDirectlyAccessible(lookup_path.c_str(), path))
        return true;
    }

    if (default_ && !matched)
      return default_->IsDirectlyAccessible(file, path);

    return false;
  }

  std::string GetFullPath(const char *file) {
    size_t index = 0;
    FileManagerInterface *fm = NULL;
    std::string lookup_path;
    bool matched = false;
    while ((fm = GetNextMatching(file, &index, &lookup_path)) != NULL) {
      matched = true;
      std::string full_path = fm->GetFullPath(lookup_path.c_str());
      if (full_path.length())
        return full_path;
    }

    if (default_ && !matched)
      return default_->GetFullPath(file);

    return std::string("");
  }

  FileManagerInterface *GetNextMatching(const char *path,
                                        size_t *index,
                                        std::string *lookup_path) {
    if (*index >= file_managers_.size() || !path || !*path)
      return NULL;

    while (*index < file_managers_.size()) {
      const std::string &prefix = file_managers_[*index].first;
      FileManagerInterface *fm = file_managers_[*index].second;
      ++*index;

      if (GadgetStrNCmp(prefix.c_str(), path, prefix.size()) == 0) {
        *lookup_path = std::string(path + prefix.size());
        return fm;
      }
    }

    return NULL;
  }

  typedef std::vector<std::pair<std::string, FileManagerInterface *> >
      FileManagerPrefixMap;

  FileManagerPrefixMap file_managers_;
  FileManagerInterface *default_;
};

FileManagerWrapper::FileManagerWrapper()
    : impl_(new Impl()) {
}

FileManagerWrapper::~FileManagerWrapper() {
  delete impl_;
  impl_ = NULL;
}

bool FileManagerWrapper::RegisterFileManager(const char *prefix,
                                             FileManagerInterface *fm) {
  return impl_->RegisterFileManager(prefix, fm);
}

bool FileManagerWrapper::IsValid() {
  return impl_->IsValid();
}

bool FileManagerWrapper::Init(const char *base_path, bool create) {
  return impl_->Init(base_path, create);
}

bool FileManagerWrapper::ReadFile(const char *file, std::string *data) {
  return impl_->ReadFile(file, data);
}

bool FileManagerWrapper::WriteFile(const char *file, const std::string &data) {
  return impl_->WriteFile(file, data);
}

bool FileManagerWrapper::RemoveFile(const char *file) {
  return impl_->RemoveFile(file);
}

bool FileManagerWrapper::ExtractFile(const char *file, std::string *into_file) {
  return impl_->ExtractFile(file, into_file);
}

bool FileManagerWrapper::FileExists(const char *file, std::string *path) {
  return impl_->FileExists(file, path);
}

bool FileManagerWrapper::IsDirectlyAccessible(const char *file,
                                          std::string *path) {
  return impl_->IsDirectlyAccessible(file, path);
}

std::string FileManagerWrapper::GetFullPath(const char *file) {
  return impl_->GetFullPath(file);
}

} // namespace ggadget
