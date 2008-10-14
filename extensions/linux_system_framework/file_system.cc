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

#include <algorithm>
#include <cstdlib>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <iterator>
#include <string>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include "ggadget/string_utils.h"
#include "ggadget/system_utils.h"
#include "ggadget/xdg/utilities.h"
#include "ggadget/scoped_ptr.h"
#include "file_system.h"

namespace ggadget {
namespace framework {
namespace linux_system {

static const size_t kBlockSize = 8192;
static const size_t kMaxFileSize = 1024 * 1024;

// utility function for replace all char1 to char2
static void ReplaceAll(std::string *str_ptr,
                       const char char1,
                       const char char2) {
  for (size_t i = 0; i < str_ptr->size(); ++i)
    if ((*str_ptr)[i] == char1)
      (*str_ptr)[i] = char2;
}

void FixCRLF(std::string *data) {
  ASSERT(data);
  size_t position = 0;
  bool in_cr = false;
  for (size_t i = 0; i < data->size(); ++i) {
    if (in_cr) {
      switch ((*data)[i]) {
        case '\n':
          (*data)[position++] = '\n';
          break;
        default:
          (*data)[position++] = '\n';
          (*data)[position++] = (*data)[i];
          break;
      }
      in_cr = false;
    } else {
      switch ((*data)[i]) {
        case '\r':
          in_cr = true;
          break;
        default:
          if (i != position) {
            (*data)[position] = (*data)[i];
          }
          ++position;
          break;
      }
    }
  }
  if (in_cr)
    (*data)[position++] = '\n';
  data->resize(position);
}

// utility function for initializing the file path
static void InitFilePath(const char *filename,
                         std::string *base_ptr,
                         std::string *name_ptr,
                         std::string *path_ptr) {
  ASSERT(filename);
  ASSERT(*filename);
  ASSERT(base_ptr);
  ASSERT(name_ptr);
  ASSERT(path_ptr);

  std::string str_path(filename);
  ReplaceAll(&str_path, '\\', '/');
  str_path = ggadget::GetAbsolutePath(str_path.c_str());

  while (str_path.size() > 0 && str_path[str_path.size() - 1] == '/')
    str_path.resize(str_path.size() - 1);

  if (str_path.empty()) {
    *name_ptr = "/";
    *base_ptr = "/";
    *path_ptr = "/";
    return;
  }

  size_t last_index = str_path.find_last_of('/');

  // filename is in absolute path
  *name_ptr = str_path.substr(last_index + 1,
                              str_path.size() - last_index - 1);
  *base_ptr = str_path.substr(0, last_index + 1);
  *path_ptr = str_path;
}

static bool CopyFile(const char *source, const char *dest, bool overwrite) {
  ASSERT(source);
  ASSERT(*source);
  ASSERT(dest);
  ASSERT(*dest);

  struct stat stat_value;
  std::string sourcefile = ggadget::NormalizeFilePath(source);
  std::string destfile;
  memset(&stat_value, 0, sizeof(stat_value));
  if (stat(dest, &stat_value) == 0) {
    if (S_ISDIR(stat_value.st_mode)) {
      // Destination is a folder.
      std::string base, name, realpath;
      InitFilePath(source, &base, &name, &realpath);
      destfile = ggadget::BuildFilePath(dest, name.c_str(), NULL);
      memset(&stat_value, 0, sizeof(stat_value));
      if (stat(destfile.c_str(), &stat_value) == 0) {
        // Destination exists.
        if (!overwrite)
          return false;
        // Destination is a directory.
        if (S_ISDIR(stat_value.st_mode))
          return false;
      }
    } else {
      // Destination is a file.
      destfile = dest;
      if (!overwrite)
        return false;
    }
  } else {
    // File doesn't exist.
    destfile = dest;
  }

  destfile = ggadget::NormalizeFilePath(destfile.c_str());
  if (sourcefile == destfile)
    return false;

  FILE *in = fopen(sourcefile.c_str(), "rb");
  if (in == NULL)
    return false;
  FILE *out = fopen(destfile.c_str(), "wb");
  if (out == NULL) {
    fclose(in);
    return false;
  }

  ggadget::scoped_ptr<char> p(new char[kBlockSize]);
  size_t size = 0;
  while ((size = fread(p.get(), 1, kBlockSize, in)) > 0) {
    if (fwrite(p.get(), 1, size, out) != size) {
      fclose(in);
      fclose(out);
      return false;
    }
  }

  fclose(in);
  fclose(out);
  return true;
}

static bool CopyFolder(const char *source, const char *dest, bool overwrite) {
  ASSERT(source);
  ASSERT(*source);
  ASSERT(dest);
  ASSERT(*dest);

  struct stat stat_value;
  std::string sourcefile = ggadget::NormalizeFilePath(source);
  std::string destfile;
  memset(&stat_value, 0, sizeof(stat_value));
  if (stat(dest, &stat_value) == 0) {
    if (S_ISDIR(stat_value.st_mode)) {
      // Destination is a folder.
      std::string base, name, realpath;
      InitFilePath(source, &base, &name, &realpath);
      destfile = ggadget::BuildFilePath(dest, name.c_str(), NULL);
      memset(&stat_value, 0, sizeof(stat_value));
      if (stat(destfile.c_str(), &stat_value) == 0) {
        // Destination exists.
        if (!overwrite)
          return false;
        // Destination is a directory.
        if (S_ISDIR(stat_value.st_mode)) {
          if (!overwrite)
            return false;
        }
      }
    } else {
      // Destination is a file.
      destfile = dest;
      if (!overwrite)
        return false;
    }
  } else {
    // File doesn't exist.
    destfile = dest;
  }

  destfile = ggadget::NormalizeFilePath(destfile.c_str());
  if (destfile.size() > sourcefile.size() &&
      destfile[sourcefile.size()] == '/' &&
      strncmp(sourcefile.c_str(), destfile.c_str(), sourcefile.size()) == 0)
    return false;
  if (sourcefile == destfile)
    return false;

  mkdir(destfile.c_str(), 0755);

  DIR *dir = NULL;
  struct dirent *entry = NULL;

  dir = opendir(source);
  if (dir == NULL)
    return false;

  while ((entry = readdir(dir)) != NULL) {
    if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
      continue;

    struct stat stat_value;
    memset(&stat_value, 0, sizeof(stat_value));
    std::string file =
        ggadget::BuildFilePath(source, entry->d_name, NULL);
    if (stat(file.c_str(), &stat_value) == 0) {
      if (S_ISDIR(stat_value.st_mode)) {
        if (!CopyFolder(file.c_str(), destfile.c_str(), overwrite)) {
          closedir(dir);
          return false;
        }
      } else {
        if (!CopyFile(file.c_str(), destfile.c_str(), overwrite)) {
          closedir(dir);
          return false;
        }
      }
    }
  }

  closedir(dir);
  return true;
}

static bool Move(const char *source, const char *dest) {
  ASSERT(source);
  ASSERT(*source);
  ASSERT(dest);
  ASSERT(*dest);

  struct stat stat_value;
  std::string destfile;
  memset(&stat_value, 0, sizeof(stat_value));
  if (stat(dest, &stat_value) == 0) {
    if (S_ISDIR(stat_value.st_mode)) {
      // Destination is a folder.
      std::string base, name, realpath;
      InitFilePath(source, &base, &name, &realpath);
      destfile = ggadget::BuildFilePath(dest, name.c_str(), NULL);
      memset(&stat_value, 0, sizeof(stat_value));
      if (stat(destfile.c_str(), &stat_value) == 0) {
        return false;
      }
    } else {
      // Destination is a file.
      return false;
    }
  } else {
    // File doesn't exist.
    destfile = dest;
  }

  return rename(source, destfile.c_str()) == 0;
}

static bool DeleteFile(const char *filename, bool force) {
  ASSERT(filename);
  ASSERT(*filename);

  if (unlink(filename) == 0) {
    return true;
  }
  return false;
}

static bool DeleteFolder(const char *filename, bool force) {
  ASSERT(filename);
  ASSERT(*filename);

  DIR *dir = NULL;
  struct dirent *entry = NULL;

  dir = opendir(filename);
  if (dir == NULL)
    return 0;

  while ((entry = readdir(dir)) != NULL) {
    if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
      continue;

    struct stat stat_value;
    memset(&stat_value, 0, sizeof(stat_value));
    std::string file =
        ggadget::BuildFilePath(filename, entry->d_name, NULL);
    if (stat(file.c_str(), &stat_value) == 0) {
      if (S_ISDIR(stat_value.st_mode)) {
        if (!DeleteFolder(file.c_str(), force)) {
          closedir(dir);
          return false;
        }
      } else {
        if (!DeleteFile(file.c_str(), force)) {
          closedir(dir);
          return false;
        }
      }
    }
  }

  closedir(dir);
  return rmdir(filename) == 0;
}

static bool SetName(const char *path, const char *dir, const char *name) {
  ASSERT(path);
  ASSERT(*path);
  ASSERT(name);
  ASSERT(*name);

  std::string n(name);
  if (n.find('/') != std::string::npos
      || n.find('\\') != std::string::npos)
    return false;
  std::string newpath = ggadget::BuildFilePath(dir, name, NULL);
  return rename(path, newpath.c_str()) == 0;
}

static int64_t GetFileSize(const char *filename) {
  ASSERT(filename);
  ASSERT(*filename);

  struct stat stat_value;
  memset(&stat_value, 0, sizeof(stat_value));
  stat(filename, &stat_value);
  return stat_value.st_size;
}

static int64_t GetFolderSize(const char *filename) {
  // Gets the init-size of folder.
  struct stat stat_value;
  memset(&stat_value, 0, sizeof(stat_value));
  if (stat(filename, &stat_value))
    return 0;
  int64_t size = stat_value.st_size;

  DIR *dir = NULL;
  struct dirent *entry = NULL;

  dir = opendir(filename);
  if (dir == NULL)
    return 0;

  while ((entry = readdir(dir)) != NULL) {
    if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
      continue;

    struct stat stat_value;
    memset(&stat_value, 0, sizeof(stat_value));
    std::string file =
        ggadget::BuildFilePath(filename, entry->d_name, NULL);
    if (stat(file.c_str(), &stat_value) == 0) {
      if (S_ISDIR(stat_value.st_mode)) {
        // sum up the folder's size
        size += GetFolderSize(file.c_str());
      } else {
        // sum up the file's size
        size += GetFileSize(file.c_str());
      }
    }
  }

  closedir(dir);

  return size;
}

class TextStream : public TextStreamInterface {
 public:
  explicit TextStream(int fd, IOMode mode, bool unicode)
      : fd_(fd),
        mode_(mode),
        line_(-1),
        col_(-1),
        readingptr_(0) {
    if (fd_ != -1) {
      line_ = 1;
      col_ = 1;
    }
  }
  ~TextStream() {
    Close();
  }

