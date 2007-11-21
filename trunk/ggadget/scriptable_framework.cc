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
#include "framework_interface.h"
#include "gadget_host_interface.h"
#include "scriptable_array.h"
#include "slot.h"
#include "string_utils.h"
#include "unicode_utils.h"

namespace ggadget {

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
      : audio_(gadget_host),
        system_(gadget_host) {
  }

  class PermanentScriptable : public ScriptableHelper<ScriptableInterface> {
    DEFINE_CLASS_ID(0x47d47fe768a8496c, ScriptableInterface);
    virtual OwnershipPolicy Attach() { return NATIVE_PERMANENT; }
  };

  class ScriptableAudioclip : public ScriptableHelper<ScriptableInterface> {
   public:
    DEFINE_CLASS_ID(0xa9f42ea54e2a4d13, ScriptableInterface);
    ScriptableAudioclip(AudioclipInterface *clip)
        : clip_(clip), onstatechange_slot_(NULL) {
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
      RegisterProperty("onstatechange",
                       NewSlot(this, &ScriptableAudioclip::GetOnStateChange),
                       NewSlot(this, &ScriptableAudioclip::SetOnStateChange));
      RegisterMethod("play", NewSlot(clip_, &AudioclipInterface::Play));
      RegisterMethod("pause", NewSlot(clip_, &AudioclipInterface::Pause));
      RegisterMethod("stop", NewSlot(clip_, &AudioclipInterface::Stop));

      clip->SetOnStateChange(
          NewSlot(this, &ScriptableAudioclip::OnStateChange));
    }

    ~ScriptableAudioclip() {
      clip_->Destroy();
      clip_ = NULL;
      delete onstatechange_slot_;
      onstatechange_slot_ = NULL;
    }

    virtual OwnershipPolicy Attach() { return OWNERSHIP_TRANSFERRABLE; }
    virtual bool Detach() { delete this; return true; }

    void OnStateChange(AudioclipInterface::State state) {
      if (onstatechange_slot_) {
        Variant params[] = { Variant(this), Variant(state) };
        onstatechange_slot_->Call(2, params);
      }
    }

    void SetOnStateChange(Slot *handler) {
      if (onstatechange_slot_)
        delete onstatechange_slot_;
      onstatechange_slot_ = handler;
    }

    Slot *GetOnStateChange() {
      return onstatechange_slot_;
    }

    AudioclipInterface *clip_;
    Slot *onstatechange_slot_;
  };

  class Audio : public PermanentScriptable {
   public:
    Audio(GadgetHostInterface *gadget_host): gadget_host_(gadget_host) {
      RegisterMethod("open", NewSlotWithDefaultArgs(NewSlot(this, &Audio::Open),
                                                    kDefaultArgsForSecondSlot));
      RegisterMethod("play", NewSlotWithDefaultArgs(NewSlot(this, &Audio::Play),
                                                    kDefaultArgsForSecondSlot));
      RegisterMethod("stop", NewSlot(this, &Audio::Stop));
    }

