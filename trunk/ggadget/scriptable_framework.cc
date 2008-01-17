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

#include "scriptable_framework.h"
#include "audioclip_interface.h"
#include "file_manager_interface.h"
#include "framework_interface.h"
#include "gadget_host_interface.h"
#include "gadget_interface.h"
#include "image_interface.h"
#include "scriptable_array.h"
#include "scriptable_file_system.h"
#include "scriptable_image.h"
#include "signals.h"
#include "string_utils.h"
#include "unicode_utils.h"
#include "view.h"
#include "view_host_interface.h"

namespace ggadget {

using framework::MachineInterface;
using framework::MemoryInterface;
using framework::NetworkInterface;
using framework::PerfmonInterface;
using framework::PowerInterface;
using framework::ProcessesInterface;
using framework::ProcessInfoInterface;
using framework::ProcessInterface;
using framework::WirelessAccessPointInterface;
using framework::WirelessInterface;
using framework::FileSystemInterface;
using framework::AudioclipInterface;

// Default argument list for methods that has single optional slot argument.
static const Variant kDefaultArgsForSingleSlot[] = {
  Variant(static_cast<Slot *>(NULL))
};

// Default argument list for methods whose second argument is an optional slot.
static const Variant kDefaultArgsForSecondSlot[] = {
  Variant(), Variant(static_cast<Slot *>(NULL))
};

class ScriptableFramework::Impl {
 public:
  Impl(GadgetHostInterface *gadget_host)
      : gadget_host_(gadget_host),
        audio_(gadget_host),
        graphics_(gadget_host),
        system_(gadget_host) {
  }

  class PermanentScriptable : public ScriptableHelperNativePermanent {
    DEFINE_CLASS_ID(0x47d47fe768a8496c, ScriptableInterface);
  };

  class ScriptableAudioclip : public ScriptableHelperOwnershipShared {
   public:
    DEFINE_CLASS_ID(0xa9f42ea54e2a4d13, ScriptableInterface);
    ScriptableAudioclip(AudioclipInterface *clip)
        : clip_(clip) {
      ASSERT(clip);
      RegisterProperty("balance",
                       NewSlot(clip_, &AudioclipInterface::GetBalance),
                       NewSlot(clip_, &AudioclipInterface::SetBalance));
      RegisterProperty("currentPosition",
                       NewSlot(clip_, &AudioclipInterface::GetCurrentPosition),
                       NewSlot(clip_, &AudioclipInterface::SetCurrentPosition));
      RegisterProperty("duration",
                       NewSlot(clip_, &AudioclipInterface::GetDuration), NULL);
      RegisterProperty("error", NewSlot(clip_, &AudioclipInterface::GetError),
                       NULL);
      RegisterProperty("src", NewSlot(clip_, &AudioclipInterface::GetSrc),
                       NewSlot(clip_, &AudioclipInterface::SetSrc));
      RegisterProperty("state", NewSlot(clip_, &AudioclipInterface::GetState),
                       NULL);
      RegisterProperty("volume", NewSlot(clip_, &AudioclipInterface::GetVolume),
                       NewSlot(clip_, &AudioclipInterface::SetVolume));
      RegisterSignal("onstatechange", &onstatechange_signal_);
      RegisterMethod("play", NewSlot(clip_, &AudioclipInterface::Play));
      RegisterMethod("pause", NewSlot(clip_, &AudioclipInterface::Pause));
      RegisterMethod("stop", NewSlot(clip_, &AudioclipInterface::Stop));

      clip->ConnectOnStateChange(
          NewSlot(this, &ScriptableAudioclip::OnStateChange));
    }

    ~ScriptableAudioclip() {
      clip_->Destroy();
      clip_ = NULL;
    }

    void OnStateChange(AudioclipInterface::State state) {
      onstatechange_signal_(this, state);
    }

    Connection *ConnectOnStateChange(Slot *slot) {
      return onstatechange_signal_.ConnectGeneral(slot);
    }

    AudioclipInterface *clip_;
    Signal2<void, ScriptableAudioclip *, AudioclipInterface::State>
        onstatechange_signal_;
  };

