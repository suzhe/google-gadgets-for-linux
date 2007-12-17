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

#include "file_system.h"

namespace ggadget {
namespace framework {
namespace linux {

class Drives : public DrivesInterface {
 public:
  virtual void Destroy() { }

 public:
  virtual int GetCount() const {
    // TODO:
    return 0;
  }

  virtual DriveInterface *GetItem(int index) {
    // TODO:
    return NULL;
  }
};

class Drive : public DriveInterface {
 public:
  Drive(const char *drive_spec) {
  }

  virtual void Destroy() {
    delete this;
  }

 public:
  virtual std::string GetPath() {
    // TODO:
    return "";
  }

  virtual std::string GetDriveLetter() {
    // TODO:
    return "";
  }

  virtual std::string GetShareName() {
    // TODO:
    return "";
  }

  virtual DriveType GetDriveType() {
    // TODO:
    return UNKNOWN_TYPE;
  }

  virtual FolderInterface *GetRootFolder() {
    // TODO:
    return NULL;
  }

  virtual int64_t GetAvailableSpace() {
    // TODO:
    return 0;
  }

  virtual int64_t GetFreeSpace() {
    // TODO:
    return 0;
  }

  virtual int64_t GetTotalSize() {
    // TODO:
    return 0;
  }

  virtual std::string GetVolumnName() {
    // TODO:
    return "";
  }

  virtual bool SetVolumnName(const char *name) {
    // TODO:
    return false;
  }

  virtual std::string GetFileSystem() {
    // TODO:
    return "";
  }

  virtual int64_t GetSerialNumber() {
    // TODO:
    return 0;
  }

  virtual bool IsReady() {
    // TODO:
    return false;
  }

};

/** IFolderCollection. */
class Folders : public FoldersInterface {
 public:
  virtual void Destroy() {
    delete this;
  }

 public:
  virtual int GetCount() const {
    // TODO:
    return 0;
  }

  virtual FolderInterface *GetItem(int index) {
    // TODO:
    return NULL;
  }
};

class Folder : public FolderInterface {
 public:
  Folder(const char *name) {
  }

  virtual void Destroy() {
    delete this;
  }

 public:
  virtual std::string GetPath() {
    // TODO:
    return "";
  }

  virtual std::string GetName() {
    // TODO:
    return "";
  }

  virtual bool SetName(const char *) {
    // TODO:
    return false;
  }

  virtual std::string GetShortPath() {
    // TODO:
    return "";
  }

  virtual std::string GetShortName() {
    // TODO:
    return "";
  }

  virtual DriveInterface *GetDrive() {
    // TODO:
    return NULL;
  }

  virtual FolderInterface *GetParentFolder() {
    // TODO:
    return NULL;
  }

  virtual FileAttribute GetAttributes() {
    // TODO:
    return NORMAL;
  }

  virtual bool SetAttributes(FileAttribute attributes) {
    // TODO:
    return false;
  }

  virtual Date GetDateCreated() {
    // TODO:
    return Date(0);
  }

  virtual Date GetDateLastModified() {
    // TODO:
    return Date(0);
  }

  virtual Date GetDateLastAccessed() {
    // TODO:
    return Date(0);
  }

  virtual std::string GetType() {
    // TODO:
    return "";
  }

  virtual bool Delete(bool force) {
    // TODO:
    return false;
  }

  virtual bool Copy(const char *dest, bool overwrite) {
    // TODO:
    return false;
  }

  virtual bool Move(const char *dest) {
    // TODO:
    return false;
  }

  virtual bool IsRootFolder() {
    // TODO:
    return false;
  }

  /** Sum of files and subfolders. */
  virtual int64_t GetSize() {
    // TODO:
    return 0;
  }

  virtual FoldersInterface *GetSubFolders() {
    // TODO:
    return NULL;
  }

  virtual FilesInterface *GetFiles() {
    // TODO:
    return NULL;
  }

  virtual TextStreamInterface *CreateTextFile(const char *filename,
                                              bool overwrite, bool unicode) {
    // TODO:
    return NULL;
  }
};

class Files : public FilesInterface {
 public:
  virtual void Destroy() {
    delete this;
  }

 public:
  virtual int GetCount() const {
    // TODO:
    return 0;
  }

  virtual FileInterface *GetItem(int index) {
    // TODO:
    return NULL;
  }
};

class File : public FileInterface {
 public:
  File(const char *filename) {
  }

  virtual void Destroy() {
    delete this;
  }

 public:
  virtual std::string GetPath() {
    // TODO:
    return "";
  }

  virtual std::string GetName() {
    // TODO:
    return "";
  }

  virtual bool SetName(const char *name) {
    // TODO:
    return false;
  }

  virtual std::string GetShortPath() {
    // TODO:
    return "";
  }

  virtual std::string GetShortName() {
    // TODO:
    return "";
  }

  virtual DriveInterface *GetDrive() {
    // TODO:
    return NULL;
  }

  virtual FolderInterface *GetParentFolder() {
    // TODO:
    return NULL;
  }

  virtual FileAttribute GetAttributes() {
    // TODO:
    return NORMAL;
  }

  virtual bool SetAttributes(FileAttribute attributes) {
    // TODO:
    return false;
  }

