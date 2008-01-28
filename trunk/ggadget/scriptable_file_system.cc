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

#include "scriptable_file_system.h"
#include "file_system_interface.h"
#include "scriptable_array.h"
#include "string_utils.h"

namespace ggadget {

using namespace framework;

// Default args for File.Delete() and Folder.Delete().
static const Variant kDeleteDefaultArgs[] = {
  Variant(false)
};
// Default args for File.Copy() and Folder.Copy().
static const Variant kCopyDefaultArgs[] = {
  Variant(),
  Variant(true)
};
// Default args for File.OpenAsTextStream().
static const Variant kOpenAsTextStreamDefaultArgs[] = {
  Variant(IO_MODE_READING), Variant(TRISTATE_FALSE)
};
// Default args for FileSystem.CreateTextFile() and Folder.CreateTextFile().
static const Variant kCreateTextFileDefaultArgs[] = {
  Variant(),
  Variant(true), Variant(false)
};
// Default args for FileSystem.OpenTextFile().
static const Variant kOpenTextFileDefaultArgs[] = {
  Variant(),
  Variant(IO_MODE_READING), Variant(false), Variant(TRISTATE_FALSE)
};
// Default args for FileSystem.DeleteFile() and FileSystem.DeleteFolder()
static const Variant kDeleteFileOrFolderDefaultArgs[] = {
  Variant(),
  Variant(false)
};
// Default args for FileSystem.CopyFile() and FileSystem.CopyFolder()
static const Variant kCopyFileOrFolderDefaultArgs[] = {
  Variant(), Variant(),
  Variant(true)
};
// Default args for FileSystem.GetStandardStream().
static const Variant kGetStandardStreamDefaultArgs[] = {
  Variant(),
  Variant(false)
};

class ScriptableFileSystem::Impl {
 public:
  Impl(FileSystemInterface *filesystem, ScriptableFileSystem *owner)
      : filesystem_(filesystem), owner_(owner) {
  }

  class FileSystemException : public ScriptableHelperDefault {
   public:
    DEFINE_CLASS_ID(0x9c53dee0b2114ce4, ScriptableInterface);

    FileSystemException(const char *message)
        : message_(message) {
      message_ += " failed.";
      RegisterConstant("message", message_);
    }

   private:
    std::string message_;
  };

  template <typename ScriptableT, typename ItemT, typename CollectionT>
  static ScriptableArray *ToScriptableArray(CollectionT *collection) {
    int count = collection->GetCount();
    ASSERT(count >= 0);
    Variant *array = new Variant[count];
    for (int i = 0; i < count; i++) {
      ItemT *item = collection->GetItem(i);
      array[i] = Variant(item ? new ScriptableT(item) : NULL);
    }
    return ScriptableArray::Create(array, static_cast<size_t>(count));
  }

  class ScriptableTextStream : public ScriptableHelperDefault {
   public:
    DEFINE_CLASS_ID(0x34828c47e6a243c5, ScriptableInterface);
    ScriptableTextStream(TextStreamInterface *stream) : stream_(stream) {
      ASSERT(stream);
      RegisterProperty("Line",
                       NewSlot(stream, &TextStreamInterface::GetLine),
                       NULL);
      RegisterProperty("Column",
                       NewSlot(stream, &TextStreamInterface::GetColumn),
                       NULL);
      RegisterProperty("AtEndOfStream",
                       NewSlot(stream, &TextStreamInterface::IsAtEndOfStream),
                       NULL);
      RegisterProperty("AtEndOfLine",
                       NewSlot(stream, &TextStreamInterface::IsAtEndOfLine),
                       NULL);
      RegisterMethod("Read", NewSlot(stream, &TextStreamInterface::Read));
      RegisterMethod("ReadLine",
                     NewSlot(stream, &TextStreamInterface::ReadLine));
      RegisterMethod("ReadAll",
                     NewSlot(stream, &TextStreamInterface::ReadAll));
      RegisterMethod("Write", NewSlot(stream, &TextStreamInterface::Write));
      RegisterMethod("WriteLine",
                     NewSlot(stream, &TextStreamInterface::WriteLine));
      RegisterMethod("WriteBlankLines",
                     NewSlot(stream, &TextStreamInterface::WriteBlankLines));
      RegisterMethod("Skip", NewSlot(stream, &TextStreamInterface::Skip));
      RegisterMethod("SkipLine",
                     NewSlot(stream, &TextStreamInterface::SkipLine));
      RegisterMethod("Close", NewSlot(stream, &TextStreamInterface::Close));
    }