  class Audio : public PermanentScriptable {
   public:
    Audio(GadgetHostInterface *gadget_host)
        : framework_(gadget_host->GetFramework()),
          file_manager_(gadget_host->GetFileManager()) {
      RegisterMethod("open", NewSlotWithDefaultArgs(NewSlot(this, &Audio::Open),
                                                    kDefaultArgsForSecondSlot));
      RegisterMethod("play", NewSlotWithDefaultArgs(NewSlot(this, &Audio::Play),
                                                    kDefaultArgsForSecondSlot));
      RegisterMethod("stop", NewSlot(this, &Audio::Stop));
    }

    ScriptableAudioclip *Open(const char *src, Slot *method) {
      if (!src || !*src)
        return NULL;

      std::string src_str;
      if (strstr(src, "://")) {
        src_str = src;
      } else {
        // src may be a relative file name under the base path of the gadget.
        std::string extracted_file;
        if (!file_manager_->ExtractFile(src, &extracted_file))
          return NULL;
        src_str = "file://" + extracted_file;
      }

      AudioclipInterface *clip = framework_->CreateAudioclip(src_str.c_str());
      if (clip) {
        ScriptableAudioclip *scriptable_clip = new ScriptableAudioclip(clip);
        scriptable_clip->ConnectOnStateChange(method);
        return scriptable_clip;
      } else {
        delete method;
      }
      return NULL;
    }

    ScriptableAudioclip *Play(const char *src, Slot *method) {
      ScriptableAudioclip *clip = Open(src, method);
      if (clip)
        clip->clip_->Play();
      return clip;
    }

    void Stop(ScriptableAudioclip *clip) {
      if (clip)
        clip->clip_->Stop();
    }

    FrameworkInterface *framework_;
    FileManagerInterface *file_manager_;
  };

  class Graphics : public PermanentScriptable {
   public:
    DEFINE_CLASS_ID(0x211b114e852e4a1b, ScriptableInterface);
    Graphics(GadgetHostInterface *gadget_host)
        : gadget_host_(gadget_host) {
      RegisterMethod("createPoint", NewSlot(this, &Graphics::CreatePoint));
      RegisterMethod("createSize", NewSlot(this, &Graphics::CreateSize));
      RegisterMethod("loadImage", NewSlot(this, &Graphics::LoadImage));
    }

    JSONString CreatePoint() {
      return JSONString("{\"x\":0,\"y\":0}");
    }
    JSONString CreateSize() {
      return JSONString("{\"height\":0,\"width\":0}");
    }

    ScriptableImage *LoadImage(const Variant &image_src) {
      // Ugly implementation, because of not so elegent API
      // "framework.graphics".
      View *view = down_cast<View *>(gadget_host_->GetGadget()->
                                     GetMainViewHost()->GetView());
      ImageInterface *image = view->LoadImage(image_src, false);
      return image ? new ScriptableImage(image) : NULL;
    }

    GadgetHostInterface *gadget_host_;
  };

  class ScriptableWirelessAccessPoint : public ScriptableHelperOwnershipShared {
   public:
    DEFINE_CLASS_ID(0xcf8c688383b54c43, ScriptableInterface);
    ScriptableWirelessAccessPoint(WirelessAccessPointInterface *ap)
        : ap_(ap) {
      ASSERT(ap);
      RegisterProperty(
          "name",
          NewSlot(ap_, &WirelessAccessPointInterface::GetName), NULL);
      RegisterProperty(
          "type",
          NewSlot(ap_, &WirelessAccessPointInterface::GetType), NULL);
      RegisterProperty(
          "signalStrength",
          NewSlot(ap_, &WirelessAccessPointInterface::GetSignalStrength), NULL);
      RegisterMethod("connect",
          NewSlotWithDefaultArgs(
              NewSlot(this, &ScriptableWirelessAccessPoint::Connect),
              kDefaultArgsForSingleSlot));
      RegisterMethod("disconnect",
          NewSlotWithDefaultArgs(
              NewSlot(this, &ScriptableWirelessAccessPoint::Disconnect),
              kDefaultArgsForSingleSlot));
    }

    virtual ~ScriptableWirelessAccessPoint() {
      ap_->Destroy();
      ap_ = NULL;
    }

    void Connect(Slot *method) {
      ap_->Connect(method ? new SlotProxy1<void, bool>(method) : NULL);
    }

    void Disconnect(Slot *method) {
      ap_->Disconnect(method ? new SlotProxy1<void, bool>(method) : NULL);
    }

    WirelessAccessPointInterface *ap_;
  };

