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

#include "common.h"
#include "gadget_consts.h"
#include "slot.h"
#include "framework_interface.h"
#include "file_system_interface.h"
#include "audioclip_interface.h"
#include "dummy_framework.h"

namespace ggadget {
namespace framework {

class DummyMachine : public MachineInterface {
 public:
  virtual std::string GetBiosSerialNumber() const { return "Unknown"; }
  virtual std::string GetMachineManufacturer() const { return "Unknown"; }
  virtual std::string GetMachineModel() const { return "Unknown"; }
  virtual std::string GetProcessorArchitecture() const { return "Unknown"; }
  virtual int GetProcessorCount() const { return 0; }
  virtual int GetProcessorFamily() const { return 0; }
  virtual int GetProcessorModel() const { return 0; }
  virtual std::string GetProcessorName() const { return "Unknown"; }
  virtual int GetProcessorSpeed() const { return 0; }
  virtual int GetProcessorStepping() const { return 0; }
  virtual std::string GetProcessorVendor() const { return "Unknown"; }
};

class DummyMemory : public MemoryInterface {
 public:
  virtual int64_t GetTotal() { return 1024*1024*1024; }
  virtual int64_t GetFree() { return 1024*1024*512; }
  virtual int64_t GetUsed() { return 1024*1024*512; }
  virtual int64_t GetFreePhysical() { return 1024*1024*512; }
  virtual int64_t GetTotalPhysical() { return 1024*1024*1024; }
  virtual int64_t GetUsedPhysical() { return 1024*1024*512; }
};

class DummyNetwork : public NetworkInterface {
 public:
  virtual bool IsOnline() const { return true; }
  virtual ConnectionType GetConnectionType() const {
    return NetworkInterface::CONNECTION_TYPE_802_3;
  }
  virtual PhysicalMediaType GetPhysicalMediaType() const {
    return NetworkInterface::PHISICAL_MEDIA_TYPE_UNSPECIFIED;
  }
};

class DummyPerfmon : public PerfmonInterface {
 public:
  virtual int64_t GetCurrentValue(const char *counter_path) { return 0; }
};

class DummyPower : public PowerInterface {
 public:
  virtual bool IsCharging() const { return false; }
  virtual bool IsPluggedIn() const { return true; }
  virtual int GetPercentRemaining() const { return 100; }
  virtual int GetTimeRemaining() const { return 3600; }
  virtual int GetTimeTotal() const { return 7200; }
};

class DummyProcessInfo : public ProcessInfoInterface {
 public:
  virtual void Destroy() { }
  virtual int GetProcessId() const { return 1234; }
  virtual std::string GetExecutablePath() const { return "/usr/bin/dummy"; }
};

class DummyProcesses : public ProcessesInterface {
 public:
  virtual void Destroy() { }
  virtual int GetCount() const { return 100; }
  virtual ProcessInfoInterface *GetItem(int index) { return &dummy_info_; }

