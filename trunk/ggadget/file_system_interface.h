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

#ifndef GGADGET_FILE_SYSTEM_INTERFACE_H__
#define GGADGET_FILE_SYSTEM_INTERFACE_H__

#include <stdint.h>
#include <time.h>

namespace ggadget {

namespace fs {

class DrivesInterface;
class DriveInterface;
class FilesInterface;
class FileInterface;
class FoldersInterface;
class FolderInterface;
class TextStreamInterface;

enum IOMode {
  FOR_READING = 1,
  FOR_WRITING = 2,
  FOR_APPENDING = 8
};

enum Tristate {
  TRISTATE_TRUE = 0xffffffff,
  TRISTATE_FALSE = 0,
  TRISTATE_USE_DEFAULT = 0xfffffffe,
  TRISTATE_MIXED = 0xfffffffe,
};

enum FileAttribute {
  NORMAL = 0,
  READONLY = 1,
  HIDDEN = 2,
  SYSTEM = 4,
  VOLUME = 8,
  DIRECTORY = 16,
  ARCHIVE = 32,
  ALIAS = 1024,
  COMPRESSED = 2048
};

enum SpecialFolder {
  WINDOWS_FOLDER = 0,
  SYSTEM_FOLDER = 1,
  TEMPORARY_FOLDER = 2
};

enum StandardStreamType {
  STD_IN = 0,
  STD_OUT = 1,
  STD_ERR = 2
};

enum DriveType {
  UNKNOWN_TYPE = 0,
  REMOVABLE = 1,
  FIXED = 2,
  REMOTE = 3,
  CDROM = 4,
  RAM_DISK = 5
};

} // namespace fs

/**
 * Simulates the Microsoft IFileSystem3 interface.
 * Used for framework.filesystem.
 *
 * NOTE: if a method returns <code>const char *</code>, the pointer must be
 * used transiently or made a copy. The pointer may become invalid after
 * another call to a method also returns <code>const char *</code> in some
 * implementations.    
 */
class FileSystemInterface {
 public:
  virtual ~FileSystemInterface() { }

 public:
  /** Get drives collection. */
  virtual fs::DrivesInterface *GetDrives() = 0;
  /** Generate a path from an existing path and a name. */
  virtual const char *BuildPath(const char *path, const char *name) = 0;
  /** Return drive from a path. */
  virtual const char *GetDriveName(const char *path) = 0;
  /** Return path to the parent folder. */
  virtual const char *GetParentFolderName(const char *path) = 0;
  /** Return the file name from a path. */
  virtual const char *GetFileName(const char *path) = 0;
  /** Return base name from a path. */
  virtual const char *GetBaseName(const char *path) = 0;
  /** Return extension from path. */
  virtual const char *GetExtensionName(const char *path) = 0;
  /** Return the canonical representation of the path. */
  virtual const char *GetAbsolutePathName(const char *path) = 0;
  /** Generate name that can be used to name a temporary file. */
  virtual const char *GetTempName() = 0;
  /** Check if a drive or a share exists. */
  virtual bool DriveExists(const char *drive_spec) = 0;
  /** Check if a file exists. */
  virtual bool FileExists(const char *file_spec) = 0;
  /** Check if a path exists. */
  virtual bool FolderExists(const char *folder_spec) = 0;
  /** Get drive or UNC share. */
  virtual fs::DriveInterface *GetDrive(const char *drive_spec) = 0;
  /** Get file. */
  virtual fs::FileInterface *GetFile(const char *file_path) = 0;
  /** Get folder. */
  virtual fs::FolderInterface *GetFolder(const char *folder_path) = 0;
  /** Get location of various system folders. */
  virtual fs::FolderInterface *GetSpecialFolder(
      fs::SpecialFolder special_folder) = 0;
  /** Delete a file. */
  virtual bool DeleteFile(const char *file_spec, bool force) = 0;
  /** Delete a folder. */
  virtual bool DeleteFolder(const char *folder_spec, bool force) = 0;
  /** Move a file. */
  virtual bool MoveFile(const char *source, const char *dest) = 0;
  /** Move a folder. */
  virtual bool MoveFolder(const char *source, const char *dest) = 0;
  /** Copy a file. */
  virtual bool CopyFile(const char *source, const char *dest,
                        bool overwrite) = 0;
  /** Copy a folder. */
  virtual bool CopyFolder(const char *source, const char *dest,
                          bool overwrite) = 0;
  /** Create a folder. */
  virtual fs::FolderInterface *CreateFolder(const char *path) = 0;
  /** Create a file as a TextStream. */
  virtual fs::TextStreamInterface *CreateTextFile(const char *filename,
                                                  bool overwrite,
                                                  bool unicode) = 0;
  /** Open a file as a TextStream. */
  virtual fs::TextStreamInterface *OpenTextFile(const char *filename,
                                                fs::IOMode mode,
                                                bool create,
                                                fs::Tristate format) = 0;
  /** Retrieve the standard input, output or error stream. */
  virtual fs::TextStreamInterface *GetStandardStream(
      fs::StandardStreamType type, bool unicode) = 0;
  /** Retrieve the file version of the specified file into a string. */
  virtual const char *GetFileVersion(const char *filename) = 0;
};

namespace fs {

/** IDriveCollection. */
class DrivesInterface {
 protected:
  virtual ~DrivesInterface() {}

 public:
  virtual void Destroy() = 0;

