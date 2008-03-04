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

#include <ggadget/gadget_manager.h>
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

  ScriptableGadgetInfo(const GadgetInfo &info)
      // Must make a copy here because the info may be unavailable when
      // background update runs. 
      : info_(info) {
    RegisterConstant("id", info_.id);
    RegisterConstant("attributes", NewScriptableMap(info_.attributes));
    RegisterConstant("titles", NewScriptableMap(info_.titles));
    RegisterConstant("descriptions", NewScriptableMap(info_.descriptions));
    RegisterConstant("updated_date", Date(info_.updated_date * INT64_C(1000)));
  }

  // Allow the script to add new script properties to this object.
  virtual bool IsStrict() const { return false; }

  GadgetInfo info_;
};

// Provides utility function for the gadget browser gadget.
class GadgetBrowserScriptUtils : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x0659826090ca44b0, ScriptableInterface);

  GadgetBrowserScriptUtils()
      : gadget_manager_(GadgetManager::Get()) {
    ASSERT(gadget_manager_);
    RegisterProperty("gadgetMetadata",
        NewSlot(this, &GadgetBrowserScriptUtils::GetGadgetMetadata), NULL);
    RegisterMethod("loadThumbnailFromCache",
        NewSlot(this, &GadgetBrowserScriptUtils::LoadThumbnailFromCache));
    RegisterMethod("getThumbnailCachedDate",
        NewSlot(this, &GadgetBrowserScriptUtils::GetThumbnailCachedDate));
    RegisterMethod("saveThumbnailToCache",
        NewSlot(this, &GadgetBrowserScriptUtils::SaveThumbnailToCache));
    RegisterMethod("needDownloadGadget",
        NewSlot(gadget_manager_, &GadgetManager::NeedDownloadGadget));
    RegisterMethod("needUpdateGadget",
        NewSlot(gadget_manager_, &GadgetManager::NeedUpdateGadget));
    RegisterMethod("saveGadget",
        NewSlot(this, &GadgetBrowserScriptUtils::SaveGadget));
    RegisterMethod("addGadget",
        NewSlot(gadget_manager_, &GadgetManager::NewGadgetInstance));
  }

  ~GadgetBrowserScriptUtils() {
  }

  ScriptableArray *GetGadgetMetadata() {
    const GadgetInfoMap &map = gadget_manager_->GetAllGadgetInfo();
    Variant *array = new Variant[map.size()];
    size_t i = 0;
    for (GadgetInfoMap::const_iterator it = map.begin();
         it != map.end(); ++it, ++i) {
      array[i] = Variant(new ScriptableGadgetInfo(it->second));
    }
    return ScriptableArray::Create(array, map.size());
  }

  void SaveThumbnailToCache(const char *thumbnail_url,
                            ScriptableBinaryData *image_data) {
    if (thumbnail_url && image_data)
      gadget_manager_->SaveThumbnailToCache(thumbnail_url, image_data->data());
  }

  ScriptableBinaryData *LoadThumbnailFromCache(const char *thumbnail_url) {
    std::string data = gadget_manager_->LoadThumbnailFromCache(thumbnail_url);
    return data.empty() ? NULL : new ScriptableBinaryData(data);
  }

  Date GetThumbnailCachedDate(const char *thumbnail_url) {
    return Date(gadget_manager_->GetThumbnailCachedTime(thumbnail_url));
  }

  void SaveGadget(const char *gadget_id, ScriptableBinaryData *data) {
    if (gadget_id && data)
      gadget_manager_->SaveGadget(gadget_id, data->data());
  }

  GadgetManager *gadget_manager_;
};

GadgetBrowserScriptUtils g_utils;

} // anonymous namespace
} // namespace ggadget

#define Initialize gadget_browser_script_utils_LTX_Initialize
#define Finalize gadget_browser_script_utils_LTX_Finalize
#define RegisterScriptExtension \
    gadget_browser_script_utils_LTX_RegisterScriptExtension

extern "C" {
  bool Initialize() {
    LOG("Initialize gadget_browser_script_utils extension.");
    return true;
  }

  void Finalize() {
    LOG("Finalize gadget_browser_script_utils extension.");
  }

  bool RegisterScriptExtension(ggadget::ScriptContextInterface *context) {
    LOG("Register ggadget_browser_script_utils extension.");
    if (context) {
      if (!context->AssignFromNative(NULL, NULL, "gadgetBrowserUtils",
                                     ggadget::Variant(&ggadget::g_utils))) {
        LOG("Failed to register helper.");
        return false;
      }
      return true;
    }
    return false;
  }
}