  class System : public PermanentScriptable {
   public:
    DEFINE_CLASS_ID(0x81227fff6f63494a, ScriptableInterface);
    System(GadgetHostInterface *gadget_host) :
        gadget_host_(gadget_host),
        framework_(gadget_host->GetFramework()),
        filesystem_(gadget_host->GetFramework()->GetFileSystem()) {
      RegisterConstant("bios", &bios_);
      RegisterConstant("cursor", &cursor_);
      RegisterConstant("filesystem", &filesystem_);
      RegisterConstant("machine", &machine_);
      RegisterConstant("memory", &memory_);
      RegisterConstant("network", &network_);
      RegisterConstant("perfmon", &perfmon_);
      RegisterConstant("power", &power_);
      RegisterConstant("process", &process_);
      RegisterConstant("processor", &processor_);
      RegisterConstant("screen", &screen_);
      RegisterMethod("getFileIcon",
                     NewSlot(gadget_host, &GadgetHostInterface::GetFileIcon));
      // TODO: RegisterMethod("languageCode",)
      RegisterMethod("localTimeToUniversalTime",
                     NewSlot(&LocalTimeToUniversalTime));

      MachineInterface *machine = framework_->GetMachine();
      bios_.RegisterProperty(
          "serialNumber",
          NewSlot(machine, &MachineInterface::GetBiosSerialNumber), NULL);
      cursor_.RegisterProperty(
          "position",
          NewSlot(this, &System::GetCursorPos), NULL);
      machine_.RegisterProperty(
          "manufacturer",
          NewSlot(machine, &MachineInterface::GetMachineManufacturer), NULL);
      machine_.RegisterProperty(
          "model",
          NewSlot(machine, &MachineInterface::GetMachineModel), NULL);

      MemoryInterface *memory = framework_->GetMemory();
      memory_.RegisterProperty(
          "free", NewSlot(memory, &MemoryInterface::GetFree), NULL);
      memory_.RegisterProperty(
          "total", NewSlot(memory, &MemoryInterface::GetTotal), NULL);
      memory_.RegisterProperty(
          "used", NewSlot(memory, &MemoryInterface::GetUsed), NULL);
      memory_.RegisterProperty(
          "freePhysical",
          NewSlot(memory, &MemoryInterface::GetFreePhysical), NULL);
      memory_.RegisterProperty(
          "totalPhysical",
          NewSlot(memory, &MemoryInterface::GetTotalPhysical), NULL);
      memory_.RegisterProperty(
          "usedPhysical",
          NewSlot(memory, &MemoryInterface::GetUsedPhysical), NULL);

      NetworkInterface *network = framework_->GetNetwork();
      network_.RegisterProperty(
          "online", NewSlot(network, &NetworkInterface::IsOnline), NULL);
      network_.RegisterProperty(
          "connectionType",
          NewSlot(network, &NetworkInterface::GetConnectionType), NULL);
      network_.RegisterProperty(
          "physicalMediaType",
          NewSlot(network, &NetworkInterface::GetPhysicalMediaType), NULL);
      network_.RegisterConstant("wireless", &wireless_);
      // TODO: Is there framework.system.network.wirelessaccesspoint?

      WirelessInterface *wireless = framework_->GetWireless();
      wireless_.RegisterProperty(
          "available",
          NewSlot(wireless, &WirelessInterface::IsAvailable), NULL);
      wireless_.RegisterProperty(
          "connected",
          NewSlot(wireless, &WirelessInterface::IsConnected), NULL);
      wireless_.RegisterProperty(
          "enumerateAvailableAccessPoints",
          NewSlot(this, &System::EnumerateAvailableAPs), NULL);
      wireless_.RegisterProperty(
          "enumerationSupported",
          NewSlot(wireless, &WirelessInterface::EnumerationSupported), NULL);
      wireless_.RegisterProperty(
          "name",
          NewSlot(wireless, &WirelessInterface::GetName), NULL);
      wireless_.RegisterProperty(
          "networkName",
          NewSlot(wireless, &WirelessInterface::GetNetworkName), NULL);
      wireless_.RegisterProperty(
          "signalStrength",
          NewSlot(wireless, &WirelessInterface::GetSignalStrength), NULL);
      wireless_.RegisterMethod("connect",
          NewSlotWithDefaultArgs(NewSlot(this, &System::ConnectAP),
                                 kDefaultArgsForSecondSlot));
      wireless_.RegisterMethod("disconnect",
          NewSlotWithDefaultArgs(NewSlot(this, &System::DisconnectAP),
                                 kDefaultArgsForSecondSlot));

      PerfmonInterface *perfmon = framework_->GetPerfmon();
      perfmon_.RegisterMethod(
          "currentValue",
          NewSlot(perfmon, &PerfmonInterface::GetCurrentValue));
      perfmon_.RegisterMethod("addCounter",
                              NewSlot(this, &System::AddPerfmonCounter));
      perfmon_.RegisterMethod("removeCounter",
                              NewSlot(this, &System::RemovePerfmonCounter));

      PowerInterface *power = framework_->GetPower();
      power_.RegisterProperty(
          "charing",
          NewSlot(power, &PowerInterface::IsCharging), NULL);
      power_.RegisterProperty(
          "percentRemaining",
          NewSlot(power, &PowerInterface::GetPercentRemaining), NULL);
      power_.RegisterProperty(
          "pluggedIn",
          NewSlot(power, &PowerInterface::IsPluggedIn), NULL);
      power_.RegisterProperty(
          "timeRemaining",
          NewSlot(power, &PowerInterface::GetTimeRemaining), NULL);
      power_.RegisterProperty(
          "timeTotal",
          NewSlot(power, &framework::PowerInterface::GetTimeTotal), NULL);

      process_.RegisterProperty("enumerateProcesses",
                                NewSlot(this, &System::EnumerateProcesses),
                                NULL);
      process_.RegisterProperty("foreground",
                                NewSlot(this, &System::GetForegroundProcess),
                                NULL);
      process_.RegisterMethod("getInfo",
                              NewSlot(this, &System::GetProcessInfo));

      processor_.RegisterProperty(
          "architecture",
          NewSlot(machine, &MachineInterface::GetProcessorArchitecture), NULL);
      processor_.RegisterProperty(
          "count",
          NewSlot(machine, &MachineInterface::GetProcessorCount), NULL);
      processor_.RegisterProperty(
          "family",
          NewSlot(machine, &MachineInterface::GetProcessorFamily), NULL);
      processor_.RegisterProperty(
          "model",
          NewSlot(machine, &MachineInterface::GetProcessorModel), NULL);
      processor_.RegisterProperty(
          "name",
          NewSlot(machine, &MachineInterface::GetProcessorName), NULL);
      processor_.RegisterProperty(
          "speed",
          NewSlot(machine, &MachineInterface::GetProcessorSpeed), NULL);
      processor_.RegisterProperty(
          "stepping",
          NewSlot(machine, &MachineInterface::GetProcessorStepping), NULL);
      processor_.RegisterProperty(
          "vendor",
          NewSlot(machine, &MachineInterface::GetProcessorVendor), NULL);

      screen_.RegisterProperty("size", NewSlot(this, &System::GetScreenSize),
                               NULL);
    }