 public:
  virtual int GetCount() const = 0;
  virtual DriveInterface *GetItem(int index) = 0;
};

/** IDrive. */
class DriveInterface {
 protected:
  virtual ~DriveInterface() { }
  
 public:
  virtual void Destroy() = 0;

 public:
  virtual const char *GetPath() = 0;
  virtual const char *GetDriveLetter() = 0;
  virtual const char *GetShareName() = 0;
  virtual DriveType GetDriveType() = 0;
  virtual FolderInterface *GetRootFolder() = 0;
  virtual int64_t GetAvailableSpace() = 0;
  virtual int64_t GetFreeSpace() = 0;
  virtual int64_t GetTotalSize() = 0;
  virtual const char *GetVolumnName() = 0;
  virtual bool SetVolumnName(const char *name) = 0;
  virtual const char *GetFileSystem() = 0;
  virtual int64_t GetSerialNumber() = 0;
  virtual bool IsReady() = 0;
};

/** IFolderCollection. */
class FoldersInterface {
 protected:
  virtual ~FoldersInterface() {}

 public:
  virtual void Destroy() = 0;

 public:
  virtual int GetCount() const = 0;
  virtual FolderInterface *GetItem(int index) = 0;
};

/** IFolder. */
class FolderInterface {
 protected:
  virtual ~FolderInterface() { }

 public:
  virtual void Destroy() = 0;

 public:
  virtual const char *GetPath() = 0;
  virtual const char *GetName() = 0;
  virtual bool SetName(const char *name) = 0;
  virtual const char *GetShortPath() = 0;
  virtual const char *GetShortName() = 0;
  virtual DriveInterface *GetDrive() = 0;
  virtual FolderInterface *GetParentFolder() = 0;
  virtual FileAttribute GetAttributes() = 0;
  virtual bool SetAttributes(FileAttribute attributes) = 0;
  virtual time_t GetDateCreated() = 0;
  virtual time_t GetDateLastModified() = 0;
  virtual time_t GetDateLastAccessed() = 0;
  virtual const char *GetType() = 0;
  virtual bool Delete(bool force) = 0;
  virtual bool Copy(const char *dest, bool overwrite) = 0;
  virtual bool Move(const char *dest) = 0;
  virtual bool IsRootFolder() = 0;
  /** Sum of files and subfolders. */
  virtual int64_t GetSize() = 0;
  virtual FoldersInterface *GetSubFolders() = 0;
  virtual FilesInterface *GetFiles() = 0;
  virtual TextStreamInterface *CreateTextFile(const char *filename,
                                              bool overwrite, bool unicode) = 0;
};

/** IFileCollection. */
class FilesInterface {
 protected:
  virtual ~FilesInterface() {}

 public:
  virtual void Destroy() = 0;

 public:
  virtual int GetCount() const = 0;
  virtual FileInterface *GetItem(int index) = 0;
};

/** IFile. */
class FileInterface {
 protected:
  virtual ~FileInterface() { }

 public:
  virtual void Destroy() = 0;

 public:
  virtual const char *GetPath() = 0;
  virtual const char *GetName() = 0;
  virtual bool SetName(const char *name) = 0;
  virtual const char *GetShortPath() = 0;
  virtual const char *GetShortName() = 0;
  virtual DriveInterface *GetDrive() = 0;
  virtual FolderInterface *GetParentFolder() = 0;
  virtual FileAttribute GetAttributes() = 0;
  virtual bool SetAttributes(FileAttribute attributes) = 0;
  virtual time_t GetDateCreated() = 0;
  virtual time_t GetDateLastModified() = 0;
  virtual time_t GetDateLastAccessed() = 0;
  virtual int64_t GetSize() = 0;
  virtual const char *GetType() = 0;
  virtual bool Delete(bool force) = 0;
  virtual bool Copy(const char *dest, bool overwrite) = 0;
  virtual bool Move(const char *dest) = 0;
  virtual TextStreamInterface *OpenAsTextStream(IOMode IOMode,
                                                Tristate Format) = 0; 
};

class TextStreamInterface {
 protected:
  virtual ~TextStreamInterface() { }

 public:
  virtual void Destroy() = 0;

 public:
  /** Current line number. */
  virtual int GetLine() = 0;
  /** Current column number. */
  virtual int GetColumn() = 0;
  /** Is the current position at the end of the stream? */
  virtual bool IsAtEndOfStream() = 0;
  /** Is the current position at the end of a line? */
  virtual bool IsAtEndOfLine() = 0;
  /** Read a specific number of characters into a string. */
  virtual const char *Read(int characters) = 0;
  /** Read an entire line into a string. */
  virtual const char *ReadLine() = 0;
  /** Read the entire stream into a string. */
  virtual const char *ReadAll() = 0;
  /** Write a string to the stream. */
  virtual void Write(const char *text) = 0;
  /** Write a string and an end of line to the stream. */
  virtual void WriteLine(const char *text) = 0;
  /** Write a number of blank lines to the stream. */
  virtual void WriteBlankLines(int lines) = 0;
  /** Skip a specific number of characters. */
  virtual void Skip(int characters) = 0;
  /** Skip a line. */
  virtual void SkipLine() = 0;
  /** Close a text stream. */
  virtual void Close() = 0;
};

} // namespace fs

} // namespace ggadget

#endif // GGADGET_FILE_SYSTEM_INTERFACE_H__