 private:
  DummyProcessInfo dummy_info_;
};

class DummyProcess : public ProcessInterface {
 public:
  virtual ProcessesInterface *EnumerateProcesses() { return &dummy_processes_; }
  virtual ProcessInfoInterface *GetForeground() { return &dummy_foreground_; }
  virtual ProcessInfoInterface *GetInfo(int pid) { return &dummy_info_; }
 private:
  DummyProcesses dummy_processes_;
  DummyProcessInfo dummy_foreground_;
  DummyProcessInfo dummy_info_;
};

class DummyWirelessAccessPoint : public WirelessAccessPointInterface {
 public:
  virtual void Destroy() { }
  virtual std::string GetName() const { return "Unknown"; }
  virtual Type GetType() const {
    return WirelessAccessPointInterface::WIRELESS_TYPE_ANY;
  }
  virtual int GetSignalStrength() const { return 50; }
  virtual void Connect(Slot1<void, bool> *callback) {
    if (callback) {
      (*callback)(true);
      delete callback;
    }
  }
  virtual void Disconnect(Slot1<void, bool> *callback) {
    if (callback) {
      (*callback)(true);
      delete callback;
    }
  }
};

class DummyWireless : public WirelessInterface {
 public:
  virtual bool IsAvailable() const { return false; }
  virtual bool IsConnected() const { return false; }
  virtual bool EnumerationSupported() const { return false; }
  virtual int GetAPCount() const { return 0; }
  virtual WirelessAccessPointInterface *GetWirelessAccessPoint(int index) {
    return NULL;
  }
  virtual std::string GetName() const { return "Unknown"; }
  virtual std::string GetNetworkName() const  { return "Unknwon"; }
  virtual int GetSignalStrength() const { return 0; }
};

class DummyDrives : public DrivesInterface {
 public:
  virtual void Destroy() { delete this; }
  virtual int GetCount() const { return 0; }
  virtual DriveInterface *GetItem(int index) { return NULL; }
};

class DummyDrive : public DriveInterface {
 public:
  virtual void Destroy() { delete this; }
  virtual std::string GetPath() { return ""; }
  virtual std::string GetDriveLetter() { return ""; }
  virtual std::string GetShareName() { return ""; }
  virtual DriveType GetDriveType() { return DRIVE_TYPE_UNKNOWN; }
  virtual FolderInterface *GetRootFolder() { return NULL; }
  virtual int64_t GetAvailableSpace() { return 0; }
  virtual int64_t GetFreeSpace() { return 0; }
  virtual int64_t GetTotalSize() { return 0; }
  virtual std::string GetVolumnName() { return ""; }
  virtual bool SetVolumnName(const char *name) { return false; }
  virtual std::string GetFileSystem() { return ""; }
  virtual int64_t GetSerialNumber() { return 0; }
  virtual bool IsReady() { return false; }
};

/** IFolderCollection. */
class DummyFolders : public FoldersInterface {
 public:
  virtual void Destroy() { }
  virtual int GetCount() const { return 0; }
  virtual FolderInterface *GetItem(int index) { return NULL; }
};

class DummyFolder : public FolderInterface {
 public:
  virtual void Destroy() { }
  virtual std::string GetPath() { return ""; }
  virtual std::string GetName() { return ""; }
  virtual bool SetName(const char *) { return false; }
  virtual std::string GetShortPath() { return ""; }
  virtual std::string GetShortName() { return ""; }
  virtual DriveInterface *GetDrive() { return NULL; }
  virtual FolderInterface *GetParentFolder() { return NULL; }
  virtual FileAttribute GetAttributes() { return FILE_ATTR_NORMAL; }
  virtual bool SetAttributes(FileAttribute attributes) { return false; }
  virtual Date GetDateCreated() { return Date(0); }
  virtual Date GetDateLastModified() { return Date(0); }
  virtual Date GetDateLastAccessed() { return Date(0); }
  virtual std::string GetType() { return ""; }
  virtual bool Delete(bool force) { return false; }
  virtual bool Copy(const char *dest, bool overwrite) { return false; }
  virtual bool Move(const char *dest) { return false; }
  virtual bool IsRootFolder() { return false; }
  virtual int64_t GetSize() { return 0; }
  virtual FoldersInterface *GetSubFolders() { return NULL; }
  virtual FilesInterface *GetFiles() { return NULL; }
  virtual TextStreamInterface *CreateTextFile(const char *filename,
                                              bool overwrite, bool unicode) {
    return NULL;
  }
};

class DummyFiles : public FilesInterface {
 public:
  virtual void Destroy() { delete this; }
  virtual int GetCount() const { return 0; }
  virtual FileInterface *GetItem(int index) { return NULL; }
};

class DummyFile : public FileInterface {
 public:
  virtual void Destroy() { delete this; }
  virtual std::string GetPath() { return ""; }
  virtual std::string GetName() { return ""; }
  virtual bool SetName(const char *name) { return false; }
  virtual std::string GetShortPath() { return ""; }
  virtual std::string GetShortName() { return ""; }
  virtual DriveInterface *GetDrive() { return NULL; }
  virtual FolderInterface *GetParentFolder() { return NULL; }
  virtual FileAttribute GetAttributes() { return FILE_ATTR_NORMAL; }
  virtual bool SetAttributes(FileAttribute attributes) { return false; }
  virtual Date GetDateCreated() { return Date(0); }
  virtual Date GetDateLastModified() { return Date(0); }
  virtual Date GetDateLastAccessed() { return Date(0); }
  virtual int64_t GetSize() { return 0; }
  virtual std::string GetType() { return ""; }
  virtual bool Delete(bool force) { return false; }
  virtual bool Copy(const char *dest, bool overwrite) { return false; }
  virtual bool Move(const char *dest) { return false; }
  virtual TextStreamInterface *OpenAsTextStream(IOMode IOMode,
                                                Tristate Format) {
    return NULL;
  }
};

class DummyTextStream : public TextStreamInterface {
 public:
  virtual void Destroy() { delete this; }
  virtual int GetLine() { return 0; }
  virtual int GetColumn() { return 0; }
  virtual bool IsAtEndOfStream() { return true; }
  virtual bool IsAtEndOfLine() { return true; }
  virtual std::string Read(int characters) { return ""; }
  virtual std::string ReadLine() { return ""; }
  virtual std::string ReadAll() { return ""; }
  virtual void Write(const char *text) { }
  virtual void WriteLine(const char *text) { }
  virtual void WriteBlankLines(int lines) { }
  virtual void Skip(int characters) { }
  virtual void SkipLine() { }
  virtual void Close() { }
};

class DummyFileSystem : public FileSystemInterface {
 public:
  virtual DrivesInterface *GetDrives() { return new DummyDrives(); }
  virtual std::string BuildPath(const char *path, const char *name) {
    return std::string(path ? path : "") +
           std::string(kDirSeparatorStr) +
           std::string(name);
  }
  virtual std::string GetDriveName(const char *path) {
    return path ? path : "";
  }
  virtual std::string GetParentFolderName(const char *path) {
    return path ? path : "";
  }
  virtual std::string GetFileName(const char *path) {
    return path ? path : "";
  }
  virtual std::string GetBaseName(const char *path) {
    return path ? path : "";
  }
  virtual std::string GetExtensionName(const char *path) {
    return path ? path : "";
  }
  virtual std::string GetAbsolutePathName(const char *path) {
    return path ? path : "";
  }
  virtual std::string GetTempName() { return "/tmp/tmptmp"; }
  virtual bool DriveExists(const char *drive_spec) { return false; }
  virtual bool FileExists(const char *file_spec) { return false; }
  virtual bool FolderExists(const char *folder_spec) { return false; }
  virtual DriveInterface *GetDrive(const char *drive_spec) {
    return new DummyDrive();
  }
  virtual FileInterface *GetFile(const char *file_path) {
    return new DummyFile();
  }
  virtual FolderInterface *GetFolder(const char *folder_path) {
    return new DummyFolder();
  }
  virtual FolderInterface *GetSpecialFolder(SpecialFolder special_folder) {
    return new DummyFolder();
  }
  virtual bool DeleteFile(const char *file_spec, bool force) { return false; }
  virtual bool DeleteFolder(const char *folder_spec, bool force) {
    return false;
  }
  virtual bool MoveFile(const char *source, const char *dest) { return false; }
  virtual bool MoveFolder(const char *source, const char *dest) {
    return false;
  }
  virtual bool CopyFile(const char *source, const char *dest,
                        bool overwrite) {
    return false;
  }
  virtual bool CopyFolder(const char *source, const char *dest,
                          bool overwrite) {
    return false;
  }

