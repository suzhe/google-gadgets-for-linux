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
#include <cerrno>

#include "common.h"
#include "logger.h"
#include "dir_file_manager.h"
#include "gadget_consts.h"
#include "system_utils.h"

namespace ggadget {

class DirFileManager::Impl {
 public:
  Impl() {
  }

  ~Impl() {
    Finalize();
  }

  void Finalize() {
    if (temp_dir_.length())
      RemoveDirectory(temp_dir_.c_str());

    temp_dir_.clear();
    base_path_.clear();
  }

  bool IsValid() {
    return !base_path_.empty();
  }

  bool Init(const char *base_path, bool create) {
    if (!base_path || !base_path[0]) {
      LOG("Base path is empty.");
      return false;
    }

    std::string path(base_path);

    // Use absolute path.
    if (*base_path != kDirSeparator)
      path = BuildFilePath(GetCurrentDirectory().c_str(), base_path, NULL);

    path = NormalizeFilePath(path.c_str());

    struct stat stat_value;
    memset(&stat_value, 0, sizeof(stat_value));
    if (::stat(path.c_str(), &stat_value) == 0) {
      if (!S_ISDIR(stat_value.st_mode)) {
        LOG("Not a directory: %s", path.c_str());
        return false;
      }

      if (::access(path.c_str(), R_OK|X_OK) != 0) {
        LOG("No permission to access the directory %s", path.c_str());
        return false;
      }
    } else if (errno == ENOENT && create) {
      if (!EnsureDirectories(path.c_str())) {
        LOG("Can't create path: %s", path.c_str());
        return false;
      }
    } else {
      return false;
    }

    DLOG("DirFileManager was initialized successfully for path %s",
         path.c_str());

    Finalize();

    base_path_ = path;
    return true;
  }

  bool ReadFile(const char *file, std::string *data) {
    ASSERT(data);
    if (data) data->clear();

    std::string path;
    if (!CheckFilePath(file, &path))
      return false;

    return ReadFileContents(path.c_str(), data);
  }

  bool WriteFile(const char *file, const std::string &data, bool overwrite) {
    std::string path;
    if (!CheckFilePath(file, &path))
      return false;

    if (::access(path.c_str(), F_OK) == 0) {
      if (overwrite) {
        if (::unlink(path.c_str()) == -1) {
          LOG("Failed to unlink file %s when trying to overwrite it: %s.",
              path.c_str(), strerror(errno));
          return false;
        }
      } else {
        LOG("Can't overwrite an existing file %s, remove it first.",
            path.c_str());
        return false;
      }
    }

    // Ensure the sub directories.
    std::string dir, filename;
    SplitFilePath(path.c_str(), &dir, &filename);
    if (!EnsureDirectories(dir.c_str()))
      return false;

    FILE *datafile = fopen(path.c_str(), "w");
    if (!datafile) {
      LOG("Failed to open file %s for writing: %s",
          path.c_str(), strerror(errno));
      return false;
    }

    bool result = (fwrite(data.c_str(), data.length(), 1, datafile) == 1);
    fclose(datafile);

    return result;
  }

  bool RemoveFile(const char *file) {
    std::string path;
    if (!CheckFilePath(file, &path))
      return false;

    bool result = false;
    struct stat stat_value;
    memset(&stat_value, 0, sizeof(stat_value));

    if (::stat(path.c_str(), &stat_value) == 0) {
      if (!S_ISDIR(stat_value.st_mode))
        result = (::unlink(path.c_str()) == 0);
      else
        result = RemoveDirectory(path.c_str());
    }

    if (!result) {
      LOG("Failed to remove file %s: %s.", file, strerror(errno));
      return false;
    }

    return true;
  }

  bool ExtractFile(const char *file, std::string *into_file) {
    ASSERT(into_file);

    std::string path;
    if (!FileExists(file, &path))
      return false;

    if (into_file->empty()) {
      if (!EnsureTempDirectory())
        return false;

      // Creates the relative sub directories under temp direcotry.
      std::string relative_path = path.substr(base_path_.length() + 1);
      std::string dir, file_name;
      SplitFilePath(relative_path.c_str(), &dir, &file_name);

      dir = BuildFilePath(temp_dir_.c_str(), dir.c_str(), NULL);
      if (!EnsureDirectories(dir.c_str()))
        return false;

      *into_file = BuildFilePath(dir.c_str(), file_name.c_str(), NULL);
    }

    FILE *in_fp = fopen(path.c_str(), "r");
    if (!in_fp) {
      LOG("Can't open file %s for reading.", path.c_str());
      return false;
    }

    FILE *out_fp = fopen(into_file->c_str(), "w");
    if (!out_fp) {
      LOG("Can't open file %s for writing.", into_file->c_str());
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
          LOG("Error when writing to file %s", into_file->c_str());
          result = false;
          break;
        }
      }
      if (read_size < kChunkSize)
        break;
    }

