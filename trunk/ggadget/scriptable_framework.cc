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

#include <map>
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
#include "gadget.h"
#include "event.h"
#include "scriptable_event.h"

namespace ggadget {
namespace framework {

// Default argument list for methods that has single optional slot argument.
static const Variant kDefaultArgsForSingleSlot[] = {
  Variant(static_cast<Slot *>(NULL))
};

// Default argument list for methods whose second argument is an optional slot.
static const Variant kDefaultArgsForSecondSlot[] = {
  Variant(), Variant(static_cast<Slot *>(NULL))
};

// Implementation of ScriptableAudio
class ScriptableAudioclip : public ScriptableHelperDefault {
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

class ScriptableAudio::Impl {
 public:
  Impl(AudioInterface *audio, Gadget *gadget)
    : audio_(audio), file_manager_(NULL) {
    file_manager_ = gadget->GetFileManager();
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

    AudioclipInterface *clip = audio_->CreateAudioclip(src_str.c_str());
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

  AudioInterface *audio_;
  FileManagerInterface *file_manager_;
};

ScriptableAudio::ScriptableAudio(AudioInterface *audio,
                                 Gadget *gadget)
  : impl_(new Impl(audio, gadget)) {
  RegisterMethod("open", NewSlotWithDefaultArgs(NewSlot(impl_, &Impl::Open),
                                                kDefaultArgsForSecondSlot));
  RegisterMethod("play", NewSlotWithDefaultArgs(NewSlot(impl_, &Impl::Play),
                                                kDefaultArgsForSecondSlot));
  RegisterMethod("stop", NewSlot(impl_, &Impl::Stop));
}

ScriptableAudio::~ScriptableAudio() {
  delete impl_;
  impl_ = NULL;
}

// Implementation of ScriptableNetwork
class ScriptableWirelessAccessPoint : public ScriptableHelperDefault {
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

class ScriptableWireless : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x1838DCFED2E146F3, ScriptableInterface);
  explicit ScriptableWireless(WirelessInterface *wireless)
    : wireless_(wireless) {
    ASSERT(wireless_);
    RegisterProperty(
        "available",
        NewSlot(wireless_, &WirelessInterface::IsAvailable), NULL);
    RegisterProperty(
        "connected",
        NewSlot(wireless_, &WirelessInterface::IsConnected), NULL);
    RegisterProperty(
        "enumerateAvailableAccessPoints",
        NewSlot(this, &ScriptableWireless::EnumerateAvailableAPs), NULL);
    RegisterProperty(
        "enumerationSupported",
        NewSlot(wireless_, &WirelessInterface::EnumerationSupported), NULL);
    RegisterProperty(
        "name",
        NewSlot(wireless_, &WirelessInterface::GetName), NULL);
    RegisterProperty(
        "networkName",
        NewSlot(wireless_, &WirelessInterface::GetNetworkName), NULL);
    RegisterProperty(
        "signalStrength",
        NewSlot(wireless_, &WirelessInterface::GetSignalStrength), NULL);
    RegisterMethod(
        "connect",
        NewSlotWithDefaultArgs(NewSlot(this, &ScriptableWireless::ConnectAP),
                               kDefaultArgsForSecondSlot));
    RegisterMethod(
        "disconnect",
        NewSlotWithDefaultArgs(NewSlot(this, &ScriptableWireless::DisconnectAP),
                               kDefaultArgsForSecondSlot));
  }

  ScriptableArray *EnumerateAvailableAPs() {
    int count = wireless_->GetAPCount();
    ASSERT(count >= 0);
    Variant *aps = new Variant[count];
    for (int i = 0; i < count; i++) {
      WirelessAccessPointInterface *ap = wireless_->GetWirelessAccessPoint(i);
      aps[i] = Variant(ap ? new ScriptableWirelessAccessPoint(ap) : NULL);
    }
    return ScriptableArray::Create(aps, static_cast<size_t>(count));
  }