    virtual ~ScriptableTextStream() {
      stream_->Destroy();
      stream_ = NULL;
    }

    TextStreamInterface *stream_;
  };

  class ScriptableFolder;
  class ScriptableDrive : public ScriptableHelperDefault {
   public:
    DEFINE_CLASS_ID(0x0a34071a4804434b, ScriptableInterface);
    ScriptableDrive(DriveInterface *drive) : drive_(drive) {
      ASSERT(drive);
      RegisterProperty("Path",
                       NewSlot(drive, &DriveInterface::GetPath),
                       NULL);
      RegisterProperty("DriveLetter",
                       NewSlot(drive, &DriveInterface::GetDriveLetter),
                       NULL);
      RegisterProperty("ShareName",
                       NewSlot(drive, &DriveInterface::GetShareName),
                       NULL);
      RegisterProperty("DriveType",
                       NewSlot(drive, &DriveInterface::GetDriveType),
                       NULL);
      RegisterProperty("RootFolder",
                       NewSlot(this, &ScriptableDrive::GetRootFolder),
                       NULL);
      RegisterProperty("AvailableSpace",
                       NewSlot(drive, &DriveInterface::GetAvailableSpace),
                       NULL);
      RegisterProperty("FreeSpace",
                       NewSlot(drive, &DriveInterface::GetFreeSpace),
                       NULL);
      RegisterProperty("TotalSize",
                       NewSlot(drive, &DriveInterface::GetTotalSize),
                       NULL);
      RegisterProperty("VolumnName",
                       NewSlot(drive, &DriveInterface::GetVolumnName),
                       NewSlot(this, &ScriptableDrive::SetVolumnName));
      RegisterProperty("FileSystem",
                       NewSlot(drive, &DriveInterface::GetFileSystem),
                       NULL);
      RegisterProperty("SerialNumber",
                       NewSlot(drive, &DriveInterface::GetSerialNumber),
                       NULL);
      RegisterProperty("IsReady",
                       NewSlot(drive, &DriveInterface::IsReady),
                       NULL);
    }

    virtual ~ScriptableDrive() {
      drive_->Destroy();
      drive_ = NULL;
    }

    ScriptableFolder *GetRootFolder();

    void SetVolumnName(const char *name) {
      if (!drive_->SetVolumnName(name))
        SetPendingException(new FileSystemException("Drive.SetVolumnName"));
    }

    DriveInterface *drive_;
  };

