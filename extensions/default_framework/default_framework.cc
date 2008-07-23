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

#include <ggadget/common.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/slot.h>
#include <ggadget/framework_interface.h>
#include <ggadget/file_system_interface.h>
#include <ggadget/file_manager_interface.h>
#include <ggadget/audioclip_interface.h>
#include <ggadget/locales.h>
#include <ggadget/registerable_interface.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/scriptable_framework.h>
#include <ggadget/scriptable_file_system.h>
#include <ggadget/scriptable_array.h>
#include <ggadget/system_utils.h>

#define Initialize default_framework_LTX_Initialize
#define Finalize default_framework_LTX_Finalize
#define RegisterFrameworkExtension \
    default_framework_LTX_RegisterFrameworkExtension

namespace ggadget {
namespace framework {

// To avoid naming conflicts.
namespace default_framework {

class DefaultMachine : public MachineInterface {
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

class DefaultMemory : public MemoryInterface {
 public:
  virtual int64_t GetTotal() { return 1024*1024*1024; }
  virtual int64_t GetFree() { return 1024*1024*512; }
  virtual int64_t GetUsed() { return 1024*1024*512; }
  virtual int64_t GetFreePhysical() { return 1024*1024*512; }
  virtual int64_t GetTotalPhysical() { return 1024*1024*1024; }
  virtual int64_t GetUsedPhysical() { return 1024*1024*512; }
};

class DefaultPerfmon : public PerfmonInterface {
 public:
  virtual Variant GetCurrentValue(const char *counter_path) { return Variant(0); }
  virtual int AddCounter(const char *counter_path, CallbackSlot *slot) {
    delete slot;
    return -1;
  }
  virtual void RemoveCounter(int id) { }
};

class DefaultPower : public PowerInterface {
 public:
  virtual bool IsCharging() { return false; }
  virtual bool IsPluggedIn() { return true; }
  virtual int GetPercentRemaining() { return 100; }
  virtual int GetTimeRemaining() { return 3600; }
  virtual int GetTimeTotal() { return 7200; }
};

class DefaultProcessInfo : public ProcessInfoInterface {
 public:
  virtual void Destroy() { }
  virtual int GetProcessId() const { return 1234; }
  virtual std::string GetExecutablePath() const { return "/usr/bin/default"; }
};

class DefaultProcesses : public ProcessesInterface {
 public:
  virtual void Destroy() { }
  virtual int GetCount() const { return 100; }
  virtual ProcessInfoInterface *GetItem(int index) { return &default_info_; }

 private:
  DefaultProcessInfo default_info_;
};

class DefaultProcess : public ProcessInterface {
 public:
  virtual ProcessesInterface *EnumerateProcesses() { return &default_processes_; }
  virtual ProcessInfoInterface *GetForeground() { return &default_foreground_; }
  virtual ProcessInfoInterface *GetInfo(int pid) { return &default_info_; }
 private:
  DefaultProcesses default_processes_;
  DefaultProcessInfo default_foreground_;
  DefaultProcessInfo default_info_;
};

class DefaultWirelessAccessPoint : public WirelessAccessPointInterface {
 public:
  virtual void Destroy() { }
  virtual std::string GetName() const { return "Unknown"; }
  virtual Type GetType() const {
    return WirelessAccessPointInterface::WIRELESS_TYPE_ANY;
  }
  virtual int GetSignalStrength() const { return 0; }
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

class DefaultWireless : public WirelessInterface {
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

class DefaultNetwork : public NetworkInterface {
 public:
  virtual bool IsOnline() { return true; }
  virtual ConnectionType GetConnectionType() {
    return NetworkInterface::CONNECTION_TYPE_802_3;
  }
  virtual PhysicalMediaType GetPhysicalMediaType() {
    return NetworkInterface::PHYSICAL_MEDIA_TYPE_UNSPECIFIED;
  }
  virtual WirelessInterface *GetWireless() {
    return &wireless_;
  }

