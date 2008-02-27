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

#include <ggadget/directory_provider_interface.h>
#include <ggadget/gadget_metadata.h>
#include <ggadget/logger.h>
#include <ggadget/script_context_interface.h>
#include <ggadget/scriptable_array.h>
#include <ggadget/scriptable_binary_data.h>
#include <ggadget/scriptable_helper.h>
#include <ggadget/scriptable_map.h>

namespace ggadget {
namespace {

class ScriptableGadgetInfo : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x61fde0b5d5b94ab4, ScriptableInterface);

  ScriptableGadgetInfo(const GadgetMetadata::GadgetInfo &info) {
    RegisterConstant("id", info.id);
    RegisterConstant("attributes", NewScriptableMap(info.attributes));
    RegisterConstant("titles", NewScriptableMap(info.titles));
    RegisterConstant("descriptions", NewScriptableMap(info.descriptions));
    RegisterConstant("updated_date", Date(info.updated_date * INT64_C(1000)));
  }

  // Allow the script to add new script properties to this object.
  virtual bool IsStrict() const { return false; }

};

// Provides utility function for the gadget browser gadget.
class GadgetBrowserScriptUtils : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x0659826090ca44b0, ScriptableInterface);

  GadgetBrowserScriptUtils()
      // TODO: Acquire gadget metadata from GadgetManager, where periodical
      // updates should be scheduled.
      : gadget_metadata_((GetDirectoryProvider()->GetProfileDirectory() +
                          "plugins.xml").c_str()) {
    RegisterProperty("gadgetMetadata",
        NewSlot(this, &GadgetBrowserScriptUtils::GetGadgetMetadata), NULL);
    RegisterMethod("loadThumbnailFromCache",
        NewSlot(this, &GadgetBrowserScriptUtils::LoadThumbnailFromCache));
    RegisterMethod("saveThumbnailToCache",
        NewSlot(this, &GadgetBrowserScriptUtils::SaveThumbnailToCache));
    RegisterMethod("needDownloadGadget",
        NewSlot(this, &GadgetBrowserScriptUtils::NeedDownloadGadget));
    RegisterMethod("saveGadget",
        NewSlot(this, &GadgetBrowserScriptUtils::SaveGadget));
    RegisterMethod("addGadget",
        NewSlot(this, &GadgetBrowserScriptUtils::AddGadget));
    RegisterMethod("updateGadget",
        NewSlot(this, &GadgetBrowserScriptUtils::UpdateGadget));
  }

  ScriptableArray *GetGadgetMetadata() {
    const GadgetMetadata::GadgetInfoMap &map =
        gadget_metadata_.GetAllGadgetInfo();
    Variant *array = new Variant[map.size()];
    size_t i = 0;
    for (GadgetMetadata::GadgetInfoMap::const_iterator it = map.begin();
         it != map.end(); ++it, ++i) {
      array[i] = Variant(new ScriptableGadgetInfo(it->second));
    }
    return ScriptableArray::Create(array, map.size());
  }

  void SaveThumbnailToCache(const char *gadget_id,
                            ScriptableBinaryData *image_data) {
    // TODO:
  }

  ScriptableBinaryData *LoadThumbnailFromCache(const char *gadget_id) {
    // TODO:
    return NULL;
  }

  bool NeedDownloadGadget(const char *gadget_id) {
    // TODO:
    return true;
  }

  void SaveGadget(const char *gadget_id, ScriptableBinaryData *data) {
    // TODO:
    LOG("Save Gadget: gadget_id=%s data_length=%d", gadget_id, data->size());
  }

  int AddGadget(const char *gadget_id) {
    // TODO:
    LOG("Add Gadget: gadget_id=%s", gadget_id);
    return 1;
  }

  int UpdateGadget(const char *gadget_id) {
    // TODO:
    LOG("Update Gadget: gadget_id=%s", gadget_id);
    return 1;
  }

  // TODO: Acquire gadget metadata from GadgetManager.
  GadgetMetadata gadget_metadata_;
};

GadgetBrowserScriptUtils *g_utils = NULL;

} // anonymous namespace
} // namespace ggadget

#define Initialize gadget_browser_script_utils_LTX_Initialize
#define Finalize gadget_browser_script_utils_LTX_Finalize
#define RegisterScriptExtension \
    gadget_browser_script_utils_LTX_RegisterScriptExtension

extern "C" {
  bool Initialize() {
    LOG("Initialize gadget_browser_script_utils extension.");
    if (!ggadget::g_utils)
      ggadget::g_utils = new ggadget::GadgetBrowserScriptUtils();
    return true;
  }

  void Finalize() {
    LOG("Finalize gadget_browser_script_utils extension.");
    delete ggadget::g_utils;
    ggadget::g_utils = NULL;
  }

  bool RegisterScriptExtension(ggadget::ScriptContextInterface *context) {
    LOG("Register ggadget_browser_script_utils extension.");
    if (context) {
      if (!context->AssignFromNative(NULL, NULL, "gadgetBrowserUtils",
                                     ggadget::Variant(ggadget::g_utils))) {
        LOG("Failed to register helper.");
        return false;
      }
      return true;
    }
    return false;
  }
}