  class ScriptableFolder : public ScriptableHelperDefault {
   public:
    DEFINE_CLASS_ID(0xa2e7a3ef662a445c, ScriptableInterface);
    ScriptableFolder(FolderInterface *folder) : folder_(folder) {
      ASSERT(folder);
      RegisterProperty("Path",
                       NewSlot(folder, &FolderInterface::GetPath),
                       NULL);
      RegisterProperty("Name",
                       NewSlot(folder, &FolderInterface::GetName),
                       NewSlot(this, &ScriptableFolder::SetName));
      RegisterProperty("ShortPath",
                       NewSlot(folder, &FolderInterface::GetShortPath),
                       NULL);
      RegisterProperty("ShortName",
                       NewSlot(folder, &FolderInterface::GetShortName),
                       NULL);
      RegisterProperty("Drive",
                       NewSlot(this, &ScriptableFolder::GetDrive),
                       NULL);
      RegisterProperty("ParentFolder",
                       NewSlot(this, &ScriptableFolder::GetParentFolder),
                       NULL);
      RegisterProperty("Attributes",
                       NewSlot(folder, &FolderInterface::GetAttributes),
                       NewSlot(this, &ScriptableFolder::SetAttributes));
      RegisterProperty("DateCreated",
                       NewSlot(folder, &FolderInterface::GetDateCreated),
                       NULL);
      RegisterProperty("DateLastModified",
                       NewSlot(folder, &FolderInterface::GetDateLastModified),
                       NULL);
      RegisterProperty("DateLastAccessed",
                       NewSlot(folder, &FolderInterface::GetDateLastAccessed),
                       NULL);
      RegisterProperty("Type",
                       NewSlot(folder, &FolderInterface::GetType),
                       NULL);
      RegisterMethod("Delete",
          NewSlotWithDefaultArgs(NewSlot(this, &ScriptableFolder::Delete),
                                 kDeleteDefaultArgs));
      RegisterMethod("Copy",
          NewSlotWithDefaultArgs(NewSlot(this, &ScriptableFolder::Copy),
                                 kCopyDefaultArgs));
      RegisterMethod("Move", NewSlot(this, &ScriptableFolder::Move));
      RegisterProperty("Size",
                       NewSlot(folder, &FolderInterface::GetSize),
                       NULL);
      RegisterProperty("SubFolders",
                       NewSlot(this, &ScriptableFolder::GetSubFolders),
                       NULL);
      RegisterProperty("Files",
                       NewSlot(this, &ScriptableFolder::GetFiles),
                       NULL);
      RegisterMethod("CreateTextFile",
                     NewSlotWithDefaultArgs(
                         NewSlot(this, &ScriptableFolder::CreateTextFile),
                         kCreateTextFileDefaultArgs));
    }

    virtual ~ScriptableFolder() {
      folder_->Destroy();
      folder_ = NULL;
    }

    void SetName(const char *name) {
      if (!folder_->SetName(name))
        SetPendingException(new FileSystemException("Folder.SetName"));
    }

    ScriptableDrive *GetDrive() {
      DriveInterface *drive = folder_->GetDrive();
      if (!drive) {
        SetPendingException(new FileSystemException("Folder.GetDrive"));
        return NULL;
      }
      return new ScriptableDrive(drive);
    }

    ScriptableFolder *GetParentFolder() {
      FolderInterface *folder = folder_->GetParentFolder();
      if (!folder) {
        SetPendingException(new FileSystemException("Folder.GetParentFolder"));
        return NULL;
      }
      return new ScriptableFolder(folder);
    }

    void SetAttributes(FileAttribute attributes) {
      if (!folder_->SetAttributes(attributes))
        SetPendingException(new FileSystemException("Folder.SetAttributes"));
    }

    void Delete(bool force) {
      if (!folder_->Delete(force))
        SetPendingException(new FileSystemException("Folder.Delete"));
    }

    void Copy(const char *dest, bool overwrite) {
      if (!folder_->Copy(dest, overwrite))
        SetPendingException(new FileSystemException("Folder.Copy"));
    }

    void Move(const char *dest) {
      if (!folder_->Move(dest))
        SetPendingException(new FileSystemException("Folder.Move"));
    }

    ScriptableArray *GetSubFolders() {
      FoldersInterface *folders = folder_->GetSubFolders();
      if (!folders) {
        SetPendingException(new FileSystemException("Folder.GetSubFolders"));
        return NULL;
      }
      return ToScriptableArray<ScriptableFolder, FolderInterface>(folders);
    }

    ScriptableArray *GetFiles() {
      FilesInterface *files = folder_->GetFiles();
      if (!files) {
        SetPendingException(new FileSystemException("Folder.GetFiles"));
        return NULL;
      }
      return ToScriptableArray<ScriptableFile, FileInterface>(files);
    }

    ScriptableTextStream *CreateTextFile(const char *filename,
                                         bool overwrite,
                                         bool unicode) {
      TextStreamInterface *stream =
          folder_->CreateTextFile(filename, overwrite, unicode);
      if (!stream) {
        SetPendingException(new FileSystemException("Folder.CreateTextFile"));
        return NULL;
      }
      return new ScriptableTextStream(stream);
    }

    FolderInterface *folder_;
  };

