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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <string>
#include <cstdio>
#include <cerrno>

#include <third_party/unzip/zip.h>
#include <third_party/unzip/unzip.h>
#include "common.h"
#include "logger.h"
#include "zip_file_manager.h"
#include "gadget_consts.h"
#include "system_utils.h"

namespace ggadget {

static const char *kZipGlobalComment = "Created by Google Gadgets for Linux.";
static const char *kZipReadMeFile = ".readme";

class ZipFileManager::Impl {
 public:
  Impl() : unzip_handle_(NULL), zip_handle_(NULL) {
  }

  ~Impl() {
    Finalize();
  }

  void Finalize() {
    if (temp_dir_.length())
      RemoveDirectory(temp_dir_.c_str());

    temp_dir_.clear();
    base_path_.clear();

    if (unzip_handle_)
      unzClose(unzip_handle_);
    if (zip_handle_)
      zipClose(zip_handle_, kZipGlobalComment);

    unzip_handle_ = NULL;
    zip_handle_ = NULL;
  }

  bool IsValid() {
    return !base_path_.empty() && (zip_handle_ || unzip_handle_);
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

    unzFile unzip_handle = NULL;
    zipFile zip_handle = NULL;
    struct stat stat_value;
    memset(&stat_value, 0, sizeof(stat_value));
    if (::stat(path.c_str(), &stat_value) == 0) {
      if (!S_ISREG(stat_value.st_mode)) {
        LOG("Not a regular file: %s", path.c_str());
        return false;
      }

      if (::access(path.c_str(), R_OK) != 0) {
        LOG("No permission to access the file %s", path.c_str());
        return false;
      }

      unzip_handle = unzOpen(path.c_str());
      if (!unzip_handle) {
        LOG("Failed to open zip file %s for reading", path.c_str());
        return false;
      }
    } else if (errno == ENOENT && create) {
      zip_handle = zipOpen(path.c_str(), APPEND_STATUS_CREATE);
      if (!zip_handle) {
        LOG("Failed to open zip file %s for writing", path.c_str());
        return false;
      }
      AddReadMeFileInZip(zip_handle);
    } else {
      LOG("Failed to open zip file %s: %s", path.c_str(), strerror(errno));
      return false;
    }

    DLOG("ZipFileManager was initialized successfully for path %s",
         path.c_str());

    Finalize();

    unzip_handle_ = unzip_handle;
    zip_handle_ = zip_handle;
    base_path_ = path;
    return true;
  }

  bool ReadFile(const char *file, std::string *data) {
    ASSERT(data);
    if (data) data->clear();

    std::string relative_path;
    if (!CheckFilePath(file, &relative_path, NULL))
      return false;

    if (!SwitchToRead())
      return false;

    if (unzLocateFile(unzip_handle_, relative_path.c_str(), 2) != UNZ_OK)
      return false;

    if (unzOpenCurrentFile(unzip_handle_) != UNZ_OK) {
      LOG("Can't open file %s for reading in zip archive %s.",
          relative_path.c_str(), base_path_.c_str());
      return false;
    }

    bool result = true;
    const int kChunkSize = 2048;
    char buffer[kChunkSize];
    while (true) {
      int read_size = unzReadCurrentFile(unzip_handle_, buffer, kChunkSize);
      if (read_size > 0) {
        data->append(buffer, read_size);
      } else if (read_size < 0) {
        LOG("Error reading file: %s in zip archive %s",
            relative_path.c_str(), base_path_.c_str());
        data->clear();
        result = false;
        break;
      } else {
        break;
      }
    }

    if (unzCloseCurrentFile(unzip_handle_) != UNZ_OK) {
      LOG("CRC error in file: %s in zip file: %s",
          relative_path.c_str(), base_path_.c_str());
      data->clear();
      result = false;
    }
    return result;
  }