  bool Init() {
    if (mode_ == IO_MODE_READING) {
      scoped_ptr<char>buffer(new char[kMaxFileSize]);
      ssize_t size = read(fd_, buffer.get(), kMaxFileSize - 1);
      if (size == -1)
        return false;
      if (size == 0)
        return true;
      buffer.get()[size] = '\0';
      if (!ConvertLocaleStringToUTF8(buffer.get(), &content_)) {
        if (!DetectAndConvertStreamToUTF8(std::string(buffer.get(), size),
                                          &content_,
                                          &encoding_))
          return false;
      }
      FixCRLF(&content_);
    }
    return true;
  }

  virtual void Destroy() { delete this; }

 public:
  virtual int GetLine() {
    return line_;
  }
  virtual int GetColumn() {
    return col_;
  }

  virtual bool IsAtEndOfStream() {
    // FIXME: should throw exception in this situation.
    if (mode_ != IO_MODE_READING)
      return true;
    return readingptr_ >= content_.size();
  }
  virtual bool IsAtEndOfLine() {
    // FIXME: should throw exception in this situation.
    if (mode_ != IO_MODE_READING)
      return true;
    return content_[readingptr_] == '\n';
  }
  virtual std::string Read(int characters) {
    // FIXME: should throw exception in this situation.
    if (mode_ != IO_MODE_READING)
      return std::string();

    size_t size = GetUTF8CharsLength(&content_[readingptr_],
                                     characters,
                                     content_.size() - readingptr_);
    std::string result = content_.substr(readingptr_, size);
    readingptr_ += size;
    UpdatePosition(result);
    return result;
  }

