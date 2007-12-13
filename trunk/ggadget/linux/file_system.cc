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

namespace fs {

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

} // namespace fs

class FileSystem : public FileSystemInterface {
 public:
  virtual fs::DrivesInterface *GetDrives() {
    return NULL;
  }

  virtual std::string BuildPath(const char *path, const char *name) {
    // TODO:
    return path;
  }

  virtual std::string GetDriveName(const char *path) {
    // TODO:
    return path;
  }

  virtual std::string GetParentFolderName(const char *path) {
    // TODO:
    return path;
  }

  virtual std::string GetFileName(const char *path) {
    // TODO:
    return path;
  }

  virtual std::string GetBaseName(const char *path) {
    // TODO:
    return path;
  }

  virtual std::string GetExtensionName(const char *path) {
    // TODO:
    return path;
  }

  virtual std::string GetAbsolutePathName(const char *path) {
    // TODO:
    return path;
  }

  virtual std::string GetTempName() {
    // TODO:
    return "/tmp/tmptmp";
  }

  virtual bool DriveExists(const char *drive_spec) {
    // TODO:
    return false;
  }

  virtual bool FileExists(const char *file_spec) {
    // TODO:
    return false;
  }

  virtual bool FolderExists(const char *folder_spec) {
    // TODO:
    return false;
  }

  virtual fs::DriveInterface *GetDrive(const char *drive_spec) {
    // TODO:
    return new fs::Drive(drive_spec);
  }

  virtual fs::FileInterface *GetFile(const char *file_path) {
    // TODO:
    return new fs::File(file_path);
  }

  virtual fs::FolderInterface *GetFolder(const char *folder_path) {
    // TODO:
    return new fs::Folder(folder_path);
  }

  virtual fs::FolderInterface *GetSpecialFolder(
      fs::SpecialFolder special_folder) {
    // TODO:
    return new fs::Folder("/");
  }

  virtual bool DeleteFile(const char *file_spec, bool force) {
    // TODO:
    return false;
  }

  virtual bool DeleteFolder(const char *folder_spec, bool force) {
    // TODO:
    return false;
  }

  virtual bool MoveFile(const char *source, const char *dest) {
    // TODO:
    return false;
  }

  virtual bool MoveFolder(const char *source, const char *dest) {
    // TODO:
    return false;
  }

  virtual bool CopyFile(const char *source, const char *dest,
                        bool overwrite) {
    // TODO:
    return false;
  }

  virtual bool CopyFolder(const char *source, const char *dest,
                          bool overwrite) {
    // TODO:
    return false;
  }

  virtual fs::FolderInterface *CreateFolder(const char *path) {
    // TODO:
    return new fs::Folder(path);
  }

  virtual fs::TextStreamInterface *CreateTextFile(const char *filename,
                                                  bool overwrite,
                                                  bool unicode) {
    // TODO:
    return new fs::TextStream(filename);
  }

  virtual fs::TextStreamInterface *OpenTextFile(const char *filename,
                                                fs::IOMode mode,
                                                bool create,
                                                fs::Tristate format) {
    // TODO:
    return new fs::TextStream(filename);
  }

  virtual fs::TextStreamInterface *GetStandardStream(
      fs::StandardStreamType type, bool unicode) {
    // TODO:
    return new fs::TextStream(type);
  }

  virtual std::string GetFileVersion(const char *filename) {
    // TODO:
    return "";
  }
};

FileSystemInterface *CreateFileSystem() {
  return new FileSystem();
}

} // namespace ggadget
