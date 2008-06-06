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
#include <ggadget/registerable_interface.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/scriptable_framework.h>
#include <ggadget/scriptable_file_system.h>

#include "file_system.h"
#include "runtime.h"
#include "machine.h"
#include "network.h"
#include "power.h"
#include "wireless.h"
#include "memory.h"
#include "perfmon.h"
#include "process.h"
#include "wireless_access_point.h"
#include "user.h"

#define Initialize linux_system_framework_LTX_Initialize
#define Finalize linux_system_framework_LTX_Finalize
#define RegisterFrameworkExtension \
    linux_system_framework_LTX_RegisterFrameworkExtension

namespace ggadget {
namespace framework {

// To avoid naming conflicts.
namespace linux_system{

static Runtime g_runtime_;
static Machine g_machine_;
static Memory g_memory_;
static Network g_network_;
static Power g_power_;
static Process g_process_;
static FileSystem g_filesystem_;
static Perfmon g_perfmon_;
static User g_user_;

static ScriptableRuntime g_script_runtime_(&g_runtime_);
static ScriptableBios g_script_bios_(&g_machine_);
static ScriptableFileSystem g_script_filesystem_(&g_filesystem_);
static ScriptableMachine g_script_machine_(&g_machine_);
static ScriptableMemory g_script_memory_(&g_memory_);
static ScriptableNetwork g_script_network_(&g_network_);
static ScriptablePower g_script_power_(&g_power_);
static ScriptableProcess g_script_process_(&g_process_);
static ScriptableProcessor g_script_processor_(&g_machine_);
static ScriptableUser g_script_user_(&g_user_);

} // namespace linux_system
} // namespace framework
} // namespace ggadget

using namespace ggadget;
using namespace ggadget::framework;
using namespace ggadget::framework::linux_system;

extern "C" {
  bool Initialize() {
    LOG("Initialize linux_system_framework extension.");
    return true;
  }

  void Finalize() {
    LOG("Finalize linux_system_framework extension.");
  }

  bool RegisterFrameworkExtension(ScriptableInterface *framework,
                                  Gadget *gadget) {
    LOG("Register linux_system_framework extension.");
    ASSERT(framework && gadget);

    if (!framework)
      return false;

    RegisterableInterface *reg_framework = framework->GetRegisterable();

    if (!reg_framework) {
      LOG("Specified framework is not registerable.");
      return false;
    }

    reg_framework->RegisterVariantConstant("runtime",
                                           Variant(&g_script_runtime_));

    ScriptableInterface *system = NULL;
    // Gets or adds system object.
    ResultVariant prop = framework->GetProperty("system");
    if (prop.v().type() != Variant::TYPE_SCRIPTABLE) {
      // property "system" is not available or have wrong type, then add one
      // with correct type.
      // Using SharedScriptable here, so that it can be destroyed correctly
      // when framework is destroyed.
      system = new SharedScriptable();
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

    reg_system->RegisterVariantConstant("bios",
                                        Variant(&g_script_bios_));
    // TODO: Filesystem feature is disabled for now to ensure gadget security.
    // Maybe we can enable it when we have better access control mechanism.
    // reg_system->RegisterVariantConstant("filesystem",
    //                                     Variant(&g_script_filesystem_));
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
    reg_system->RegisterVariantConstant("user",
                                        Variant(&g_script_user_));

    // ScriptablePerfmon is per gadget, so create a new instance here.
    ScriptablePerfmon *script_perfmon =
        new ScriptablePerfmon(&g_perfmon_, gadget);

    reg_system->RegisterVariantConstant("perfmon", Variant(script_perfmon));

    return true;
  }
}