    // In standard JavaScript, the Date object supports both local time and
    // UTC at the same time, and our Date object always use UTC, so this
    // function always returns the input.
    static Date LocalTimeToUniversalTime(const Date &date) {
      return date;
    }

    JSONString GetCursorPos() {
      int x, y;
      gadget_host_->GetCursorPos(&x, &y);
      return JSONString(StringPrintf("{\"x\":%d,\"y\":%d}", x, y));
    }

    ScriptableArray *EnumerateAvailableAPs() {
      WirelessInterface *wireless = framework_->GetWireless();
      ASSERT(wireless);

      int count = wireless->GetAPCount();
      ASSERT(count >= 0);
      Variant *aps = new Variant[count];
      for (int i = 0; i < count; i++) {
        WirelessAccessPointInterface *ap = wireless->GetWirelessAccessPoint(i);
        aps[i] = Variant(ap ? new ScriptableWirelessAccessPoint(ap) : NULL);
      }
      return ScriptableArray::Create(aps, static_cast<size_t>(count));
    }

    WirelessAccessPointInterface *GetAPByName(const char *ap_name) {
      if (!ap_name) return NULL;

      WirelessInterface *wireless = framework_->GetWireless();
      ASSERT(wireless);

      int count = wireless->GetAPCount();
      ASSERT(count >= 0);
      for (int i = 0; i < count; i++) {
        WirelessAccessPointInterface *ap = wireless->GetWirelessAccessPoint(i);
        if (ap) {
          if (ap->GetName() == ap_name)
            return ap;
          ap->Destroy();
        }
      }
      return NULL;
    }

