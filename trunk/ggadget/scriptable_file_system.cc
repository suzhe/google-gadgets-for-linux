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
  Variant(fs::FOR_READING), Variant(fs::TRISTATE_FALSE)
};
// Default args for FileSystem.CreateTextFile() and Folder.CreateTextFile().
static const Variant kCreateTextFileDefaultArgs[] = {
  Variant(),
  Variant(true), Variant(false)
};
// Default args for FileSystem.OpenTextFile().
static const Variant kOpenTextFileDefaultArgs[] = {
  Variant(),
  Variant(fs::FOR_READING), Variant(false), Variant(fs::TRISTATE_FALSE)
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

  class FileSystemException : public ScriptableHelper<ScriptableInterface> {
   public:
    DEFINE_CLASS_ID(0x9c53dee0b2114ce4, ScriptableInterface);

    FileSystemException(const char *message)
        : message_(message) {
      message_ += " failed.";
      RegisterConstant("message", message_);
    }

    // This is a script owned object.
    virtual OwnershipPolicy Attach() { return OWNERSHIP_TRANSFERRABLE; } 
    virtual bool Detach() { delete this; return true; }

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
    return ScriptableArray::Create(array, static_cast<size_t>(count), false);
  }

  class ScriptableTextStream : public ScriptableHelper<ScriptableInterface> {
   public:
    DEFINE_CLASS_ID(0x34828c47e6a243c5, ScriptableInterface);
    ScriptableTextStream(fs::TextStreamInterface *stream) : stream_(stream) {
      ASSERT(stream);
      RegisterProperty("Line",
                       NewSlot(stream, &fs::TextStreamInterface::GetLine),
                       NULL);
      RegisterProperty("Column",
                       NewSlot(stream, &fs::TextStreamInterface::GetColumn),
                       NULL);
      RegisterProperty("AtEndOfStream",
                       NewSlot(stream,
                               &fs::TextStreamInterface::IsAtEndOfStream),
                       NULL);
      RegisterProperty("AtEndOfLine",
                       NewSlot(stream, &fs::TextStreamInterface::IsAtEndOfLine),
                       NULL);
      RegisterMethod("Read", NewSlot(stream, &fs::TextStreamInterface::Read));
      RegisterMethod("ReadLine",
                     NewSlot(stream, &fs::TextStreamInterface::ReadLine));
      RegisterMethod("ReadAll",
                     NewSlot(stream, &fs::TextStreamInterface::ReadAll));
      RegisterMethod("Write", NewSlot(stream, &fs::TextStreamInterface::Write));
      RegisterMethod("WriteLine",
                     NewSlot(stream, &fs::TextStreamInterface::WriteLine));
      RegisterMethod("WriteBlankLines",
                     NewSlot(stream,
                             &fs::TextStreamInterface::WriteBlankLines));
      RegisterMethod("Skip", NewSlot(stream, &fs::TextStreamInterface::Skip));
      RegisterMethod("SkipLine",
                     NewSlot(stream, &fs::TextStreamInterface::SkipLine));
      RegisterMethod("Close", NewSlot(stream, &fs::TextStreamInterface::Close));
    }

    virtual ~ScriptableTextStream() {
      stream_->Destroy();
      stream_ = NULL;
    }
    virtual OwnershipPolicy Attach() { return OWNERSHIP_TRANSFERRABLE; }
    virtual bool Detach() { delete this; return true; }

    fs::TextStreamInterface *stream_;
  };

  class ScriptableFolder;
  class ScriptableDrive : public ScriptableHelper<ScriptableInterface> {
   public:
    DEFINE_CLASS_ID(0x0a34071a4804434b, ScriptableInterface);
    ScriptableDrive(fs::DriveInterface *drive) : drive_(drive) {
      ASSERT(drive);
      RegisterProperty("Path",
                       NewSlot(drive, &fs::DriveInterface::GetPath),
                       NULL);
      RegisterProperty("DriveLetter",
                       NewSlot(drive, &fs::DriveInterface::GetDriveLetter),
                       NULL);
      RegisterProperty("ShareName",
                       NewSlot(drive, &fs::DriveInterface::GetShareName),
                       NULL);
      RegisterProperty("DriveType",
                       NewSlot(drive, &fs::DriveInterface::GetDriveType),
                       NULL);
      RegisterProperty("RootFolder",
                       NewSlot(this, &ScriptableDrive::GetRootFolder),
                       NULL);
      RegisterProperty("AvailableSpace",
                       NewSlot(drive, &fs::DriveInterface::GetAvailableSpace),
                       NULL);
      RegisterProperty("FreeSpace",
                       NewSlot(drive, &fs::DriveInterface::GetFreeSpace),
                       NULL);
      RegisterProperty("TotalSize",
                       NewSlot(drive, &fs::DriveInterface::GetTotalSize),
                       NULL);
      RegisterProperty("VolumnName",
                       NewSlot(drive, &fs::DriveInterface::GetVolumnName),
                       NewSlot(this, &ScriptableDrive::SetVolumnName));
      RegisterProperty("FileSystem",
                       NewSlot(drive, &fs::DriveInterface::GetFileSystem),
                       NULL);
      RegisterProperty("SerialNumber",
                       NewSlot(drive, &fs::DriveInterface::GetSerialNumber),
                       NULL);
      RegisterProperty("IsReady",
                       NewSlot(drive, &fs::DriveInterface::IsReady),
                       NULL);
    }

    virtual ~ScriptableDrive() {
      drive_->Destroy();
      drive_ = NULL;
    }
    virtual OwnershipPolicy Attach() { return OWNERSHIP_TRANSFERRABLE; }
    virtual bool Detach() { delete this; return true; }

    ScriptableFolder *GetRootFolder();

    void SetVolumnName(const char *name) {
      if (!drive_->SetVolumnName(name))
        SetPendingException(new FileSystemException("Drive.SetVolumnName"));
    }

    fs::DriveInterface *drive_;
  };

  class ScriptableFolder : public ScriptableHelper<ScriptableInterface> {
   public:
    DEFINE_CLASS_ID(0xa2e7a3ef662a445c, ScriptableInterface);
    ScriptableFolder(fs::FolderInterface *folder) : folder_(folder) {
      ASSERT(folder);
      RegisterProperty("Path",
                       NewSlot(folder, &fs::FolderInterface::GetPath),
                       NULL);
      RegisterProperty("Name",
                       NewSlot(folder, &fs::FolderInterface::GetName),
                       NewSlot(this, &ScriptableFolder::SetName));
      RegisterProperty("ShortPath",
                       NewSlot(folder, &fs::FolderInterface::GetShortPath),
                       NULL);
      RegisterProperty("ShortName",
                       NewSlot(folder, &fs::FolderInterface::GetShortName),
                       NULL);
      RegisterProperty("Drive",
                       NewSlot(this, &ScriptableFolder::GetDrive),
                       NULL);
      RegisterProperty("ParentFolder",
                       NewSlot(this, &ScriptableFolder::GetParentFolder),
                       NULL);
      RegisterProperty("Attributes",
                       NewSlot(folder, &fs::FolderInterface::GetAttributes),
                       NewSlot(this, &ScriptableFolder::SetAttributes));
      RegisterProperty("DateCreated",
                       NewSlot(folder, &fs::FolderInterface::GetDateCreated),
                       NULL);
      RegisterProperty("DateLastModified",
                       NewSlot(folder,
                               &fs::FolderInterface::GetDateLastModified),
                       NULL);
      RegisterProperty("DateLastAccessed",
                       NewSlot(folder,
                               &fs::FolderInterface::GetDateLastAccessed),
                       NULL);
      RegisterProperty("Type",
                       NewSlot(folder, &fs::FolderInterface::GetType),
                       NULL);
      RegisterMethod("Delete",
          NewSlotWithDefaultArgs(NewSlot(this, &ScriptableFolder::Delete),
                                 kDeleteDefaultArgs));
      RegisterMethod("Copy",
          NewSlotWithDefaultArgs(NewSlot(this, &ScriptableFolder::Copy),
                                 kCopyDefaultArgs));
      RegisterMethod("Move", NewSlot(this, &ScriptableFolder::Move));
      RegisterProperty("Size",
                       NewSlot(folder, &fs::FolderInterface::GetSize),
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
    virtual OwnershipPolicy Attach() { return OWNERSHIP_TRANSFERRABLE; }
    virtual bool Detach() { delete this; return true; }

    void SetName(const char *name) {
      if (!folder_->SetName(name))
        SetPendingException(new FileSystemException("Folder.SetName"));
    }

    ScriptableDrive *GetDrive() {
      fs::DriveInterface *drive = folder_->GetDrive();
      if (!drive) {
        SetPendingException(new FileSystemException("Folder.GetDrive"));
        return NULL;
      }
      return new ScriptableDrive(drive);
    }

    ScriptableFolder *GetParentFolder() {
      fs::FolderInterface *folder = folder_->GetParentFolder();
      if (!folder) {
        SetPendingException(new FileSystemException("Folder.GetParentFolder"));
        return NULL;
      }
      return new ScriptableFolder(folder);
    }

    void SetAttributes(fs::FileAttribute attributes) {
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
      fs::FoldersInterface *folders = folder_->GetSubFolders();
      if (!folders) {
        SetPendingException(new FileSystemException("Folder.GetSubFolders"));
        return NULL;
      }
      return ToScriptableArray<ScriptableFolder, fs::FolderInterface>(folders);
    }

    ScriptableArray *GetFiles() {
      fs::FilesInterface *files = folder_->GetFiles();
      if (!files) {
        SetPendingException(new FileSystemException("Folder.GetFiles"));
        return NULL;
      }
      return ToScriptableArray<ScriptableFile, fs::FileInterface>(files);
    }

    ScriptableTextStream *CreateTextFile(const char *filename,
                                         bool overwrite,
                                         bool unicode) {
      fs::TextStreamInterface *stream =
          folder_->CreateTextFile(filename, overwrite, unicode);
      if (!stream) {
        SetPendingException(new FileSystemException("Folder.CreateTextFile"));
        return NULL;
      }
      return new ScriptableTextStream(stream);
    }

    fs::FolderInterface *folder_;
  };

  class ScriptableFile : public ScriptableHelper<ScriptableInterface> {
   public:
    DEFINE_CLASS_ID(0xd8071714bc0a4d2c, ScriptableInterface);
    ScriptableFile(fs::FileInterface *file) : file_(file) {
      ASSERT(file);
      RegisterProperty("Path",
                       NewSlot(file, &fs::FileInterface::GetPath),
                       NULL);
      RegisterProperty("Name",
                       NewSlot(file, &fs::FileInterface::GetName),
                       NewSlot(this, &ScriptableFile::SetName));
      RegisterProperty("ShortPath",
                       NewSlot(file, &fs::FileInterface::GetShortPath),
                       NULL);
      RegisterProperty("ShortName",
                       NewSlot(file, &fs::FileInterface::GetShortName),
                       NULL);
      RegisterProperty("Drive", NewSlot(this, &ScriptableFile::GetDrive), NULL);
      RegisterProperty("ParentFolder",
                       NewSlot(this, &ScriptableFile::GetParentFolder),
                       NULL);
      RegisterProperty("Attributes",
                       NewSlot(file, &fs::FileInterface::GetAttributes),
                       NewSlot(this, &ScriptableFile::SetAttributes));
      RegisterProperty("DateCreated",
                       NewSlot(file, &fs::FileInterface::GetDateCreated),
                       NULL);
      RegisterProperty("DateLastModified",
                       NewSlot(file, &fs::FileInterface::GetDateLastModified),
                       NULL);
      RegisterProperty("DateLastAccessed",
                       NewSlot(file, &fs::FileInterface::GetDateLastAccessed),
                       NULL);
      RegisterProperty("Size",
                       NewSlot(file, &fs::FileInterface::GetSize),
                       NULL);
      RegisterProperty("Type",
                       NewSlot(file, &fs::FileInterface::GetType),
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
    virtual OwnershipPolicy Attach() { return OWNERSHIP_TRANSFERRABLE; }
    virtual bool Detach() { delete this; return true; }

    void SetName(const char *name) {
      if (!file_->SetName(name))
        SetPendingException(new FileSystemException("File.SetName"));
    }

    ScriptableDrive *GetDrive() {
      fs::DriveInterface *drive = file_->GetDrive();
      if (!drive) {
        SetPendingException(new FileSystemException("File.GetDrive"));
        return NULL;
      }
      return new ScriptableDrive(drive);
    }

    ScriptableFolder *GetParentFolder() {
      fs::FolderInterface *folder = file_->GetParentFolder();
      if (!folder) {
        SetPendingException(new FileSystemException("File.GetParentFolder"));
        return NULL;
      }
      return new ScriptableFolder(folder);
    }

    void SetAttributes(fs::FileAttribute attributes) {
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

    ScriptableTextStream *OpenAsTextStream(fs::IOMode mode,
                                           fs::Tristate format) {
      fs::TextStreamInterface *stream =
          file_->OpenAsTextStream(mode, format);
      if (!stream) {
        SetPendingException(new FileSystemException("File.OpenAsTextStream"));
        return NULL;
      }
      return new ScriptableTextStream(stream);
    }

    fs::FileInterface *file_;
  };

  ScriptableArray *GetDrives() {
    fs::DrivesInterface *drives = filesystem_->GetDrives();
    if (!drives) {
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.GetDrives"));
      return NULL;
    }
    return ToScriptableArray<ScriptableDrive, fs::DriveInterface>(drives);
  }

  ScriptableDrive *GetDrive(const char *drive_spec) {
    fs::DriveInterface *drive = filesystem_->GetDrive(drive_spec);
    if (!drive) {
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.GetDrive"));
      return NULL;
    }
    return new ScriptableDrive(drive);
  }

  ScriptableFile *GetFile(const char *file_path) {
    fs::FileInterface *file = filesystem_->GetFile(file_path);
    if (!file) {
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.GetFile"));
      return NULL;
    }
    return new ScriptableFile(file);
  }

  ScriptableFolder *GetFolder(const char *folder_path) {
    fs::FolderInterface *folder = filesystem_->GetFolder(folder_path);
    if (!folder) {
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.GetFolder"));
      return NULL;
    }
    return new ScriptableFolder(folder);
  }

  ScriptableFolder *GetSpecialFolder(fs::SpecialFolder special) {
    fs::FolderInterface *folder = filesystem_->GetSpecialFolder(special);
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
    fs::FolderInterface *folder = filesystem_->CreateFolder(path);
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
    fs::TextStreamInterface *stream =
        filesystem_->CreateTextFile(filename, overwrite, unicode);
    if (!stream) {
      owner_->SetPendingException(new FileSystemException(
          "Filesystem.CreateTextFile"));
      return NULL;
    }
    return new ScriptableTextStream(stream);
  }

  ScriptableTextStream *OpenTextFile(const char *filename, fs::IOMode mode,
                                     bool create, fs::Tristate format) {
    fs::TextStreamInterface *stream =
        filesystem_->OpenTextFile(filename, mode, create, format);
    if (!stream) {
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.OpenTextFile"));
      return NULL;
    }
    return new ScriptableTextStream(stream);
  }

  ScriptableTextStream *GetStandardStream(fs::StandardStreamType type,
                                          bool unicode) {
    fs::TextStreamInterface *stream =
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
  fs::FolderInterface *folder = drive_->GetRootFolder();
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