  DefaultWireless wireless_;
};


class DefaultDrives : public DrivesInterface {
 public:
  virtual void Destroy() { delete this; }
  virtual int GetCount() const { return 0; }
  virtual DriveInterface *GetItem(int index) { return NULL; }
};

class DefaultDrive : public DriveInterface {
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
class DefaultFolders : public FoldersInterface {
 public:
  virtual void Destroy() { }
  virtual int GetCount() const { return 0; }
  virtual FolderInterface *GetItem(int index) { return NULL; }
};

class DefaultFolder : public FolderInterface {
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

class DefaultFiles : public FilesInterface {
 public:
  virtual void Destroy() { delete this; }
  virtual int GetCount() const { return 0; }
  virtual FileInterface *GetItem(int index) { return NULL; }
};

class DefaultFile : public FileInterface {
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

class DefaultTextStream : public TextStreamInterface {
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

class DefaultFileSystem : public FileSystemInterface {
 public:
  virtual DrivesInterface *GetDrives() { return new DefaultDrives(); }
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
    return new DefaultDrive();
  }
  virtual FileInterface *GetFile(const char *file_path) {
    return new DefaultFile();
  }
  virtual FolderInterface *GetFolder(const char *folder_path) {
    return new DefaultFolder();
  }
  virtual FolderInterface *GetSpecialFolder(SpecialFolder special_folder) {
    return new DefaultFolder();
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
    return new DefaultFolder();
  }
  virtual TextStreamInterface *CreateTextFile(const char *filename,
                                              bool overwrite,
                                              bool unicode) {
    return new DefaultTextStream();
  }
  virtual TextStreamInterface *OpenTextFile(const char *filename,
                                            IOMode mode,
                                            bool create,
                                            Tristate format) {
    return new DefaultTextStream();
  }
  virtual TextStreamInterface *GetStandardStream(StandardStreamType type,
                                                 bool unicode) {
    return new DefaultTextStream();
  }
  virtual std::string GetFileVersion(const char *filename) {
    return "";
  }
};

class DefaultAudioclip : public AudioclipInterface {
 public:
  virtual void Destroy() { delete this; }
  virtual int GetBalance() const { return 0; }
  virtual void SetBalance(int balance) { }
  virtual int GetCurrentPosition() const { return 0; }
  virtual void SetCurrentPosition(int position) { }
  virtual int GetDuration() const { return 100; }
  virtual ErrorCode GetError() const { return SOUND_ERROR_NO_ERROR; }
  virtual std::string GetSrc() const { return ""; }
  virtual void SetSrc(const char *src) { }
  virtual State GetState() const { return SOUND_STATE_PLAYING; }
  virtual int GetVolume() const { return 100; }
  virtual void SetVolume(int volume) { }
  virtual void Play() { }
  virtual void Pause() { }
  virtual void Stop() { }
  virtual Connection *ConnectOnStateChange(OnStateChangeHandler *handler) {
    delete handler;
    return NULL;
  }
};

class DefaultAudio : public AudioInterface {
 public:
  virtual AudioclipInterface * CreateAudioclip(const char *src)  {
    return new DefaultAudioclip();
  }
};

class DefaultRuntime : public RuntimeInterface {
 public:
  virtual std::string GetAppName() const {
    return "Google Desktop";
  }
  virtual std::string GetAppVersion() const {
    return GGL_API_VERSION;
  }
  virtual std::string GetOSName() const {
    return "";
  }
  virtual std::string GetOSVersion() const {
    return "";
  }
};

class DefaultCursor : public CursorInterface {
 public:
  virtual void GetPosition(int *x, int *y)  {
    if (x) *x = 0;
    if (y) *y = 0;
  }
};

class DefaultScreen : public ScreenInterface {
 public:
  virtual void GetSize(int *width, int *height) {
    if (width) *width = 1024;
    if (height) *height = 768;
  }
};

class DefaultUser : public UserInterface {
 public:
  virtual bool IsUserIdle() {
    return false;
  }
  virtual void SetIdlePeriod(time_t period) {
  }
  virtual time_t GetIdlePeriod() const {
    return 0;
  }
};

static DefaultMachine g_machine_;
static DefaultMemory g_memory_;
static DefaultNetwork g_network_;
static DefaultPower g_power_;
static DefaultProcess g_process_;
static DefaultFileSystem g_filesystem_;
static DefaultAudio g_audio_;
static DefaultRuntime g_runtime_;
static DefaultCursor g_cursor_;
static DefaultScreen g_screen_;
static DefaultPerfmon g_perfmon_;
static DefaultUser g_user_;

static ScriptableBios g_script_bios_(&g_machine_);
static ScriptableCursor g_script_cursor_(&g_cursor_);
static ScriptableMachine g_script_machine_(&g_machine_);
static ScriptableMemory g_script_memory_(&g_memory_);
static ScriptableNetwork g_script_network_(&g_network_);
static ScriptablePower g_script_power_(&g_power_);
static ScriptableProcess g_script_process_(&g_process_);
static ScriptableProcessor g_script_processor_(&g_machine_);
static ScriptableScreen g_script_screen_(&g_screen_);
static ScriptableUser g_script_user_(&g_user_);

static std::string DefaultGetFileIcon(const char *filename) {
  return std::string("");
}

static std::string DefaultBrowseForFile(const char *filter) {
  return std::string("");
}

static ScriptableArray *DefaultBrowseForFiles(const char *filter) {
  return new ScriptableArray();
}

static Date DefaultLocalTimeToUniversalTime(const Date &date) {
  return date;
}

static bool DefaultOpenURL(const char *url) {
  LOG("Don't know how to open url.");
  return false;
}

} // namespace default_framework
} // namespace framework
} // namespace ggadget

using namespace ggadget;
using namespace ggadget::framework;
using namespace ggadget::framework::default_framework;

extern "C" {
  bool Initialize() {
    LOGI("Initialize default_framework extension.");
    return true;
  }

  void Finalize() {
    LOGI("Finalize default_framework extension.");
  }

  bool RegisterFrameworkExtension(ScriptableInterface *framework,
                                  Gadget *gadget) {
    LOGI("Register default_framework extension.");
    ASSERT(framework && gadget);

    if (!framework)
      return false;

    RegisterableInterface *reg_framework = framework->GetRegisterable();

    if (!reg_framework) {
      LOG("Specified framework is not registerable.");
      return false;
    }

    // ScriptableAudio is per gadget, so create a new instance here.
    ScriptableAudio *script_audio = new ScriptableAudio(&g_audio_, gadget);
    reg_framework->RegisterVariantConstant("audio", Variant(script_audio));
    reg_framework->RegisterMethod("BrowseForFile",
                                  NewSlot(DefaultBrowseForFile));
    reg_framework->RegisterMethod("BrowseForFiles",
                                  NewSlot(DefaultBrowseForFiles));
    reg_framework->RegisterMethod("openUrl", NewSlot(DefaultOpenURL));

    // ScriptableGraphics is per gadget, so create a new instance here.
    ScriptableGraphics *script_graphics = new ScriptableGraphics(gadget);
    reg_framework->RegisterVariantConstant("graphics",
                                           Variant(script_graphics));

    reg_framework->RegisterVariantConstant("runtime", Variant(&g_runtime_));

    ScriptableInterface *system = NULL;
    // Gets or adds system object.
    ResultVariant prop = framework->GetProperty("system");
    if (prop.v().type() != Variant::TYPE_SCRIPTABLE) {
      // property "system" is not available or have wrong type, then add one
      // with correct type.
      // Using SharedScriptable here, so that it can be destroyed correctly
      // when framework is destroyed.
      system = new SharedScriptable<UINT64_C(0x002bf7e456d94f52)>();
      reg_framework->RegisterVariantConstant("system", Variant(system));
    } else {
      system = VariantValue<ScriptableInterface *>()(prop.v());
    }

    if (!system) {
      LOG("Failed to retrieve or add framework.system object.");
      return false;
    }

    RegisterableInterface *reg_system = system->GetRegisterable();
    if (!reg_system) {
      LOG("framework.system object is not registerable.");
      return false;
    }

    // ScriptableFileSystem is per gadget, so create a new instance here.
    ScriptableFileSystem *script_filesystem =
        new ScriptableFileSystem(&g_filesystem_, gadget);
    reg_system->RegisterVariantConstant("filesystem",
                                        Variant(script_filesystem));

    reg_system->RegisterVariantConstant("bios",
                                        Variant(&g_script_bios_));
    reg_system->RegisterVariantConstant("cursor",
                                        Variant(&g_script_cursor_));
    reg_system->RegisterVariantConstant("machine",
                                        Variant(&g_script_machine_));
    reg_system->RegisterVariantConstant("memory",
                                        Variant(&g_script_memory_));
    reg_system->RegisterVariantConstant("network",
                                        Variant(&g_script_network_));
    reg_system->RegisterVariantConstant("power",
                                        Variant(&g_script_power_));
    reg_system->RegisterVariantConstant("process",
                                        Variant(&g_script_process_));
    reg_system->RegisterVariantConstant("processor",
                                        Variant(&g_script_processor_));
    reg_system->RegisterVariantConstant("screen",
                                        Variant(&g_script_screen_));
    reg_system->RegisterVariantConstant("user",
                                        Variant(&g_script_user_));

    reg_system->RegisterMethod("getFileIcon", NewSlot(DefaultGetFileIcon));
    reg_system->RegisterMethod("languageCode", NewSlot(GetSystemLocaleName));
    reg_system->RegisterMethod("localTimeToUniversalTime",
                               NewSlot(DefaultLocalTimeToUniversalTime));

    // ScriptablePerfmon is per gadget, so create a new instance here.
    ScriptablePerfmon *script_perfmon =
        new ScriptablePerfmon(&g_perfmon_, gadget);

    reg_system->RegisterVariantConstant("perfmon", Variant(script_perfmon));

    return true;
  }
}