  virtual std::string ReadLine() {
    // FIXME: should throw exception in this situation.
    if (mode_ != IO_MODE_READING)
      return std::string();

    std::string result;
    std::string::size_type position = content_.find('\n', readingptr_);
    if (position == std::string::npos) {
      result = content_.substr(readingptr_);
      readingptr_ = content_.size();
      UpdatePosition(result);
    } else {
      result = content_.substr(readingptr_, position - readingptr_);
      readingptr_ = position + 1;
      col_ = 1;
      ++line_;
    }
    return result;
  }
  virtual std::string ReadAll() {
    // FIXME: should throw exception in this situation.
    if (mode_ != IO_MODE_READING)
      return std::string();

    std::string result;
    result = content_.substr(readingptr_);
    readingptr_ = content_.size();
    UpdatePosition(result);
    return result;
  }

  virtual void Write(const std::string &text) {
    // FIXME: should throw exception in this situation.
    if (mode_ == IO_MODE_READING)
      return;

    std::string copy = text;
    FixCRLF(&copy);
    WriteString(copy);
    UpdatePosition(copy);
  }
  virtual void WriteLine(const std::string &text) {
    // FIXME: should throw exception in this situation.
    if (mode_ == IO_MODE_READING)
      return;

    Write(text);
    Write("\n");
  }
  virtual void WriteBlankLines(int lines) {
    // FIXME: should throw exception in this situation.
    if (mode_ == IO_MODE_READING)
      return;

    for (int i = 0; i < lines; ++i)
      Write("\n");
  }

  virtual void Skip(int characters) {
    // FIXME: should throw exception in this situation.
    if (mode_ != IO_MODE_READING)
      return;
    Read(characters);
  }
  virtual void SkipLine() {
    // FIXME: should throw exception in this situation.
    if (mode_ != IO_MODE_READING)
      return;
    ReadLine();
  }

  virtual void Close() {
    if (fd_ == -1)
      return;
    if (fd_ > STDERR_FILENO) {
      close(fd_);
    }
    fd_ = -1;
  }

 private:
  void UpdatePosition(const std::string &character) {
    size_t position = 0;
    while (position < character.size()) {
      if (character[position] == '\n') {
        col_ = 1;
        ++line_;
        ++position;
      } else {
        position += GetUTF8CharLength(&character[position]);
        ++col_;
      }
    }
  }

  bool WriteString(const std::string &data) {
    std::string buffer;
    if (ConvertUTF8ToLocaleString(data.c_str(), &buffer)) {
      write(fd_, buffer.c_str(), buffer.size());
    }
    return true;
  }

 private:
  int fd_;
  IOMode mode_;
  size_t line_;
  size_t col_;
  std::string content_;
  std::string encoding_;
  size_t readingptr_;
};

static TextStreamInterface *OpenTextFile(const char *filename,
                                         IOMode mode,
                                         bool create,
                                         bool overwrite,
                                         Tristate format) {
  ASSERT(filename);
  ASSERT(*filename);

  int flags = 0;

  switch (mode) {
  case IO_MODE_READING:
    flags = O_RDONLY;
    break;
  case IO_MODE_APPENDING:
    flags = O_APPEND | O_WRONLY;
    break;
  case IO_MODE_WRITING:
    flags = O_TRUNC | O_WRONLY;
    break;
  default:
    ASSERT(false);
    break;
  }

  if (create) {
    flags |= O_CREAT;
  }

  if (!overwrite) {
    flags |= O_EXCL;
  }

  int fd = open(filename, flags, S_IRUSR | S_IWUSR);
  if (fd == -1)
    return NULL;

  TextStream *stream = new TextStream(fd, mode, format == TRISTATE_TRUE);
  if (stream) {
    if (stream->Init()) {
      return stream;
    }
    stream->Destroy();
  }

  return NULL;
}

/**
 * Gets the attributes of a given file or directory.
 * @param path the full path of the file or directory.
 * @param base the base name of the file or directory, that is, the last part
 *     of the full path.  For example, the base of "/path/to/file" is "file".
 */
static FileAttribute GetAttributes(const char *path, const char *base) {
  ASSERT(path);
  ASSERT(*path);
  ASSERT(base);
  ASSERT(*base);

  // FIXME: Check whether path specifies a file or a folder.

  FileAttribute attribute = FILE_ATTR_NORMAL;

  if (base[0] == '.') {
    attribute = static_cast<FileAttribute>(attribute | FILE_ATTR_HIDDEN);
  }

  struct stat stat_value;
  memset(&stat_value, 0, sizeof(stat_value));
  if (stat(path, &stat_value) == -1) {
    return attribute;
  }

  mode_t mode = stat_value.st_mode;

  if (S_ISLNK(mode)) {
    // it is a symbolic link
    attribute = static_cast<FileAttribute>(attribute | FILE_ATTR_ALIAS);
  }

  if (!(mode & S_IWUSR) && (mode & S_IRUSR)) {
    // it is read only by owner
    attribute = static_cast<FileAttribute>(attribute | FILE_ATTR_READONLY);
  }

  return attribute;
}

static bool SetAttributes(const char *filename,
                          FileAttribute attributes) {
  ASSERT(filename);
  ASSERT(*filename);

  struct stat stat_value;
  memset(&stat_value, 0, sizeof(stat_value));
  if (stat(filename, &stat_value) == -1) {
    return false;
  }

  mode_t mode = stat_value.st_mode;
  bool should_change = false;

  // only accept FILE_ATTR_READONLY.
  if ((attributes & FILE_ATTR_READONLY) != 0 &&
      (mode & FILE_ATTR_READONLY) == 0) {
    // modify all the attributes to read only
    mode = static_cast<FileAttribute>((mode | S_IRUSR) & ~S_IWUSR);
    mode = static_cast<FileAttribute>((mode | S_IRGRP) & ~S_IWGRP);
    mode = static_cast<FileAttribute>((mode | S_IROTH) & ~S_IWOTH);
    should_change = true;
  } else if ((attributes & FILE_ATTR_READONLY) == 0 &&
             (mode & FILE_ATTR_READONLY) != 0) {
    mode = static_cast<FileAttribute>(mode | S_IRUSR | S_IWUSR);
    should_change = true;
  }

  if (should_change)
    return chmod(filename, mode) == 0;

  return true;
}

static Date GetDateLastModified(const char *path) {
  ASSERT(path);
  ASSERT(*path);

  struct stat stat_value;
  memset(&stat_value, 0, sizeof(stat_value));
  if (stat(path, &stat_value))
    return Date(0);

  return Date(stat_value.st_mtim.tv_sec * 1000
              + stat_value.st_mtim.tv_nsec / 1000000);
}

static Date GetDateLastAccessed(const char *path) {
  ASSERT(path);
  ASSERT(*path);

  struct stat stat_value;
  memset(&stat_value, 0, sizeof(stat_value));
  if (stat(path, &stat_value))
    return Date(0);

  return Date(stat_value.st_atim.tv_sec * 1000
              + stat_value.st_atim.tv_nsec / 1000000);
}

class Drive : public DriveInterface {
 public:
  Drive() { }