  bool WriteFile(const char *file, const std::string &data, bool overwrite) {
    std::string relative_path;
    if (!CheckFilePath(file, &relative_path, NULL))
      return false;

    // The 'overwrite' parameter is ignored here.
    if (FileExists(file, NULL)) {
      LOG("Can't overwrite an existing file %s in zip archive %s.",
          relative_path.c_str(), base_path_.c_str());
      return false;
    }

    if (!SwitchToWrite())
      return false;

    if (zipOpenNewFileInZip(zip_handle_, relative_path.c_str(),
                            NULL, // zipfi
                            NULL, // extrafield_local
                            0,    // size_extrafield_local
                            NULL, // extrafield_global
                            0,    // size_extrafield_global
                            NULL, // comment
                            Z_DEFLATED,
                            Z_DEFAULT_COMPRESSION) != ZIP_OK) {
      LOG("Can't add new file %s in zip archive %s.",
           relative_path.c_str(), base_path_.c_str());
      return false;
    }

    int result = zipWriteInFileInZip(zip_handle_, data.c_str(), data.length());
    zipCloseFileInZip(zip_handle_);

    if (result != ZIP_OK) {
      LOG("Failed to write file %s into zip archive %s.",
          relative_path.c_str(), base_path_.c_str());
    }

    return result == ZIP_OK;
  }

  bool RemoveFile(const char *file) {
    LOG("Can't remove a file in a zip archive.");
    return false;
  }

  bool ExtractFile(const char *file, std::string *into_file) {
    ASSERT(into_file);

    std::string relative_path;
    if (!CheckFilePath(file, &relative_path, NULL))
      return false;

    if (!SwitchToRead())
      return false;

    if (unzLocateFile(unzip_handle_, relative_path.c_str(), 2) != UNZ_OK)
      return false;

    if (into_file->empty()) {
      if (!EnsureTempDirectory())
        return false;

      // Creates the relative sub directories under temp direcotry.
      std::string dir, file_name;
      SplitFilePath(relative_path.c_str(), &dir, &file_name);

      dir = BuildFilePath(temp_dir_.c_str(), dir.c_str(), NULL);
      if (!EnsureDirectories(dir.c_str()))
        return false;

      *into_file = BuildFilePath(dir.c_str(), file_name.c_str(), NULL);
    }

    FILE *out_fp = fopen(into_file->c_str(), "w");
    if (!out_fp) {
      LOG("Can't open file %s for writing.", into_file->c_str());
      return false;
    }

    if (unzOpenCurrentFile(unzip_handle_) != UNZ_OK) {
      LOG("Can't open file %s for reading in zip archive %s.",
          relative_path.c_str(), base_path_.c_str());
      fclose(out_fp);
      return false;
    }

    bool result = true;
    const int kChunkSize = 8192;
    char buffer[kChunkSize];
    while(true) {
      int read_size = unzReadCurrentFile(unzip_handle_, buffer, kChunkSize);
      if (read_size > 0) {
        if (fwrite(buffer, read_size, 1, out_fp) != 1) {
          result = false;
          LOG("Error when writing to file %s", into_file->c_str());
          break;
        }
      } else if (read_size < 0) {
        LOG("Error reading file: %s in zip archive %s",
            relative_path.c_str(), base_path_.c_str());
        result = false;
        break;
      } else {
        break;
      }
    }

    if (unzCloseCurrentFile(unzip_handle_) != UNZ_OK) {
      LOG("CRC error in file: %s in zip file: %s",
          relative_path.c_str(), base_path_.c_str());
      result = false;
    }
    // fclose() is placed first to ensure it's always called.
    result = (fclose(out_fp) == 0 && result);

    if (!result)
      unlink(into_file->c_str());

    return result;
  }

  bool FileExists(const char *file, std::string *path) {
    std::string full_path, relative_path;
    bool result = CheckFilePath(file, &relative_path, &full_path);
    if (path) *path = full_path;

    // Case insensitive.
    return result && SwitchToRead() &&
           unzLocateFile(unzip_handle_, relative_path.c_str(), 2) == UNZ_OK;
  }

  bool IsDirectlyAccessible(const char *file, std::string *path) {
    CheckFilePath(file, NULL, path);
    return false;
  }

  std::string GetFullPath(const char *file) {
    std::string path;
    if (!file || !*file)
      return base_path_;
    else if (CheckFilePath(file, NULL, &path))
      return path;
    return std::string("");
  }