    void ConnectAP(const char *ap_name, Slot *method) {
      WirelessAccessPointInterface *ap = GetAPByName(ap_name);
      if (ap) {
        ap->Connect(method ? new SlotProxy1<void, bool>(method) : NULL);
      } else {
        delete method;
      }
    }

    void DisconnectAP(const char *ap_name, Slot *method) {
      WirelessAccessPointInterface *ap = GetAPByName(ap_name);
      if (ap) {
        ap->Disconnect(method ? new SlotProxy1<void, bool>(method) : NULL);
      } else {
        delete method;
      }
    }

    void AddPerfmonCounter(const char *counter_path, Slot *script) {
      // TODO:
    }

    void RemovePerfmonCounter(const char *counter_path) {
      // TODO:
    }

    std::string EncodeProcessInfo(ProcessInfoInterface *proc_info) {
      if (!proc_info)
        return "null";

      std::string path = proc_info->GetExecutablePath();
      UTF16String utf16_path;
      ConvertStringUTF8ToUTF16(path.c_str(), path.size(), &utf16_path);
      return StringPrintf("{\"processId\":%d,\"executablePath\":\"%s\"}",
                          proc_info->GetProcessId(),
                          EncodeJavaScriptString(utf16_path.c_str()).c_str());
    }

    JSONString EnumerateProcesses() {
      ProcessesInterface *processes =
          framework_->GetProcess()->EnumerateProcesses();
      if (processes) {
        std::string json("[");
        int count = processes->GetCount();
        ASSERT(count >= 0);
        for (int i = 0; i < count; i++) {
          if (i > 0) json += ',';
          json += EncodeProcessInfo(processes->GetItem(i));
        }
        processes->Destroy();

        json += ']';
        return JSONString(json);
      } else {
        return JSONString("null");
      }
    }

    JSONString GetForegroundProcess() {
      return JSONString(EncodeProcessInfo(framework_->GetProcess()->
                                          GetForeground()));
    }

    JSONString GetProcessInfo(int pid) {
      return JSONString(EncodeProcessInfo(framework_->GetProcess()->
                                          GetInfo(pid)));
    }

    JSONString GetScreenSize() {
      int width, height;
      gadget_host_->GetScreenSize(&width, &height);
      return JSONString(StringPrintf("{\"width\":%d,\"height\":%d}",
                                     width, height));
    }

    GadgetHostInterface *gadget_host_;
    FrameworkInterface *framework_;
    PermanentScriptable bios_;
    PermanentScriptable cursor_;
    ScriptableFileSystem filesystem_;
    PermanentScriptable machine_;
    PermanentScriptable memory_;
    PermanentScriptable network_;
    PermanentScriptable wireless_;
    PermanentScriptable perfmon_;
    PermanentScriptable power_;
    PermanentScriptable process_;
    PermanentScriptable processor_;
    PermanentScriptable screen_;
  };

  std::string BrowseForFile(const char *filter) {
    std::string result;
    std::vector<std::string> files;
    if (gadget_host_->BrowseForFiles(filter, false, &files) &&
        files.size() > 0)
      result = files[0];
    return result;
  }

  ScriptableArray *BrowseForFiles(const char *filter) {
    std::vector<std::string> files;
    gadget_host_->BrowseForFiles(filter, true, &files);
    return ScriptableArray::Create(files.begin(), files.size());
  }

  GadgetHostInterface *gadget_host_;
  Audio audio_;
  Graphics graphics_;
  System system_;
};

ScriptableFramework::ScriptableFramework(GadgetHostInterface *gadget_host)
    : impl_(new Impl(gadget_host)) {
  RegisterConstant("audio", &impl_->audio_);
  RegisterConstant("graphics", &impl_->graphics_);
  RegisterConstant("system", &impl_->system_);
  RegisterMethod("BrowseForFile", NewSlot(impl_, &Impl::BrowseForFile));
  RegisterMethod("BrowseForFiles", NewSlot(impl_, &Impl::BrowseForFiles));
}

ScriptableFramework::~ScriptableFramework() {
  delete impl_;
  impl_ = NULL;
}

} // namespace ggadget