  class ScriptableFile : public ScriptableHelperDefault {
   public:
    DEFINE_CLASS_ID(0xd8071714bc0a4d2c, ScriptableInterface);
    ScriptableFile(FileInterface *file) : file_(file) {
      ASSERT(file);
      RegisterProperty("Path",
                       NewSlot(file, &FileInterface::GetPath),
                       NULL);
      RegisterProperty("Name",
                       NewSlot(file, &FileInterface::GetName),
                       NewSlot(this, &ScriptableFile::SetName));
      RegisterProperty("ShortPath",
                       NewSlot(file, &FileInterface::GetShortPath),
                       NULL);
      RegisterProperty("ShortName",
                       NewSlot(file, &FileInterface::GetShortName),
                       NULL);
      RegisterProperty("Drive", NewSlot(this, &ScriptableFile::GetDrive), NULL);
      RegisterProperty("ParentFolder",
                       NewSlot(this, &ScriptableFile::GetParentFolder),
                       NULL);
      RegisterProperty("Attributes",
                       NewSlot(file, &FileInterface::GetAttributes),
                       NewSlot(this, &ScriptableFile::SetAttributes));
      RegisterProperty("DateCreated",
                       NewSlot(file, &FileInterface::GetDateCreated),
                       NULL);
      RegisterProperty("DateLastModified",
                       NewSlot(file, &FileInterface::GetDateLastModified),
                       NULL);
      RegisterProperty("DateLastAccessed",
                       NewSlot(file, &FileInterface::GetDateLastAccessed),
                       NULL);
      RegisterProperty("Size",
                       NewSlot(file, &FileInterface::GetSize),
                       NULL);
      RegisterProperty("Type",
                       NewSlot(file, &FileInterface::GetType),
                       NULL);
      RegisterMethod("Delete",
          NewSlotWithDefaultArgs(NewSlot(this, &ScriptableFile::Delete),
                                 kDeleteDefaultArgs));
      RegisterMethod("Copy",
          NewSlotWithDefaultArgs(NewSlot(this, &ScriptableFile::Copy),
                                 kCopyDefaultArgs));
      RegisterMethod("Move", NewSlot(this, &ScriptableFile::Move));
      RegisterMethod("OpenAsTextStream",
                     NewSlotWithDefaultArgs(
                         NewSlot(this, &ScriptableFile::OpenAsTextStream),
                         kOpenAsTextStreamDefaultArgs));
    }

    virtual ~ScriptableFile() {
      file_->Destroy();
      file_ = NULL;
    }

    void SetName(const char *name) {
      if (!file_->SetName(name))
        SetPendingException(new FileSystemException("File.SetName"));
    }

    ScriptableDrive *GetDrive() {
      DriveInterface *drive = file_->GetDrive();
      if (!drive) {
        SetPendingException(new FileSystemException("File.GetDrive"));
        return NULL;
      }
      return new ScriptableDrive(drive);
    }

    ScriptableFolder *GetParentFolder() {
      FolderInterface *folder = file_->GetParentFolder();
      if (!folder) {
        SetPendingException(new FileSystemException("File.GetParentFolder"));
        return NULL;
      }
      return new ScriptableFolder(folder);
    }

    void SetAttributes(FileAttribute attributes) {
      if (!file_->SetAttributes(attributes))
        SetPendingException(new FileSystemException("File.SetAttributes"));
    }

    void Delete(bool force) {
      if (!file_->Delete(force))
        SetPendingException(new FileSystemException("File.Delete"));
    }

    void Copy(const char *dest, bool overwrite) {
      if (!file_->Copy(dest, overwrite))
        SetPendingException(new FileSystemException("File.Copy"));
    }

    void Move(const char *dest) {
      if (!file_->Move(dest))
        SetPendingException(new FileSystemException("File.Move"));
    }

    ScriptableTextStream *OpenAsTextStream(IOMode mode, Tristate format) {
      TextStreamInterface *stream =
          file_->OpenAsTextStream(mode, format);
      if (!stream) {
        SetPendingException(new FileSystemException("File.OpenAsTextStream"));
        return NULL;
      }
      return new ScriptableTextStream(stream);
    }