  // Check if the given file path is valid and return the full path and
  // relative path.
  bool CheckFilePath(const char *file, std::string *relative_path,
                     std::string *full_path) {
    if (relative_path) relative_path->clear();
    if (full_path) full_path->clear();

    if (base_path_.empty()) {
      LOG("ZipFileManager hasn't been initialized.");
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

    if (relative_path)
      relative_path->assign(path.begin() + base_path_.length()+1, path.end());

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

  bool SwitchToRead() {
    if (base_path_.empty())
      return false;

    if (unzip_handle_)
      return true;

    if (zip_handle_) {
      zipClose(zip_handle_, kZipGlobalComment);
      zip_handle_ = NULL;
    }

    unzip_handle_ = unzOpen(base_path_.c_str());
    if (!unzip_handle_)
      LOG("Can't open zip archive %s for reading.", base_path_.c_str());

    return unzip_handle_ != NULL;
  }

  bool SwitchToWrite() {
    if (base_path_.empty())
      return false;

    if (zip_handle_)
      return true;

    if (unzip_handle_) {
      unzClose(unzip_handle_);
      unzip_handle_ = NULL;
    }

    // If the file already exists, then try to open in append mode,
    // otherwise open in create mode.
    if (::access(base_path_.c_str(), F_OK) == 0) {
      zip_handle_ = zipOpen(base_path_.c_str(), APPEND_STATUS_ADDINZIP);
    } else {
      zip_handle_ = zipOpen(base_path_.c_str(), APPEND_STATUS_CREATE);
      if (zip_handle_)
        AddReadMeFileInZip(zip_handle_);
    }

    if (!zip_handle_)
      LOG("Can't open zip archive %s for writing.", base_path_.c_str());

    return zip_handle_ != NULL;
  }

  // At least one file must be added to an empty zip archive, otherwise the
  // archive will become invalid and can't be opened again.
  bool AddReadMeFileInZip(zipFile zip) {
    ASSERT(zip);
    if (zipOpenNewFileInZip(zip, kZipReadMeFile,
                            NULL, // zipfi
                            NULL, // extrafield_local
                            0,    // size_extrafield_local
                            NULL, // extrafield_global
                            0,    // size_extrafield_global
                            NULL, // comment
                            Z_DEFLATED,
                            Z_DEFAULT_COMPRESSION) != ZIP_OK) {
      LOG("Can't add .readme file in newly created zip archive.");
      return false;
    }
    int result =
        zipWriteInFileInZip(zip, kZipGlobalComment, strlen(kZipGlobalComment));
    zipCloseFileInZip(zip);

    if (result != ZIP_OK)
      LOG("Error when adding .readme file in newly created zip archive.");

    return result == ZIP_OK;
  }

  std::string temp_dir_;
  std::string base_path_;

  unzFile unzip_handle_;
  zipFile zip_handle_;
};


ZipFileManager::ZipFileManager()
  : impl_(new Impl()){
}

ZipFileManager::~ZipFileManager() {
  delete impl_;
}

bool ZipFileManager::IsValid() {
  return impl_->IsValid();
}

bool ZipFileManager::Init(const char *base_path, bool create) {
  return impl_->Init(base_path, create);
}

bool ZipFileManager::ReadFile(const char *file, std::string *data) {
  return impl_->ReadFile(file, data);
}

bool ZipFileManager::WriteFile(const char *file, const std::string &data,
                               bool overwrite) {
  return impl_->WriteFile(file, data, overwrite);
}

bool ZipFileManager::RemoveFile(const char *file) {
  return impl_->RemoveFile(file);
}

bool ZipFileManager::ExtractFile(const char *file, std::string *into_file) {
  return impl_->ExtractFile(file, into_file);
}

bool ZipFileManager::FileExists(const char *file, std::string *path) {
  return impl_->FileExists(file, path);
}

bool ZipFileManager::IsDirectlyAccessible(const char *file,
                                          std::string *path) {
  return impl_->IsDirectlyAccessible(file, path);
}

std::string ZipFileManager::GetFullPath(const char *file) {
  return impl_->GetFullPath(file);
}

uint64_t ZipFileManager::GetLastModifiedTime(const char *base_path) {
  // Not implemented.
  return 0;
}

FileManagerInterface *ZipFileManager::Create(const char *base_path,
                                             bool create) {
  FileManagerInterface *fm = new ZipFileManager();
  if (fm->Init(base_path, create))
    return fm;

  delete fm;
  return NULL;
}

} // namespace ggadget