 public:
  virtual void Destroy() {
    // Deliberately does nothing.
  }

  virtual std::string GetPath() {
    return "/";
  }

  virtual std::string GetDriveLetter() {
    return "";
  }

  virtual std::string GetShareName() {
    // TODO: implement this.
    return "";
  }

  virtual DriveType GetDriveType() {
    // TODO: implement this.
    return DRIVE_TYPE_UNKNOWN;
  }

  virtual FolderInterface *GetRootFolder();

  virtual int64_t GetAvailableSpace() {
    // TODO: implement this.
    return 0;
  }

  virtual int64_t GetFreeSpace() {
    // TODO: implement this.
    return 0;
  }

  virtual int64_t GetTotalSize() {
    // TODO: implement this.
    return 0;
  }

  virtual std::string GetVolumnName() {
    // TODO: implement this.
    return "";
  }

  virtual bool SetVolumnName(const char *name) {
    // TODO: implement this.
    return false;
  }

  virtual std::string GetFileSystem() {
    // TODO: implement this.
    return "";
  }

  virtual int64_t GetSerialNumber() {
    // TODO: implement this.
    return 0;
  }

  virtual bool IsReady() {
    return true;
  }
};

static Drive root_drive;

// Drives object simulates a collection contains only one "root" drive.
class Drives : public DrivesInterface {
 public:
  Drives() : at_end_(false) { }
  virtual void Destroy() { }

 public:
  virtual int GetCount() const { return 1; }
  virtual bool AtEnd() { return at_end_; }
  virtual DriveInterface *GetItem() { return at_end_ ? NULL : &root_drive; }
  virtual void MoveFirst() { at_end_ = false; }
  virtual void MoveNext() { at_end_ = true; }

 private:
  bool at_end_;
};

class File : public FileInterface {
 public:
  explicit File(const char *filename) {
    ASSERT(filename);
    ASSERT(*filename);

    InitFilePath(filename, &base_, &name_, &path_);
    struct stat stat_value;
    memset(&stat_value, 0, sizeof(stat_value));
    if (stat(path_.c_str(), &stat_value))
      path_.clear();
    if (S_ISDIR(stat_value.st_mode))
      // it is not a directory
      path_.clear();
  }

  virtual void Destroy() {
    delete this;
  }

 public:
  virtual std::string GetPath() {
    return path_;
  }

  virtual std::string GetName() {
    return name_;
  }

  virtual bool SetName(const char *name) {
    if (!name || !*name)
      return false;
    if (path_.empty())
      return false;
    if (!strcmp(name, name_.c_str()))
      return true;
    if (linux_system::SetName(path_.c_str(), base_.c_str(), name)) {
      path_ = ggadget::BuildFilePath(base_.c_str(), name, NULL);
      InitFilePath(path_.c_str(), &base_, &name_, &path_);
      return true;
    }
    return false;
  }

  virtual std::string GetShortPath() {
    return GetPath();
  }

  virtual std::string GetShortName() {
    return GetName();
  }

  virtual DriveInterface *GetDrive() {
    return &root_drive;
  }

  virtual FolderInterface *GetParentFolder();

  virtual FileAttribute GetAttributes() {
    if (path_.empty())
      return FILE_ATTR_NORMAL;
    return linux_system::GetAttributes(path_.c_str(), name_.c_str());
  }

  virtual bool SetAttributes(FileAttribute attributes) {
    if (path_.empty())
      return false;
    return linux_system::SetAttributes(path_.c_str(), attributes);
  }

  virtual Date GetDateCreated() {
    // can't determine the created date under linux os
    return Date(0);
  }

  virtual Date GetDateLastModified() {
    if (path_.empty())
      return Date(0);
    return linux_system::GetDateLastModified(path_.c_str());
  }

  virtual Date GetDateLastAccessed() {
    if (path_.empty())
      return Date(0);
    return linux_system::GetDateLastAccessed(path_.c_str());
  }

  virtual int64_t GetSize() {
    if (path_.empty())
      return 0;
    return linux_system::GetFileSize(path_.c_str());
  }

