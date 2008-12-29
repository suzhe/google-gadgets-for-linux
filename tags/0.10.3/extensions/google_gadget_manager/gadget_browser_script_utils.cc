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

#include <cstring>
#include <ggadget/logger.h>
#include <ggadget/gadget.h>
#include "google_gadget_manager_interface.h"

#define Initialize gadget_browser_script_utils_LTX_Initialize
#define Finalize gadget_browser_script_utils_LTX_Finalize
#define RegisterScriptExtension \
    gadget_browser_script_utils_LTX_RegisterScriptExtension

using ggadget::GadgetManagerInterface;
using ggadget::GetGadgetManager;
using ggadget::Gadget;
using ggadget::ScriptContextInterface;
using ggadget::google::GoogleGadgetManagerInterface;
using ggadget::google::kGoogleGadgetManagerTag;

extern "C" {
  bool Initialize() {
    LOGI("Initialize gadget_browser_script_utils extension.");
    return true;
  }

  void Finalize() {
    LOGI("Finalize gadget_browser_script_utils extension.");
  }

  bool RegisterScriptExtension(ScriptContextInterface *context,
                               Gadget *gadget) {
    LOGI("Register ggadget_browser_script_utils extension.");
    GadgetManagerInterface *gadget_manager = GetGadgetManager();
    if (!gadget_manager ||
        strcmp(gadget_manager->GetImplTag(), kGoogleGadgetManagerTag) != 0) {
      LOG("GoogleGadgetManager expected as the global gadget manager");
      return false;
    }

    // Can't use down_cast here, because the main library knows nothing
    // about GoogleGadgetManagerInterface.
    GoogleGadgetManagerInterface *google_gadget_manager =
        static_cast<GoogleGadgetManagerInterface *>(gadget_manager);
    return context &&
           google_gadget_manager->RegisterGadgetBrowserScriptUtils(context);
  }
}