    FileInterface *file_;
  };

  ScriptableArray *GetDrives() {
    DrivesInterface *drives = filesystem_->GetDrives();
    if (!drives) {
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.GetDrives"));
      return NULL;
    }
    return ToScriptableArray<ScriptableDrive, DriveInterface>(drives);
  }

  ScriptableDrive *GetDrive(const char *drive_spec) {
    DriveInterface *drive = filesystem_->GetDrive(drive_spec);
    if (!drive) {
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.GetDrive"));
      return NULL;
    }
    return new ScriptableDrive(drive);
  }

  ScriptableFile *GetFile(const char *file_path) {
    FileInterface *file = filesystem_->GetFile(file_path);
    if (!file) {
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.GetFile"));
      return NULL;
    }
    return new ScriptableFile(file);
  }

  ScriptableFolder *GetFolder(const char *folder_path) {
    FolderInterface *folder = filesystem_->GetFolder(folder_path);
    if (!folder) {
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.GetFolder"));
      return NULL;
    }
    return new ScriptableFolder(folder);
  }

  ScriptableFolder *GetSpecialFolder(SpecialFolder special) {
    FolderInterface *folder = filesystem_->GetSpecialFolder(special);
    if (!folder) {
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.GetSpecialFolder"));
      return NULL;
    }
    return new ScriptableFolder(folder);
  }

  void DeleteFile(const char *file_spec, bool force) {
    if (!filesystem_->DeleteFile(file_spec, force))
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.DeleteFile"));
  }

  void DeleteFolder(const char *folder_spec, bool force) {
    if (!filesystem_->DeleteFolder(folder_spec, force))
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.DeleteFolder"));
  }

  void MoveFile(const char *source, const char *dest) {
    if (!filesystem_->MoveFile(source, dest))
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.MoveFile"));
  }

  void MoveFolder(const char *source, const char *dest) {
    if (!filesystem_->MoveFolder(source, dest))
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.MoveFolder"));
  }

  void CopyFile(const char *source, const char *dest, bool overwrite) {
    if (!filesystem_->CopyFile(source, dest, overwrite))
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.CopyFile"));
  }

  void CopyFolder(const char *source, const char *dest, bool overwrite) {
    if (!filesystem_->CopyFolder(source, dest, overwrite))
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.CopyFolder"));
  }

  ScriptableFolder *CreateFolder(const char *path) {
    FolderInterface *folder = filesystem_->CreateFolder(path);
    if (!folder) {
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.CreateFolder"));
      return NULL;
    }
    return new ScriptableFolder(folder);
  }

  ScriptableTextStream *CreateTextFile(const char *filename,
                                       bool overwrite,
                                       bool unicode) {
    TextStreamInterface *stream =
        filesystem_->CreateTextFile(filename, overwrite, unicode);
    if (!stream) {
      owner_->SetPendingException(new FileSystemException(
          "Filesystem.CreateTextFile"));
      return NULL;
    }
    return new ScriptableTextStream(stream);
  }

  ScriptableTextStream *OpenTextFile(const char *filename, IOMode mode,
                                     bool create, Tristate format) {
    TextStreamInterface *stream =
        filesystem_->OpenTextFile(filename, mode, create, format);
    if (!stream) {
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.OpenTextFile"));
      return NULL;
    }
    return new ScriptableTextStream(stream);
  }

  ScriptableTextStream *GetStandardStream(StandardStreamType type,
                                          bool unicode) {
    TextStreamInterface *stream =
        filesystem_->GetStandardStream(type, unicode);
    if (!stream) {
      owner_->SetPendingException(new FileSystemException(
          "Filesystem.GetStandardStream"));
      return NULL;
    }
    return new ScriptableTextStream(stream);
  }