  virtual std::string GetType() {
    if (path_.empty())
      return "";
    return ggadget::xdg::GetFileMimeType(path_.c_str());
  }

  virtual bool Delete(bool force) {
    if (path_.empty())
      return false;
    bool result = linux_system::DeleteFile(path_.c_str(), force);
    if (result)
      path_.clear();
    return result;
  }

  virtual bool Copy(const char *dest, bool overwrite) {
    if (path_.empty())
      return false;
    if (!dest || !*dest)
      return false;
    return linux_system::CopyFile(path_.c_str(), dest, overwrite);
  }

  virtual bool Move(const char *dest) {
    if (path_.empty())
      return false;
    if (!dest || !*dest)
      return false;
    bool result = linux_system::Move(path_.c_str(), dest);
    if (result) {
      std::string path = ggadget::GetAbsolutePath(dest);
      InitFilePath(path.c_str(), &base_, &name_, &path_);
    }
    return result;
  }

  virtual TextStreamInterface *OpenAsTextStream(IOMode mode,
                                                Tristate format) {
    if (path_.empty())
      return NULL;
    return linux_system::OpenTextFile(path_.c_str(),
                                      mode,
                                      false,
                                      true,
                                      format);
  }

 private:
  std::string path_;
  std::string base_;
  std::string name_;
};

class Files : public FilesInterface {
 public:
  Files(const char *path)
      : path_(path),
        dir_(NULL),
        at_end_(true) {
  }

  ~Files() {
    if (dir_)
      closedir(dir_);
  }

  bool Init() {
    if (dir_)
      closedir(dir_);
    dir_ = opendir(path_.c_str());
    if (dir_ == NULL) {
      return errno == EACCES;
    }
    at_end_ = false;
    MoveNext();
    return true;
  }

  virtual void Destroy() {
    delete this;
  }

 public:
  virtual int GetCount() const {
    int count = 0;
    DIR *dir = opendir(path_.c_str());
    if (dir == NULL)
        return 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
      if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
        continue;
      struct stat stat_value;
      memset(&stat_value, 0, sizeof(stat_value));
      std::string filename =
          ggadget::BuildFilePath(path_.c_str(), entry->d_name, NULL);
      if (stat(filename.c_str(), &stat_value) == 0) {
        if (!S_ISDIR(stat_value.st_mode))
          ++count;
      }
    }
    closedir(dir);
    return count;
  }

  virtual bool AtEnd() {
    return at_end_;
  }

  virtual FileInterface *GetItem() {
    if (current_file_.empty())
      return NULL;
    return new File(current_file_.c_str());
  }

  virtual void MoveFirst() {
    Init();
  }

  virtual void MoveNext() {
    if (dir_ == NULL)
        return;
    struct dirent *entry;
    while ((entry = readdir(dir_)) != NULL) {
      if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
        continue;
      struct stat stat_value;
      memset(&stat_value, 0, sizeof(stat_value));
      std::string filename =
          ggadget::BuildFilePath(path_.c_str(), entry->d_name, NULL);
      if (stat(filename.c_str(), &stat_value) == 0) {
        if (!S_ISDIR(stat_value.st_mode)) {
          current_file_ = filename;
          return;
        }
      }
    }
    at_end_ = true;
    return;
  }

 private:
  std::string path_;
  DIR *dir_;
  bool at_end_;
  std::string current_file_;
};

class Folders : public FoldersInterface {
 public:
  Folders(const char *path)
      : path_(path),
        dir_(NULL),
        at_end_(true) {
  }

  ~Folders() {
    if (dir_)
      closedir(dir_);
  }

  bool Init() {
    if (dir_)
      closedir(dir_);
    dir_ = opendir(path_.c_str());
    if (dir_ == NULL) {
      return errno == EACCES;
    }
    at_end_ = false;
    MoveNext();
    return true;
  }

  virtual void Destroy() {
    delete this;
  }

 public:
  virtual int GetCount() const {
    int count = 0;
    DIR *dir = opendir(path_.c_str());
    if (dir == NULL)
        return 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
      if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
        continue;
      struct stat stat_value;
      memset(&stat_value, 0, sizeof(stat_value));
      std::string filename =
          ggadget::BuildFilePath(path_.c_str(), entry->d_name, NULL);
      if (stat(filename.c_str(), &stat_value) == 0) {
        if (S_ISDIR(stat_value.st_mode))
          ++count;
      }
    }
    closedir(dir);
    return count;
  }

  virtual bool AtEnd() {
    return at_end_;
  }

  virtual FolderInterface *GetItem();

  virtual void MoveFirst() {
    Init();
  }

  virtual void MoveNext() {
    if (dir_ == NULL)
        return;
    struct dirent *entry;
    while ((entry = readdir(dir_)) != NULL) {
      if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
        continue;
      struct stat stat_value;
      memset(&stat_value, 0, sizeof(stat_value));
      std::string folder =
          ggadget::BuildFilePath(path_.c_str(), entry->d_name, NULL);
      if (stat(folder.c_str(), &stat_value) == 0) {
        if (S_ISDIR(stat_value.st_mode)) {
          current_folder_ = folder;
          return;
        }
      }
    }
    at_end_ = true;
    return;
  }

 private:
  std::string path_;
  DIR *dir_;
  bool at_end_;
  std::string current_folder_;
};

class Folder : public FolderInterface {
 public:
  explicit Folder(const char *filename) {
    ASSERT(filename);
    ASSERT(*filename);

    InitFilePath(filename, &base_, &name_, &path_);
    struct stat stat_value;
    memset(&stat_value, 0, sizeof(stat_value));
    if (stat(path_.c_str(), &stat_value))
      path_.clear();
    if (!S_ISDIR(stat_value.st_mode))
      // it is not a directory
      path_.clear();
  }

  virtual void Destroy() {
    delete this;
  }

 public:
  virtual std::string GetPath() {
    return path_;
  }