  WirelessAccessPointInterface *GetAPByName(const char *ap_name) {
    if (!ap_name) return NULL;

    int count = wireless_->GetAPCount();
    ASSERT(count >= 0);
    for (int i = 0; i < count; i++) {
      WirelessAccessPointInterface *ap = wireless_->GetWirelessAccessPoint(i);
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

  WirelessInterface *wireless_;
};

class ScriptableNetwork::Impl {
 public:
  Impl(NetworkInterface *network)
    : network_(network), scriptable_wireless_(network_->GetWireless()) {
  }

  NetworkInterface *network_;
  ScriptableWireless scriptable_wireless_;
};

ScriptableNetwork::ScriptableNetwork(NetworkInterface *network)
  : impl_(new Impl(network)) {
  RegisterProperty(
      "online", NewSlot(network, &NetworkInterface::IsOnline), NULL);
  RegisterProperty(
      "connectionType",
      NewSlot(network, &NetworkInterface::GetConnectionType), NULL);
  RegisterProperty(
      "physicalMediaType",
      NewSlot(network, &NetworkInterface::GetPhysicalMediaType), NULL);
  RegisterConstant("wireless", &impl_->scriptable_wireless_);
}

ScriptableNetwork::~ScriptableNetwork() {
  delete impl_;
  impl_ = NULL;
}

// Implementation of ScriptablePerfmon
class ScriptablePerfmon::Impl {
 public:
  struct Counter {
    int id;
    EventSignal signal;
  };

  Impl(PerfmonInterface *perfmon, Gadget *gadget)
    : perfmon_(perfmon), gadget_(gadget) {
    ASSERT(perfmon_);
    ASSERT(gadget_);
  }

  ~Impl() {
    for (CounterMap::iterator it = counters_.begin();
         it != counters_.end(); ++it) {
      perfmon_->RemoveCounter(it->second->id);
      delete it->second;
    }
  }

  void AddCounter(const char *path, Slot *slot) {
    ASSERT(path && *path && slot);

    std::string str_path(path);
    CounterMap::iterator it = counters_.find(str_path);
    if (it != counters_.end()) {
      // Remove the old one.
      perfmon_->RemoveCounter(it->second->id);
      delete it->second;
      counters_.erase(it);
    }

    Counter *counter = new Counter;
    static_cast<Signal*>(&counter->signal)->ConnectGeneral(slot);
    counter->id = perfmon_->AddCounter(path, NewSlot(this, &Impl::Call));

    if (counter->id >= 0)
      counters_[str_path] = counter;
    else
      delete counter;
  }

  void RemoveCounter(const char *path) {
    ASSERT(path && *path);
    std::string str_path(path);
    CounterMap::iterator it = counters_.find(str_path);
    if (it != counters_.end()) {
      perfmon_->RemoveCounter(it->second->id);
      delete it->second;
      counters_.erase(it);
    }
  }

  void Call(const char *path, const Variant &value) {
    ASSERT(path && *path);
    std::string str_path(path);
    CounterMap::iterator it = counters_.find(str_path);
    if (it != counters_.end()) {
      // FIXME: Ugly hack, to be changed after refactorying other parts.
      PerfmonEvent event(value);
      ScriptableEvent scriptable_event(&event, NULL, NULL);
      View *view = down_cast<View*>(gadget_->GetMainViewHost()->GetView());
      view->FireEvent(&scriptable_event, it->second->signal);
    }
  }

  typedef std::map<std::string, Counter *> CounterMap;
  CounterMap counters_;
  PerfmonInterface *perfmon_;
  Gadget *gadget_;
};

ScriptablePerfmon::ScriptablePerfmon(PerfmonInterface *perfmon,
                                     Gadget *gadget)
  : impl_(new Impl(perfmon, gadget)) {
  RegisterMethod("currentValue",
                 NewSlot(perfmon, &PerfmonInterface::GetCurrentValue));
  RegisterMethod("addCounter",
                 NewSlot(impl_, &Impl::AddCounter));
  RegisterMethod("removeCounter",
                 NewSlot(impl_, &Impl::RemoveCounter));
}

ScriptablePerfmon::~ScriptablePerfmon() {
  delete impl_;
  impl_ = NULL;
}

// Implementation of ScriptableProcess
class ScriptableProcess::Impl {
 public:
  Impl(ProcessInterface *process)
    : process_(process) {
    ASSERT(process_);
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
    ProcessesInterface *processes = process_->EnumerateProcesses();
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
    return JSONString(EncodeProcessInfo(process_->GetForeground()));
  }

  JSONString GetProcessInfo(int pid) {
    return JSONString(EncodeProcessInfo(process_->GetInfo(pid)));
  }

  ProcessInterface *process_;
};

ScriptableProcess::ScriptableProcess(ProcessInterface *process)
  : impl_(new Impl(process)) {
  RegisterProperty("enumerateProcesses",
                   NewSlot(impl_, &Impl::EnumerateProcesses), NULL);
  RegisterProperty("foreground",
                   NewSlot(impl_, &Impl::GetForegroundProcess), NULL);
  RegisterMethod("getInfo",
                 NewSlot(impl_, &Impl::GetProcessInfo));
}

ScriptableProcess::~ScriptableProcess() {
  delete impl_;
  impl_ = NULL;
}

// Implementation of ScriptablePower
ScriptablePower::ScriptablePower(PowerInterface *power) {
  ASSERT(power);
  RegisterProperty(
      "charging",
      NewSlot(power, &PowerInterface::IsCharging), NULL);
  RegisterProperty(
      "percentRemaining",
      NewSlot(power, &PowerInterface::GetPercentRemaining), NULL);
  RegisterProperty(
      "pluggedIn",
      NewSlot(power, &PowerInterface::IsPluggedIn), NULL);
  RegisterProperty(
      "timeRemaining",
      NewSlot(power, &PowerInterface::GetTimeRemaining), NULL);
  RegisterProperty(
      "timeTotal",
      NewSlot(power, &PowerInterface::GetTimeTotal), NULL);
}

// Implementation of ScriptableMemory
ScriptableMemory::ScriptableMemory(MemoryInterface *memory) {
  ASSERT(memory);
  RegisterProperty("free", NewSlot(memory, &MemoryInterface::GetFree), NULL);
  RegisterProperty("total", NewSlot(memory, &MemoryInterface::GetTotal), NULL);
  RegisterProperty("used", NewSlot(memory, &MemoryInterface::GetUsed), NULL);
  RegisterProperty("freePhysical",
                   NewSlot(memory, &MemoryInterface::GetFreePhysical), NULL);
  RegisterProperty("totalPhysical",
                   NewSlot(memory, &MemoryInterface::GetTotalPhysical), NULL);
  RegisterProperty("usedPhysical",
                   NewSlot(memory, &MemoryInterface::GetUsedPhysical), NULL);
}

// Implementation of ScriptableBios
ScriptableBios::ScriptableBios(MachineInterface *machine) {
  ASSERT(machine);
  RegisterProperty(
      "serialNumber",
      NewSlot(machine, &MachineInterface::GetBiosSerialNumber), NULL);
}

// Implementation of ScriptableMachine
ScriptableMachine::ScriptableMachine(MachineInterface *machine) {
  ASSERT(machine);
  RegisterProperty(
      "manufacturer",
      NewSlot(machine, &MachineInterface::GetMachineManufacturer), NULL);
  RegisterProperty(
      "model",
      NewSlot(machine, &MachineInterface::GetMachineModel), NULL);
}

// Implementation of ScriptableProcessor
ScriptableProcessor::ScriptableProcessor(MachineInterface *machine) {
  ASSERT(machine);
  RegisterProperty(
      "architecture",
      NewSlot(machine, &MachineInterface::GetProcessorArchitecture), NULL);
  RegisterProperty(
      "count",
      NewSlot(machine, &MachineInterface::GetProcessorCount), NULL);
  RegisterProperty(
      "family",
      NewSlot(machine, &MachineInterface::GetProcessorFamily), NULL);
  RegisterProperty(
      "model",
      NewSlot(machine, &MachineInterface::GetProcessorModel), NULL);
  RegisterProperty(
      "name",
      NewSlot(machine, &MachineInterface::GetProcessorName), NULL);
  RegisterProperty(
      "speed",
      NewSlot(machine, &MachineInterface::GetProcessorSpeed), NULL);
  RegisterProperty(
      "stepping",
      NewSlot(machine, &MachineInterface::GetProcessorStepping), NULL);
  RegisterProperty(
      "vendor",
      NewSlot(machine, &MachineInterface::GetProcessorVendor), NULL);
}

// Implementation of ScriptableCursor
class ScriptableCursor::Impl {
 public:
  Impl(CursorInterface *cursor) : cursor_(cursor) {
    ASSERT(cursor_);
  }

  JSONString GetPosition() {
    int x, y;
    cursor_->GetPosition(&x, &y);
    return JSONString(StringPrintf("{\"x\":%d,\"y\":%d}", x, y));
  }

  CursorInterface *cursor_;
};

ScriptableCursor::ScriptableCursor(CursorInterface *cursor)
  : impl_(new Impl(cursor)) {
  RegisterProperty("position", NewSlot(impl_, &Impl::GetPosition), NULL);
}

ScriptableCursor::~ScriptableCursor() {
  delete impl_;
  impl_ = NULL;
}

// Implementation of ScriptableScreen
class ScriptableScreen::Impl {
 public:
  Impl(ScreenInterface *screen) : screen_(screen) {
    ASSERT(screen_);
  }

  JSONString GetSize() {
    int width, height;
    screen_->GetSize(&width, &height);
    return JSONString(StringPrintf("{\"width\":%d,\"height\":%d}",
                                   width, height));
  }

  ScreenInterface *screen_;
};

ScriptableScreen::ScriptableScreen(ScreenInterface *screen)
  : impl_(new Impl(screen)) {
  RegisterProperty("size", NewSlot(impl_, &Impl::GetSize), NULL);
}

ScriptableScreen::~ScriptableScreen() {
  delete impl_;
  impl_ = NULL;
}

// Implementation of ScriptableGraphics
class ScriptableGraphics::Impl {
 public:
  Impl(Gadget *gadget) : gadget_(gadget) {
    ASSERT(gadget_);
  }

  JSONString CreatePoint() {
    return JSONString("{\"x\":0,\"y\":0}");
  }

  JSONString CreateSize() {
    return JSONString("{\"height\":0,\"width\":0}");
  }

  ScriptableImage *LoadImage(const Variant &image_src) {
    //FIXME: Ugly hack
    View *view = down_cast<View *>(gadget_->GetMainViewHost()->GetView());
    ASSERT(view);
    ImageInterface *image = view->LoadImage(image_src, false);
    return image ? new ScriptableImage(image) : NULL;
  }

  Gadget *gadget_;
};

ScriptableGraphics::ScriptableGraphics(Gadget *gadget)
  : impl_(new Impl(gadget)) {
  RegisterMethod("createPoint", NewSlot(impl_, &Impl::CreatePoint));
  RegisterMethod("createSize", NewSlot(impl_, &Impl::CreateSize));
  RegisterMethod("loadImage", NewSlot(impl_, &Impl::LoadImage));
}

ScriptableGraphics::~ScriptableGraphics() {
  delete impl_;
  impl_ = NULL;
}

} // namespace framework
} // namespace ggadget