  virtual Date GetDateCreated() {
    // TODO:
    return Date(0);
  }

  virtual Date GetDateLastModified() {
    // TODO:
    return Date(0);
  }

  virtual Date GetDateLastAccessed() {
    // TODO:
    return Date(0);
  }

  virtual int64_t GetSize() {
    // TODO:
    return 0;
  }

  virtual std::string GetType() {
    // TODO:
    return "";
  }

  virtual bool Delete(bool force) {
    // TODO:
    return false;
  }

  virtual bool Copy(const char *dest, bool overwrite) {
    // TODO:
    return false;
  }

  virtual bool Move(const char *dest) {
    // TODO:
    return false;
  }

  virtual TextStreamInterface *OpenAsTextStream(IOMode IOMode,
                                                Tristate Format) {
    // TODO:
    return NULL;
  }
};

class TextStream : public TextStreamInterface {
 public:
  TextStream(const char *filename) {
  }

  TextStream(StandardStreamType type) {
  }

  virtual void Destroy() { delete this; }

 public:
  virtual int GetLine() {
    // TODO:
    return 0;
  }

  virtual int GetColumn() {
    // TODO:
    return 0;
  }

  virtual bool IsAtEndOfStream() {
    // TODO:
    return true;
  }

  virtual bool IsAtEndOfLine() {
    // TODO:
    return true;
  }

  virtual std::string Read(int characters) {
    // TODO:
    return "";
  }

  virtual std::string ReadLine() {
    // TODO:
    return "";
  }

  virtual std::string ReadAll() {
    // TODO:
    return "";
  }

  virtual void Write(const char *text) {
    // TODO:
  }

  virtual void WriteLine(const char *text) {
    // TODO:
  }

  virtual void WriteBlankLines(int lines) {
    // TODO:
  }

  virtual void Skip(int characters) {
    // TODO:
  }

  virtual void SkipLine() {
    // TODO:
  }

  virtual void Close() {
    // TODO:
  }
};

// Implementation of FileSystem
FileSystem::FileSystem() {
  // TODO:
}

FileSystem::~FileSystem() {
  // TODO:
}

DrivesInterface *FileSystem::GetDrives() {
  return NULL;
}

std::string FileSystem::BuildPath(const char *path, const char *name) {
  // TODO:
  return path;
}

std::string FileSystem::GetDriveName(const char *path) {
  // TODO:
  return path;
}

std::string FileSystem::GetParentFolderName(const char *path) {
  // TODO:
  return path;
}

std::string FileSystem::GetFileName(const char *path) {
  // TODO:
  return path;
}

std::string FileSystem::GetBaseName(const char *path) {
  // TODO:
  return path;
}

std::string FileSystem::GetExtensionName(const char *path) {
  // TODO:
  return path;
}

std::string FileSystem::GetAbsolutePathName(const char *path) {
  // TODO:
  return path;
}

std::string FileSystem::GetTempName() {
  // TODO:
  return "/tmp/tmptmp";
}

bool FileSystem::DriveExists(const char *drive_spec) {
  // TODO:
  return false;
}

bool FileSystem::FileExists(const char *file_spec) {
  // TODO:
  return false;
}

bool FileSystem::FolderExists(const char *folder_spec) {
  // TODO:
  return false;
}

DriveInterface *FileSystem::GetDrive(const char *drive_spec) {
  // TODO:
  return new Drive(drive_spec);
}

FileInterface *FileSystem::GetFile(const char *file_path) {
  // TODO:
  return new File(file_path);
}

FolderInterface *FileSystem::GetFolder(const char *folder_path) {
  // TODO:
  return new Folder(folder_path);
}

FolderInterface *FileSystem::GetSpecialFolder(SpecialFolder special_folder) {
  // TODO:
  return new Folder("/");
}

bool FileSystem::DeleteFile(const char *file_spec, bool force) {
  // TODO:
  return false;
}

bool FileSystem::DeleteFolder(const char *folder_spec, bool force) {
  // TODO:
  return false;
}

bool FileSystem::MoveFile(const char *source, const char *dest) {
  // TODO:
  return false;
}

bool FileSystem::MoveFolder(const char *source, const char *dest) {
  // TODO:
  return false;
}

bool FileSystem::CopyFile(const char *source, const char *dest,
                               bool overwrite) {
  // TODO:
  return false;
}

bool FileSystem::CopyFolder(const char *source, const char *dest,
                                 bool overwrite) {
  // TODO:
  return false;
}

FolderInterface *FileSystem::CreateFolder(const char *path) {
  // TODO:
  return new Folder(path);
}

TextStreamInterface *FileSystem::CreateTextFile(const char *filename,
                                                     bool overwrite,
                                                     bool unicode) {
  // TODO:
  return new TextStream(filename);
}

TextStreamInterface *FileSystem::OpenTextFile(const char *filename,
                                                   IOMode mode,
                                                   bool create,
                                                   Tristate format) {
  // TODO:
  return new TextStream(filename);
}

TextStreamInterface *
FileSystem::GetStandardStream(StandardStreamType type, bool unicode) {
  // TODO:
  return new TextStream(type);
}

std::string FileSystem::GetFileVersion(const char *filename) {
  // TODO:
  return "";
}

} // namespace linux
} // namespace framework
} // namespace ggadget