  virtual std::string GetName() {
    return name_;
  }

  virtual bool SetName(const char *name) {
    if (!name || !*name)
      return false;
    if (path_.empty())
      return false;
    if (strcmp(name, name_.c_str()) == 0)
      return true;
    if (linux_system::SetName(path_.c_str(), base_.c_str(), name)) {
      path_ = ggadget::BuildFilePath(base_.c_str(), name, NULL);
      InitFilePath(path_.c_str(), &base_, &name_, &path_);
      return true;
    }
    return false;
  }

  virtual std::string GetShortPath() {
    return GetPath();
  }

  virtual std::string GetShortName() {
    return GetName();
  }

  virtual DriveInterface *GetDrive() {
    return NULL;
  }

  virtual FolderInterface *GetParentFolder() {
    if (path_.empty())
      return NULL;
    return new Folder(base_.c_str());
  }

  virtual FileAttribute GetAttributes() {
    if (path_.empty())
      return FILE_ATTR_DIRECTORY;
    return linux_system::GetAttributes(path_.c_str(), name_.c_str());
  }

  virtual bool SetAttributes(FileAttribute attributes) {
    if (path_.empty())
      return false;
    return linux_system::SetAttributes(path_.c_str(), attributes);
  }

  virtual Date GetDateCreated() {
    // can't determine the created date under linux os
    return Date(0);
  }

  virtual Date GetDateLastModified() {
    if (path_.empty())
      return Date(0);
    return linux_system::GetDateLastModified(path_.c_str());
  }

  virtual Date GetDateLastAccessed() {
    if (path_.empty())
      return Date(0);
    return linux_system::GetDateLastAccessed(path_.c_str());
  }

  virtual std::string GetType() {
    if (path_.empty())
      return "";
    return ggadget::xdg::GetFileMimeType(path_.c_str());
  }

  virtual bool Delete(bool force) {
    if (path_.empty())
      return false;
    return linux_system::DeleteFolder(path_.c_str(), force);
  }

  virtual bool Copy(const char *dest, bool overwrite) {
    if (path_.empty())
      return false;
    if (!dest || !*dest)
      return false;
    return linux_system::CopyFolder(path_.c_str(), dest, overwrite);
  }

  virtual bool Move(const char *dest) {
    if (path_.empty())
      return false;
    if (!dest || !*dest)
      return false;
    bool result = linux_system::Move(path_.c_str(), dest);
    if (result) {
      std::string path = ggadget::GetAbsolutePath(dest);
      InitFilePath(path.c_str(), &base_, &name_, &path_);
    }
    return result;
  }

  virtual bool IsRootFolder() {
    return path_ == "/";
  }

  /** Sum of files and subfolders. */
  virtual int64_t GetSize() {
    if (path_.empty())
      return 0;
    return linux_system::GetFolderSize(path_.c_str());
  }

  virtual FoldersInterface *GetSubFolders() {
    if (path_.empty())
      return NULL;

    // creates the Folders instance
    Folders* folders_ptr = new Folders(path_.c_str());
    if (folders_ptr->Init())
      return folders_ptr;
    folders_ptr->Destroy();
    return NULL;
  }

  virtual FilesInterface *GetFiles() {
    if (path_.empty())
      return NULL;

    // Creates the Files instance.
    Files* files_ptr = new Files(path_.c_str());
    if (files_ptr->Init())
      return files_ptr;
    files_ptr->Destroy();
    return NULL;
  }

  virtual TextStreamInterface *CreateTextFile(const char *filename,
                                              bool overwrite, bool unicode) {
    if (!filename || !*filename)
      return NULL;
    if (path_.empty())
      return NULL;

    std::string str_path(filename);
    ReplaceAll(&str_path, '\\', '/');
    std::string file;

    if (ggadget::IsAbsolutePath(str_path.c_str())) {
      // indicates filename is already the absolute path
      file = str_path;
    } else {
      // if not, generate the absolute path
      file = ggadget::BuildFilePath(path_.c_str(), str_path.c_str(), NULL);
    }
    return linux_system::OpenTextFile(
        file.c_str(),
        IO_MODE_WRITING,
        true,
        overwrite,
        unicode ? TRISTATE_TRUE : TRISTATE_FALSE);
  }