    ScriptableAudioclip *Open(const char *src, Slot *method) {
      AudioclipInterface *clip = gadget_host_->CreateAudioclip(src);
      if (clip) {
        ScriptableAudioclip *scriptable_clip = new ScriptableAudioclip(clip);
        scriptable_clip->SetOnStateChange(method);
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

    GadgetHostInterface *gadget_host_;
  };

  class Graphics : public PermanentScriptable {
   public:
    DEFINE_CLASS_ID(0x211b114e852e4a1b, ScriptableInterface);
    Graphics() {
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
    // Just return the original image_src directly, to let the receiver
    // actually load it.
    Variant LoadImage(const Variant &image_src) {
      return image_src;
    }
  };

  class ScriptableWirelessAccessPoint
      : public ScriptableHelper<ScriptableInterface> {
   public:
    DEFINE_CLASS_ID(0xcf8c688383b54c43, ScriptableInterface);
    ScriptableWirelessAccessPoint(framework::WirelessAccessPointInterface *ap)
        : ap_(ap) {
      ASSERT(ap);
      RegisterProperty(
          "name",
          NewSlot(ap_, &framework::WirelessAccessPointInterface::GetName),
          NULL);
      RegisterProperty(
          "type",
          NewSlot(ap_, &framework::WirelessAccessPointInterface::GetType),
          NULL);
      RegisterProperty(
          "signalStrength",
          NewSlot(ap_,
                  &framework::WirelessAccessPointInterface::GetSignalStrength),
          NULL);
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

    virtual OwnershipPolicy Attach() { return OWNERSHIP_TRANSFERRABLE; }
    virtual bool Detach() { delete this; return true; }

    void Connect(Slot *method) {
      ap_->Connect(method ? new SlotProxy1<void, bool>(method) : NULL);
    }

    void Disconnect(Slot *method) {
      ap_->Disconnect(method ? new SlotProxy1<void, bool>(method) : NULL);
    }

    framework::WirelessAccessPointInterface *ap_;
  };

  class System : public PermanentScriptable {
   public:
    DEFINE_CLASS_ID(0x81227fff6f63494a, ScriptableInterface);
    System(GadgetHostInterface *gadget_host) :
        gadget_host_(gadget_host),
        framework_(gadget_host->GetFramework()) {
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
      // TODO: RegisterMethod("localTimeToUniversalTime", )

      framework::MachineInterface *machine = framework_->GetMachineInterface();
      bios_.RegisterProperty(
          "serialNumber",
          NewSlot(machine, &framework::MachineInterface::GetBiosSerialNumber),
          NULL);
      cursor_.RegisterProperty("position", NewSlot(this, &System::GetCursorPos),
                               NULL);
      machine_.RegisterProperty(
          "manufacturer",
          NewSlot(machine,
                  &framework::MachineInterface::GetMachineManufacturer),
          NULL);
      machine_.RegisterProperty(
          "model",
          NewSlot(machine, &framework::MachineInterface::GetMachineModel),
          NULL);

      framework::MemoryInterface *memory = framework_->GetMemoryInterface();
      memory_.RegisterProperty(
          "free", NewSlot(memory, &framework::MemoryInterface::GetFree), NULL);
      memory_.RegisterProperty(
          "total", NewSlot(memory, &framework::MemoryInterface::GetTotal),
          NULL);
      memory_.RegisterProperty(
          "used", NewSlot(memory, &framework::MemoryInterface::GetUsed), NULL);
      memory_.RegisterProperty(
          "freePhysical",
          NewSlot(memory, &framework::MemoryInterface::GetFreePhysical), NULL);
      memory_.RegisterProperty(
          "totalPhysical",
          NewSlot(memory, &framework::MemoryInterface::GetTotalPhysical), NULL);
      memory_.RegisterProperty(
          "usedPhysical",
          NewSlot(memory, &framework::MemoryInterface::GetUsedPhysical), NULL);

      framework::NetworkInterface *network = framework_->GetNetworkInterface();
      network_.RegisterProperty(
          "online", NewSlot(network, &framework::NetworkInterface::IsOnline),
          NULL);
      network_.RegisterProperty(
          "connectionType",
          NewSlot(network, &framework::NetworkInterface::GetConnectionType),
          NULL);
      network_.RegisterProperty(
          "physicalMediaType",
          NewSlot(network, &framework::NetworkInterface::GetPhysicalMediaType),
          NULL);
      network_.RegisterConstant("wireless", &wireless_);
      // TODO: Is there framework.system.network.wirelessaccesspoint? 

      framework::WirelessInterface *wireless =
          framework_->GetWirelessInterface();
      wireless_.RegisterProperty(
          "available",
          NewSlot(wireless, &framework::WirelessInterface::IsAvailable), NULL);
      wireless_.RegisterProperty(
          "connected",
          NewSlot(wireless, &framework::WirelessInterface::IsConnected), NULL);
      wireless_.RegisterProperty(
          "enumerateAvailableAccessPoints",
          NewSlot(this, &System::EnumerateAvailableAPs), NULL);
      wireless_.RegisterProperty(
          "enumerationSupported",
          NewSlot(wireless,
                  &framework::WirelessInterface::EnumerationSupported),
          NULL);
      wireless_.RegisterProperty(
          "name",
          NewSlot(wireless, &framework::WirelessInterface::GetName), NULL);
      wireless_.RegisterProperty(
          "networkName",
          NewSlot(wireless, &framework::WirelessInterface::GetNetworkName),
          NULL);
      wireless_.RegisterProperty(
          "signalStrength",
          NewSlot(wireless, &framework::WirelessInterface::GetSignalStrength),
          NULL);
      wireless_.RegisterMethod("connect",
          NewSlotWithDefaultArgs(NewSlot(this, &System::ConnectAP),
                                 kDefaultArgsForSecondSlot));
      wireless_.RegisterMethod("disconnect",
          NewSlotWithDefaultArgs(NewSlot(this, &System::DisconnectAP),
                                 kDefaultArgsForSecondSlot));

      framework::PerfmonInterface *perfmon = framework_->GetPerfmonInterface();
      perfmon_.RegisterMethod(
          "currentValue",
          NewSlot(perfmon, &framework::PerfmonInterface::GetCurrentValue));
      perfmon_.RegisterMethod("addCounter",
                              NewSlot(this, &System::AddPerfmonCounter));
      perfmon_.RegisterMethod("removeCounter",
                              NewSlot(this, &System::RemovePerfmonCounter));

      framework::PowerInterface *power = framework_->GetPowerInterface();
      power_.RegisterProperty(
          "charing",
          NewSlot(power, &framework::PowerInterface::IsCharging), NULL);
      power_.RegisterProperty(
          "percentRemaining",
          NewSlot(power, &framework::PowerInterface::GetPercentRemaining),
          NULL);
      power_.RegisterProperty(
          "pluggedIn",
          NewSlot(power, &framework::PowerInterface::IsPluggedIn), NULL);
      power_.RegisterProperty(
          "timeRemaining",
          NewSlot(power, &framework::PowerInterface::GetTimeRemaining), NULL);
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
          NewSlot(machine,
                  &framework::MachineInterface::GetProcessorArchitecture),
          NULL);
      processor_.RegisterProperty(
          "count",
          NewSlot(machine, &framework::MachineInterface::GetProcessorCount),
          NULL);
      processor_.RegisterProperty(
          "family",
          NewSlot(machine, &framework::MachineInterface::GetProcessorFamily),
          NULL);
      processor_.RegisterProperty(
          "model",
          NewSlot(machine, &framework::MachineInterface::GetProcessorModel),
          NULL);
      processor_.RegisterProperty(
          "name",
          NewSlot(machine, &framework::MachineInterface::GetProcessorName),
          NULL);
      processor_.RegisterProperty(
          "speed",
          NewSlot(machine, &framework::MachineInterface::GetProcessorSpeed),
          NULL);
      processor_.RegisterProperty(
          "stepping",
          NewSlot(machine, &framework::MachineInterface::GetProcessorStepping),
          NULL);
      processor_.RegisterProperty(
          "vendor",
          NewSlot(machine, &framework::MachineInterface::GetProcessorVendor),
          NULL);

      screen_.RegisterProperty("size", NewSlot(this, &System::GetScreenSize),
                               NULL);
    }

    JSONString GetCursorPos() {
      int x, y;
      gadget_host_->GetCursorPos(&x, &y);
      return JSONString(StringPrintf("{\"x\":%d,\"y\":%d}", x, y));
    }

    ScriptableArray *EnumerateAvailableAPs() {
      framework::WirelessInterface *wireless =
          framework_->GetWirelessInterface();
      ASSERT(wireless);

      int count = wireless->GetAPCount();
      ASSERT(count >= 0);
      Variant *aps = new Variant[count];
      for (int i = 0; i < count; i++) {
        framework::WirelessAccessPointInterface *ap =
            wireless->GetWirelessAccessPoint(i);
        aps[i] = Variant(ap ? new ScriptableWirelessAccessPoint(ap) : NULL);
      }
      return ScriptableArray::Create(aps, static_cast<size_t>(count), false);
    }

    framework::WirelessAccessPointInterface *GetAPByName(const char *ap_name) {
      if (!ap_name) return NULL;

      framework::WirelessInterface *wireless =
          framework_->GetWirelessInterface();
      ASSERT(wireless);

      int count = wireless->GetAPCount();
      ASSERT(count >= 0);
      for (int i = 0; i < count; i++) {
        framework::WirelessAccessPointInterface *ap =
            wireless->GetWirelessAccessPoint(i);
        if (ap) {
          if (ap->GetName() && strcmp(ap->GetName(), ap_name) == 0)
            return ap;
          ap->Destroy();
        }
      }
      return NULL;
    }

    void ConnectAP(const char *ap_name, Slot *method) {
      framework::WirelessAccessPointInterface *ap = GetAPByName(ap_name);
      if (ap) {
        ap->Connect(method ? new SlotProxy1<void, bool>(method) : NULL);
      } else {
        delete method;
      }
    }

    void DisconnectAP(const char *ap_name, Slot *method) {
      framework::WirelessAccessPointInterface *ap = GetAPByName(ap_name);
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

    std::string EncodeProcessInfo(framework::ProcessInfoInterface *proc_info) {
      if (!proc_info)
        return "null";

      const char *path = proc_info->GetExecutablePath();
      UTF16String utf16_path;
      ConvertStringUTF8ToUTF16(path, strlen(path), &utf16_path);
      return StringPrintf("{\"processId\":%d,\"executablePath\":\"%s\"}",
                          proc_info->GetProcessId(),
                          EncodeJavaScriptString(utf16_path.c_str()).c_str());
    }

    JSONString EnumerateProcesses() {
      framework::ProcessesInterface *processes =
          framework_->GetProcessInterface()->EnumerateProcesses();
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
      return JSONString(EncodeProcessInfo(framework_->GetProcessInterface()->
                                          GetForeground()));
    }

    JSONString GetProcessInfo(int pid) {
      return JSONString(EncodeProcessInfo(framework_->GetProcessInterface()->
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
    PermanentScriptable filesystem_;
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

  ScriptableArray *BrowseForFiles(const char *filter) {
    GadgetHostInterface::FilesInterface *files =
        gadget_host_->BrowseForFiles(filter);
    if (files) {
      int count = files->GetCount();
      ASSERT(count >= 0);
      Variant *filenames = new Variant[count];
      for (int i = 0; i < count; i++)
        filenames[i] = Variant(files->GetItem(i));
  
      files->Destroy();
      return ScriptableArray::Create(filenames, static_cast<size_t>(count),
                                     false);
    } else {
      return NULL;
    }
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
  RegisterMethod("BrowseForFile",
                 NewSlot(gadget_host, &GadgetHostInterface::BrowseForFile));
  RegisterMethod("BrowseForFiles", NewSlot(impl_, &Impl::BrowseForFiles));
}

ScriptableFramework::~ScriptableFramework() {
  delete impl_;
  impl_ = NULL;
}

} // namespace ggadget