  FileSystemInterface *filesystem_;
  ScriptableFileSystem *owner_;
};

ScriptableFileSystem::ScriptableFileSystem(FileSystemInterface *filesystem)
    : impl_(new Impl(filesystem, this)) {
  ASSERT(filesystem);
  RegisterProperty("Drives", NewSlot(impl_, &Impl::GetDrives), NULL);
  RegisterMethod("BuildPath",
                 NewSlot(filesystem, &FileSystemInterface::BuildPath));
  RegisterMethod("GetDriveName",
                 NewSlot(filesystem,
                         &FileSystemInterface::GetDriveName));
  RegisterMethod("GetParentFolderName",
                 NewSlot(filesystem,
                         &FileSystemInterface::GetParentFolderName));
  RegisterMethod("GetFileName",
                 NewSlot(filesystem, &FileSystemInterface::GetFileName));
  RegisterMethod("GetBaseName",
                 NewSlot(filesystem, &FileSystemInterface::GetBaseName));
  RegisterMethod("GetExtensionName",
                 NewSlot(filesystem, &FileSystemInterface::GetExtensionName));
  RegisterMethod("GetAbsolutePathName",
                 NewSlot(filesystem,
                         &FileSystemInterface::GetAbsolutePathName));
  RegisterMethod("GetTempName",
                 NewSlot(filesystem, &FileSystemInterface::GetTempName));
  RegisterMethod("DriveExists",
                 NewSlot(filesystem, &FileSystemInterface::DriveExists));
  RegisterMethod("FileExists",
                 NewSlot(filesystem, &FileSystemInterface::FileExists));
  RegisterMethod("FolderExists",
                 NewSlot(filesystem, &FileSystemInterface::FolderExists));
  RegisterMethod("GetDrive", NewSlot(impl_, &Impl::GetDrive));
  RegisterMethod("GetFile", NewSlot(impl_, &Impl::GetFile));
  RegisterMethod("GetFolder", NewSlot(impl_, &Impl::GetFolder));
  RegisterMethod("GetSpecialFolder", NewSlot(impl_, &Impl::GetSpecialFolder));
  RegisterMethod("DeleteFile",
                 NewSlotWithDefaultArgs(NewSlot(impl_, &Impl::DeleteFile),
                                        kDeleteFileOrFolderDefaultArgs));
  RegisterMethod("DeleteFolder",
                 NewSlotWithDefaultArgs(NewSlot(impl_, &Impl::DeleteFolder),
                                        kDeleteFileOrFolderDefaultArgs));
  RegisterMethod("MoveFile", NewSlot(impl_, &Impl::MoveFile));
  RegisterMethod("MoveFolder", NewSlot(impl_, &Impl::MoveFolder));
  RegisterMethod("CopyFile",
                 NewSlotWithDefaultArgs(NewSlot(impl_, &Impl::CopyFile),
                                        kCopyFileOrFolderDefaultArgs));
  RegisterMethod("CopyFolder",
                 NewSlotWithDefaultArgs(NewSlot(impl_, &Impl::CopyFolder),
                                        kCopyFileOrFolderDefaultArgs));
  RegisterMethod("CreateFolder", NewSlot(impl_, &Impl::CreateFolder));
  RegisterMethod("CreateTextFile",
                 NewSlotWithDefaultArgs(NewSlot(impl_, &Impl::CreateTextFile),
                                        kCreateTextFileDefaultArgs));
  RegisterMethod("OpenTextFile",
                 NewSlotWithDefaultArgs(NewSlot(impl_, &Impl::OpenTextFile),
                                        kOpenTextFileDefaultArgs));
  RegisterMethod("GetStandardStream",
      NewSlotWithDefaultArgs(NewSlot(impl_, &Impl::GetStandardStream),
                             kGetStandardStreamDefaultArgs));
  RegisterMethod("GetFileVersion",
                 NewSlot(filesystem, &FileSystemInterface::GetFileVersion));
}

ScriptableFileSystem::Impl::ScriptableFolder *
ScriptableFileSystem::Impl::ScriptableDrive::GetRootFolder() {
  FolderInterface *folder = drive_->GetRootFolder();
  if (!folder) {
    SetPendingException(new FileSystemException("Drive.GetRootFolder"));
    return NULL;
  }
  return new ScriptableFolder(folder);
}

ScriptableFileSystem::~ScriptableFileSystem() {
  delete impl_;
  impl_ = NULL;
}

} // namespace ggadget