 private:
  std::string path_;
  std::string base_;
  std::string name_;
};

FolderInterface *Drive::GetRootFolder() {
  return new Folder("/");
}

FolderInterface *Folders::GetItem() {
  if (current_folder_.empty())
    return NULL;
  return new Folder(current_folder_.c_str());
}

// Implementation of FileSystem
FileSystem::FileSystem() {
}

FileSystem::~FileSystem() {
}

DrivesInterface *FileSystem::GetDrives() {
  return new Drives();
}

std::string FileSystem::BuildPath(const char *path, const char *name) {
  if (!path || !*path)
    return "";
  // Don't need to check name for NULL or empty string.
  return ggadget::BuildFilePath(path, name, NULL);
}

std::string FileSystem::GetDriveName(const char *path) {
  return "";
}

std::string FileSystem::GetParentFolderName(const char *path) {
  if (!path || !*path)
    return "";
  std::string base, name, realpath;
  InitFilePath(path, &base, &name, &realpath);
  // Returns "" for root directory.
  if (realpath == "/")
    return "";
  // Removes the trailing slash from the path.
  if (base.size() > 1 && base[base.size() - 1] == '/')
    base.resize(base.size() - 1);
  return base;
}

std::string FileSystem::GetFileName(const char *path) {
  if (!path || !*path)
    return "";
  std::string base, name, realpath;
  InitFilePath(path, &base, &name, &realpath);
  // Returns "" for root directory.
  if (realpath == "/")
    return "";
  return name;
}

std::string FileSystem::GetBaseName(const char *path) {
  if (!path || !*path)
    return "";
  std::string base, name, realpath;
  InitFilePath(path, &base, &name, &realpath);
  size_t end_index = name.find_last_of('.');
  if (end_index == std::string::npos)
    return name;
  return name.substr(0, end_index);
}

std::string FileSystem::GetExtensionName(const char *path) {
  if (!path || !*path)
    return "";
  std::string base, name, realpath;
  InitFilePath(path, &base, &name, &realpath);
  size_t end_index = name.find_last_of('.');
  if (end_index == std::string::npos)
    return "";
  return name.substr(end_index + 1);
}

// creates the character for filename
static char GetFileChar() {
  while (1) {
    char ch = static_cast<char>(random() % 123);
    if (ch == '_' ||
        ch == '.' ||
        ch == '-' ||
        ('A' <= ch && ch <= 'Z') ||
        ('a' <= ch && ch <= 'z'))
      return ch;
  }
}

std::string FileSystem::GetAbsolutePathName(const char *path) {
  return ggadget::GetAbsolutePath(path);
}

// FIXME: Use system timer to generate a more unique filename.
std::string FileSystem::GetTempName() {
  // Typically, however, file names only use alphanumeric characters(mostly
  // lower case), underscores, hyphens and periods. Other characters, such as
  // dollar signs, percentage signs and brackets, have special meanings to the
  // shell and can be distracting to work with. File names should never begin
  // with a hyphen.
  char filename[9] = {0};
  char character = 0;
  while((character = GetFileChar()) != '-')
    filename[0] = character;
  for (int i = 1; i < 8; ++i)
    filename[i] = GetFileChar();
  return std::string(filename) + ".tmp";
}

bool FileSystem::DriveExists(const char *drive_spec) {
  return false;
}

bool FileSystem::FileExists(const char *file_spec) {
  if (!file_spec || !*file_spec)
    return false;

  std::string str_path(file_spec);
  ReplaceAll(&str_path, '\\', '/');

  if (access(str_path.c_str(), F_OK))
    return false;

  struct stat stat_value;
  memset(&stat_value, 0, sizeof(stat_value));
  if (stat(str_path.c_str(), &stat_value))
    return false;

  if (S_ISDIR(stat_value.st_mode))
    // it is a directory
    return false;

  return true;
}

bool FileSystem::FolderExists(const char *folder_spec) {
  if (!folder_spec || !*folder_spec)
    return false;

  std::string str_path(folder_spec);
  ReplaceAll(&str_path, '\\', '/');

  if (access(str_path.c_str(), F_OK))
    return false;

  struct stat stat_value;
  memset(&stat_value, 0, sizeof(stat_value));
  if (stat(str_path.c_str(), &stat_value))
    return false;
  if (!S_ISDIR(stat_value.st_mode))
    // it is not a directory
    return false;

  return true;
}

DriveInterface *FileSystem::GetDrive(const char *drive_spec) {
  return NULL;
}

// note: if file not exists, return NULL
FileInterface *FileSystem::GetFile(const char *file_path) {
  if (!FileExists(file_path))
    return NULL;
  return new File(file_path);
}

FolderInterface *FileSystem::GetFolder(const char *folder_path) {
  if (!FolderExists(folder_path))
    return NULL;
  return new Folder(folder_path);
}

FolderInterface *FileSystem::GetSpecialFolder(SpecialFolder special_folder) {
  switch (special_folder) {
  case SPECIAL_FOLDER_WINDOWS:
    return new Folder("/");
  case SPECIAL_FOLDER_SYSTEM:
    return new Folder("/");
  case SPECIAL_FOLDER_TEMPORARY:
    return new Folder("/tmp");
  default:
    break;
  }
  return new Folder("/tmp");
}

bool FileSystem::DeleteFile(const char *file_spec, bool force) {
  if (!file_spec || !*file_spec)
    return false;

  glob_t globbuf;
  if (glob(file_spec,
           GLOB_NOSORT | GLOB_PERIOD | GLOB_TILDE,
           NULL, &globbuf) != 0) {
    globfree(&globbuf);
    return false;
  }

  if (globbuf.gl_pathc == 0) {
    globfree(&globbuf);
    return false;
  }

  size_t count = 0;
  for (size_t i = 0; i < globbuf.gl_pathc; ++i) {
    if (FileExists(globbuf.gl_pathv[i])) {
      ++count;
      if (!linux_system::DeleteFile(globbuf.gl_pathv[i], force)) {
        globfree(&globbuf);
        return false;
      }
    }
  }

  globfree(&globbuf);
  return count > 0;
}

bool FileSystem::DeleteFolder(const char *folder_spec, bool force) {
  if (!folder_spec || !*folder_spec)
    return false;

  glob_t globbuf;
  if (glob(folder_spec,
           GLOB_NOSORT | GLOB_PERIOD | GLOB_TILDE,
           NULL, &globbuf) != 0) {
    globfree(&globbuf);
    return false;
  }

  if (globbuf.gl_pathc == 0) {
    globfree(&globbuf);
    return false;
  }

  size_t count = 0;
  for (size_t i = 0; i < globbuf.gl_pathc; ++i) {
    if (FolderExists(globbuf.gl_pathv[i])) {
      ++count;
      if (!linux_system::DeleteFolder(globbuf.gl_pathv[i], force)) {
        globfree(&globbuf);
        return false;
      }
    }
  }

  globfree(&globbuf);
  return count > 0;
}

bool FileSystem::MoveFile(const char *source, const char *dest) {
  if (!source || !*source || !dest || !*dest)
    return false;

  glob_t globbuf;
  if (glob(source,
           GLOB_NOSORT | GLOB_PERIOD | GLOB_TILDE,
           NULL, &globbuf) != 0) {
    globfree(&globbuf);
    return false;
  }

  if (globbuf.gl_pathc > 1) {
    if (!FolderExists(dest)) {
      globfree(&globbuf);
      return false;
    }

    size_t count = 0;
    for (size_t i = 0; i < globbuf.gl_pathc; ++i) {
      if (FileExists(globbuf.gl_pathv[i])) {
        ++count;
        if (!linux_system::Move(globbuf.gl_pathv[i], dest)) {
          globfree(&globbuf);
          return false;
        }
      }
    }
    globfree(&globbuf);
    return count > 0;
  } else if (globbuf.gl_pathc == 1) {
    globfree(&globbuf);
    return linux_system::Move(source, dest);
  }

  globfree(&globbuf);
  return false;
}

bool FileSystem::MoveFolder(const char *source, const char *dest) {
  if (!source || !dest || !*source || !*dest)
    return false;
  if (!source || !*source || !dest || !*dest)
    return false;

  glob_t globbuf;
  if (glob(source,
           GLOB_NOSORT | GLOB_PERIOD | GLOB_TILDE,
           NULL, &globbuf) != 0) {
    globfree(&globbuf);
    return false;
  }

  if (globbuf.gl_pathc > 1) {
    if (!FolderExists(dest)) {
      globfree(&globbuf);
      return false;
    }

    size_t count = 0;
    for (size_t i = 0; i < globbuf.gl_pathc; ++i) {
      if (FolderExists(globbuf.gl_pathv[i])) {
        ++count;
        if (!linux_system::Move(globbuf.gl_pathv[i], dest)) {
          globfree(&globbuf);
          return false;
        }
      }
    }
    globfree(&globbuf);
    return count > 0;
  } else if (globbuf.gl_pathc == 1) {
    globfree(&globbuf);
    return linux_system::Move(source, dest);
  }

  globfree(&globbuf);
  return false;
}

bool FileSystem::CopyFile(const char *source, const char *dest,
                          bool overwrite) {
  if (!source || !*source || !dest || !*dest)
    return false;

  glob_t globbuf;
  if (glob(source,
           GLOB_NOSORT | GLOB_PERIOD | GLOB_TILDE,
           NULL, &globbuf) != 0) {
    globfree(&globbuf);
    return false;
  }

  if (globbuf.gl_pathc > 1) {
    if (!FolderExists(dest)) {
      globfree(&globbuf);
      return false;
    }

    size_t count = 0;
    for (size_t i = 0; i < globbuf.gl_pathc; ++i) {
      if (FileExists(globbuf.gl_pathv[i])) {
        ++count;
        if (!linux_system::CopyFile(globbuf.gl_pathv[i], dest, overwrite)) {
          globfree(&globbuf);
          return false;
        }
      }
    }
    globfree(&globbuf);
    return count > 0;
  } else if (globbuf.gl_pathc == 1) {
    globfree(&globbuf);
    return linux_system::CopyFile(source, dest, overwrite);
  }

  globfree(&globbuf);
  return false;
}

bool FileSystem::CopyFolder(const char *source, const char *dest,
                            bool overwrite) {
  if (!source || !*source || !dest || !*dest)
    return false;

  glob_t globbuf;
  if (glob(source,
           GLOB_NOSORT | GLOB_PERIOD | GLOB_TILDE,
           NULL, &globbuf) != 0) {
    globfree(&globbuf);
    return false;
  }

  if (globbuf.gl_pathc > 1) {
    if (!FolderExists(dest)) {
      globfree(&globbuf);
      return false;
    }

    size_t count = 0;
    for (size_t i = 0; i < globbuf.gl_pathc; ++i) {
      if (FolderExists(globbuf.gl_pathv[i])) {
        ++count;
        if (!linux_system::CopyFolder(globbuf.gl_pathv[i], dest, overwrite)) {
          globfree(&globbuf);
          return false;
        }
      }
    }
    globfree(&globbuf);
    return count > 0;
  } else if (globbuf.gl_pathc == 1) {
    globfree(&globbuf);
    return linux_system::CopyFolder(source, dest, overwrite);
  }

  globfree(&globbuf);
  return false;
}

FolderInterface *FileSystem::CreateFolder(const char *path) {
  if (!path || !*path)
    return NULL;
  std::string str_path(path);
  ReplaceAll(&str_path, '\\', '/');
  struct stat stat_value;
  memset(&stat_value, 0, sizeof(stat_value));
  if (stat(str_path.c_str(), &stat_value) == 0)
    return NULL; // File or directory existed.
  if (mkdir(str_path.c_str(), 0755) == 0)
    return new Folder(str_path.c_str());
  return NULL;
}

TextStreamInterface *FileSystem::CreateTextFile(const char *filename,
                                                bool overwrite,
                                                bool unicode) {
  if (filename == NULL || !*filename)
    return NULL;
  return linux_system::OpenTextFile(filename,
                                    IO_MODE_WRITING,
                                    true,
                                    overwrite,
                                    unicode ? TRISTATE_TRUE : TRISTATE_FALSE);
}

TextStreamInterface *FileSystem::OpenTextFile(const char *filename,
                                              IOMode mode,
                                              bool create,
                                              Tristate format) {
  if (filename == NULL || !*filename)
    return NULL;
  return linux_system::OpenTextFile(filename, mode, create, true, format);
}

TextStreamInterface *
FileSystem::GetStandardStream(StandardStreamType type, bool unicode) {
  TextStream *stream = NULL;
  switch (type) {
  case STD_STREAM_IN:
    stream = new TextStream(STDIN_FILENO, IO_MODE_READING, unicode);
    break;
  case STD_STREAM_OUT:
    stream = new TextStream(STDOUT_FILENO, IO_MODE_WRITING, unicode);
    break;
  case STD_STREAM_ERR:
    stream = new TextStream(STDERR_FILENO, IO_MODE_WRITING, unicode);
  default:
    return NULL;
  }
  if (!stream->Init()) {
    stream->Destroy();
    return NULL;
  }
  return stream;
}

std::string FileSystem::GetFileVersion(const char *filename) {
  return "";
}

FolderInterface *File::GetParentFolder() {
  if (path_.empty())
    return NULL;
  return new Folder(base_.c_str());
}

} // namespace linux_system
} // namespace framework
} // namespace ggadget