    if (ferror(in_fp)) {
      LOG("Error when reading file: %s", path.c_str());
      result = false;
    }

    fclose(in_fp);
    fclose(out_fp);

    if (!result)
      ::unlink(into_file->c_str());

    return result;
  }

  bool FileExists(const char *file, std::string *path) {
    std::string file_path;
    bool result = CheckFilePath(file, &file_path);
    if (path) *path = file_path;

    return result && ::access(file_path.c_str(), F_OK) == 0;
  }

  bool IsDirectlyAccessible(const char *file, std::string *path) {
    if (!CheckFilePath(file, path))
      return false;
    return true;
  }

  std::string GetFullPath(const char *file) {
    std::string path;
    if (!file || !*file)
      return base_path_;
    else if (CheckFilePath(file, &path))
      return path;
    return std::string("");
  }

  // Check if the given file path is valid and return the full path.
  bool CheckFilePath(const char *file, std::string *full_path) {
    if (full_path) full_path->clear();

    if (base_path_.empty()) {
      LOG("DirFileManager hasn't been initialized.");
      return false;
    }

    // Can't read a file from an absolute path.
    // The file must be a relative path under base_path.
    if (!file || !*file || *file == kDirSeparator) {
      LOG("Invalid file path: %s", (file ? file : "(NULL)"));
      return false;
    }

    std::string path;
    path = BuildFilePath(base_path_.c_str(), file, NULL);
    path = NormalizeFilePath(path.c_str());

    if (full_path) *full_path = path;

    // Check if the normalized path is starting from base_path.
    if (path.length() <= base_path_.length() ||
        strncmp(base_path_.c_str(), path.c_str(), base_path_.length()) != 0 ||
        path[base_path_.length()] != kDirSeparator) {
      LOG("Invalid file path: %s", file);
      return false;
    }

    return true;
  }

  bool EnsureTempDirectory() {
    if (temp_dir_.length())
      return EnsureDirectories(temp_dir_.c_str());

    if (base_path_.length()) {
      std::string path, name;
      SplitFilePath(base_path_.c_str(), &path, &name);

      if (CreateTempDirectory(name.c_str(), &path)) {
        temp_dir_ = path;
        DLOG("A temporary directory has been created: %s", path.c_str());
        return true;
      }
    }

    return false;
  }

  uint64_t GetLastModifiedTime(const char *file) {
    std::string path;
    if (!CheckFilePath(file, &path))
      return 0;
    struct stat stat_value;
    memset(&stat_value, 0, sizeof(stat_value));
    return ::stat(path.c_str(), &stat_value) == 0 ?
           stat_value.st_mtime * UINT64_C(1000) : 0;
  }

  std::string temp_dir_;
  std::string base_path_;
};


DirFileManager::DirFileManager()
  : impl_(new Impl()){
}

DirFileManager::~DirFileManager() {
  delete impl_;
}

bool DirFileManager::IsValid() {
  return impl_->IsValid();
}

bool DirFileManager::Init(const char *base_path, bool create) {
  return impl_->Init(base_path, create);
}

bool DirFileManager::ReadFile(const char *file, std::string *data) {
  return impl_->ReadFile(file, data);
}

bool DirFileManager::WriteFile(const char *file, const std::string &data,
                               bool overwrite) {
  return impl_->WriteFile(file, data, overwrite);
}

bool DirFileManager::RemoveFile(const char *file) {
  return impl_->RemoveFile(file);
}

bool DirFileManager::ExtractFile(const char *file, std::string *into_file) {
  return impl_->ExtractFile(file, into_file);
}

bool DirFileManager::FileExists(const char *file, std::string *path) {
  return impl_->FileExists(file, path);
}

bool DirFileManager::IsDirectlyAccessible(const char *file,
                                          std::string *path) {
  return impl_->IsDirectlyAccessible(file, path);
}

std::string DirFileManager::GetFullPath(const char *file) {
  return impl_->GetFullPath(file);
}

uint64_t DirFileManager::GetLastModifiedTime(const char *file) {
  return impl_->GetLastModifiedTime(file);
}

FileManagerInterface *DirFileManager::Create(const char *base_path,
                                             bool create) {
  FileManagerInterface *fm = new DirFileManager();
  if (fm->Init(base_path, create))
    return fm;

  delete fm;
  return NULL;
}

} // namespace ggadget