  virtual FolderInterface *CreateFolder(const char *path) {
    return new DummyFolder();
  }
  virtual TextStreamInterface *CreateTextFile(const char *filename,
                                              bool overwrite,
                                              bool unicode) {
    return new DummyTextStream();
  }
  virtual TextStreamInterface *OpenTextFile(const char *filename,
                                            IOMode mode,
                                            bool create,
                                            Tristate format) {
    return new DummyTextStream();
  }
  virtual TextStreamInterface *GetStandardStream(StandardStreamType type,
                                                 bool unicode) {
    return new DummyTextStream();
  }
  virtual std::string GetFileVersion(const char *filename) {
    return "";
  }
};

class DummyAudioclip : public AudioclipInterface {
 public:
  virtual void Destroy() { delete this; }
  virtual int GetBalance() const { return 0; }
  virtual void SetBalance(int balance) { }
  virtual int GetCurrentPosition() const { return 0; }
  virtual void SetCurrentPosition(int position) { }
  virtual int GetDuration() const { return 100; }
  virtual ErrorCode GetError() const { return SOUND_ERROR_NO_ERROR; }
  virtual std::string GetSrc() const { return "src"; }
  virtual void SetSrc(const char *src) { }
  virtual State GetState() const { return SOUND_STATE_PLAYING; }
  virtual int GetVolume() const { return 100; }
  virtual void SetVolume(int volume) const { }
  virtual void Play() { }
  virtual void Pause() { }
  virtual void Stop() { }
  virtual Connection *ConnectOnStateChange(OnStateChangeHandler *handler) {
    delete handler;
    return NULL;
  }
};

} // namespace framework

struct DummyFramework::Impl {
  framework::DummyMachine machine_;
  framework::DummyMemory memory_;
  framework::DummyNetwork network_;
  framework::DummyPerfmon perfmon_;
  framework::DummyPower power_;
  framework::DummyProcess process_;
  framework::DummyWireless wireless_;
  framework::DummyFileSystem filesystem_;
};

DummyFramework::DummyFramework()
  : impl_(new Impl()) {
}

DummyFramework::~DummyFramework() {
  delete impl_;
  impl_ = NULL;
}

framework::MachineInterface *DummyFramework::GetMachine() {
  return &impl_->machine_;
}
framework::MemoryInterface *DummyFramework::GetMemory() {
  return &impl_->memory_;
}
framework::NetworkInterface *DummyFramework::GetNetwork() {
  return &impl_->network_;
}
framework::PerfmonInterface *DummyFramework::GetPerfmon() {
  return &impl_->perfmon_;
}
framework::PowerInterface *DummyFramework::GetPower() {
  return &impl_->power_;
}
framework::ProcessInterface *DummyFramework::GetProcess() {
  return &impl_->process_;
}
framework::WirelessInterface *DummyFramework::GetWireless() {
  return &impl_->wireless_;
}
framework::FileSystemInterface *DummyFramework::GetFileSystem() {
  return &impl_->filesystem_;
}
framework::AudioclipInterface *
DummyFramework::CreateAudioclip(const char *src) {
  return new framework::DummyAudioclip();
}

} // namespace ggadget
