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

#include "gadget_manager.h"

#include <time.h>
#include <vector>
#include "digest_utils.h"
#include "file_manager_factory.h"
#include "gadget.h"
#include "gadget_consts.h"
#include "main_loop_interface.h"
#include "options_interface.h"
#include "scriptable_holder.h"
#include "signal.h"
#include "xml_http_request_interface.h"
#include "xml_parser_interface.h"

namespace ggadget {

// Time interval of gadget metadata updates: 7 days.
static const int kGadgetsMetadataUpdateInterval = 7 * 86400 * 1000;

// First retry interval for unsuccessful gadget metadata updates.
// If first retry fails, the second retry will be scheduled after 
// 2 * kGadgetMetadataRetryInterval, and so on, until the interval becomes
// greater than @c kGadgetMetadataRetryMaxInterval.
static const int kGadgetsMetadataRetryInterval = 2 * 3600 * 1000;
static const int kGadgetsMetadataRetryMaxInterval = 86400 * 1000;

// Options key to record the last time of successful metadata update.
static const char kOptionsLastUpdateTime[] = "GadgetsMetadataLastUpdateTime";
// Options key to record the last time of trying metadata update. This value
// will be cleared when the update successfully finishes.
static const char kOptionsLastTryTime[] = "GadgetsMetadataLastTryTime";
// Options key to record the current retry timeout value.
static const char kOptionsRetryTimeout[] = "GadgetsMetadataRetryTimeout";

// Notes about inactive gadget instances:
// When the last instance of a gadget is to be removed, the instance won't be
// actually removed, but becomes inactive. When the user'd add a new instance
// of the gadget, the inactive instance will be reused, so that the last
// options data can continue to be used.

// Options key to record the current maximum instance id of gadget instances
// including active and inavtive ones.
static const char kOptionsMaxInstanceId[] = "GadgetMaxInstanceId";

// The prefix of options keys each of which records the status of gadget
// instances, including both active and inactive ones. The position number
// of the value is the instance id.
// The meaning of the options value:
//   0: An empty slot.
//   1: An active instance.
// >=2: An inactive instance. The value is the expiration score, which is
//      initially 2 when the instance is just become inactive, and will be
//      incremented on some events. If the number reaches kExpirationThreshold,
//      the instance will be actually removed (expire).
static const char kOptionsInstanceStatusPrefix[] = "GadgetInstanceStatus.";

static const int kInstanceStatusNone = 0;
static const int kInstanceStatusActive = 1;
static const int kInstanceStatusInactiveStart = 2;

// The prefix of options keys to record the corresponding gadget id of
// each instance.
static const char kOptionsInstanceGadgetIdPrefix[] = "GadgetInstanceGadgetId.";

// A hard limit of maximum number of active and inactive gadget instances.
static const int kMaxNumGadgetInstances = 128;
// The maximum expiration score before an inactive instance expires.
static const int kExpirationThreshold = 64;

static const char kDownloadedGadgetsDir[] = "profile://downloaded_gadgets/";
static const char kThumbnailCacheDir[] = "profile://thumbnails/";

// Convert a string to a valid and safe file name. Need not be inversable. 
static std::string MakeGoodFileName(const char *gadget_id) {
  std::string result(gadget_id);
  for (size_t i = 0; i < result.size(); i++) {
    char c = result[i];
    if (!isalnum(c) && c != '-' && c != '_' && c != '.' && c != '+')
      result[i] = '_';
  }
  return result;
}

class GadgetManager::Impl {
 public:
  Impl()
      : main_loop_(GetGlobalMainLoop()),
        global_options_(GetGlobalOptions()),
        file_manager_(GetGlobalFileManager()),
        last_update_time_(0), last_try_time_(0), retry_timeout_(0),
        update_timer_(0),
        full_download_(false) {
    ASSERT(main_loop_);
    ASSERT(global_options_);
    ASSERT(file_manager_);

    if (metadata_.GetAllGadgetInfo().size() == 0) {
      // Schedule an immediate update if there is no cached metadata.
      ScheduleUpdate(0);
    } else {
      ScheduleNextUpdate();
    }

    int current_max_id = 0;
    global_options_->GetValue(kOptionsMaxInstanceId).
        ConvertToInt(&current_max_id);
    if (current_max_id >= kMaxNumGadgetInstances)
      current_max_id = kMaxNumGadgetInstances - 1;

    instance_statuses_.resize(current_max_id + 1);
    for (int i = 0; i <= current_max_id; i++) {
      std::string key(kOptionsInstanceStatusPrefix);
      key += StringPrintf("%d", i);
      int status = 0;
      global_options_->GetValue(key.c_str()).ConvertToInt(&status);
      instance_statuses_[i] = status;
    }
  }

  void ScheduleNextUpdate() {
    if (last_try_time_ == 0) {
      global_options_->GetValue(kOptionsLastTryTime).
          ConvertToInt64(&last_try_time_);
    }
    if (last_try_time_ > 0) {
      // Schedule a retry update because the last update failed.
      if (retry_timeout_ == 0) {
        global_options_->GetValue(kOptionsRetryTimeout).
            ConvertToInt(&retry_timeout_);
      }
      if (retry_timeout_ <= 0 ||
          retry_timeout_ > kGadgetsMetadataRetryMaxInterval) {
        retry_timeout_ = kGadgetsMetadataRetryInterval;
      }
      ScheduleUpdate(last_try_time_ + retry_timeout_);
    } else {
      // Schedule a normal update.
      if (last_update_time_ == 0) {
        global_options_->GetValue(kOptionsLastUpdateTime).
            ConvertToInt64(&last_update_time_);
      }
      ScheduleUpdate(last_update_time_ + kGadgetsMetadataUpdateInterval);
    }
  }

  void ScheduleUpdate(int64_t time) {
    if (update_timer_) {
      main_loop_->RemoveWatch(update_timer_);
      update_timer_ = 0;
    }

    int64_t current_time = static_cast<int64_t>(main_loop_->GetCurrentTime());
    int time_diff = std::max(0, static_cast<int>(time - current_time));
    update_timer_ = main_loop_->AddTimeoutWatch(
        time_diff, new WatchCallbackSlot(NewSlot(this, &Impl::OnUpdateTimer)));
  }

  bool OnUpdateTimer(int timer) {
    UpdateGadgetsMetadata(false);
    return false;
  }

  void UpdateGadgetsMetadata(bool full_download) {
    full_download_ = full_download;
    last_try_time_ = static_cast<int64_t>(main_loop_->GetCurrentTime());
    global_options_->PutValue(kOptionsLastTryTime, Variant(last_try_time_));
    metadata_.UpdateFromServer(full_download,
                               CreateXMLHttpRequest(GetXMLParser()),
                               NewSlot(this, &Impl::OnUpdateDone));
  }

  void OnUpdateDone(bool request_success, bool parsing_success) {
    if (request_success) {
      if (parsing_success) {
        LOG("Successfully updated gadget metadata");
        last_update_time_ = static_cast<int64_t>(main_loop_->GetCurrentTime());
        last_try_time_ = -1;
        retry_timeout_ = 0;
        global_options_->PutValue(kOptionsLastTryTime, Variant(last_try_time_));
        global_options_->PutValue(kOptionsRetryTimeout,
                                  Variant(retry_timeout_));
        global_options_->PutValue(kOptionsLastUpdateTime,
                                  Variant(last_update_time_));
        return;
      }

      LOG("Succeeded to request gadget metadata update, "
          "but failed to parse the result");
      if (!full_download_) {
        // The failed partial update may be because of corrupted cached file,
        // so immediately do a full download now.
        UpdateGadgetsMetadata(true);
        return;
      }
    }

    if (retry_timeout_ == 0) {
      retry_timeout_ = kGadgetsMetadataRetryInterval;
    } else {
      retry_timeout_ = std::min(retry_timeout_ * 2,
                                kGadgetsMetadataRetryMaxInterval);
    }
    global_options_->PutValue(kOptionsRetryTimeout, Variant(retry_timeout_));
    LOG("Failed to update gadget metadata. Will retry after %dms",
        retry_timeout_);
    ScheduleNextUpdate();
  }
 
  std::string GetInstanceGadgetId(int instance_id) {
    std::string key(kOptionsInstanceGadgetIdPrefix);
    key += StringPrintf("%d", instance_id);
    std::string result;
    global_options_->GetValue(key.c_str()).ConvertToString(&result);
    return result;
  }

  void SaveInstanceGadgetId(int instance_id, const char *gadget_id) {
    std::string key(kOptionsInstanceGadgetIdPrefix);
    key += StringPrintf("%d", instance_id);
    if (gadget_id && *gadget_id)
      global_options_->PutValue(key.c_str(), Variant(gadget_id));
    else
      global_options_->Remove(key.c_str());
  }

  void SetInstanceStatus(int instance_id, int status) {
    instance_statuses_[instance_id] = status;
    std::string key(kOptionsInstanceStatusPrefix);
    key += StringPrintf("%d", instance_id);
    if (status == kInstanceStatusNone)
      global_options_->Remove(key.c_str());
    else
      global_options_->PutValue(key.c_str(), Variant(status));
  }

  // Trims the instance statuses array by removing trailing empty slots.
  void TrimInstanceStatuses() {
    int size = static_cast<int>(instance_statuses_.size());
    for (int i = size - 1; i >= 0; i--) {
      if (instance_statuses_[i] != kInstanceStatusNone) {
        if (i < size - 1) {
          instance_statuses_.resize(i + 1);
          global_options_->PutValue(kOptionsMaxInstanceId, Variant(i));
        }
      }
    }
  }

  void ActuallyRemoveInstance(int instance_id, bool remove_gadget_file) {
    SetInstanceStatus(instance_id, kInstanceStatusNone);
    OptionsInterface *instance_options =
        CreateOptions(GetGadgetInstanceOptionsName(instance_id).c_str());
    instance_options->DeleteStorage();
    // TODO: Remove the gadget file.
    delete instance_options;
  }

  void IncrementAndCheckExpirationScores() {
    int size = static_cast<int>(instance_statuses_.size());
    for (int i = 0; i < size; i++) {
      if (instance_statuses_[i] >= kInstanceStatusInactiveStart) {
        // This is an inactive instance.
        if (++instance_statuses_[i] >= kExpirationThreshold) {
          // The expriation score reaches the threshold, actually remove
          // the instance.
          ActuallyRemoveInstance(i, true);
        }
      }
    }
  }

  // Gets a lowest available instance id for a new instance.
  int GetNewInstanceId() {
    int size = static_cast<int>(instance_statuses_.size());
    for (int i = 0; i < size; i++) {
      if (instance_statuses_[i] == kInstanceStatusNone) {
        SetInstanceStatus(i, kInstanceStatusActive);
        return i;
      }
    }

    if (size < kMaxNumGadgetInstances) {
      instance_statuses_.resize(size + 1);
      global_options_->PutValue(kOptionsMaxInstanceId, Variant(size));
      SetInstanceStatus(size, kInstanceStatusActive);
      return size;
    }

    LOG("Too many gadget instances");
    return -1;
  }

  int NewGadgetInstance(const char *gadget_id) {
    // First try to find the inactive instance of of this gadget.
    int size = static_cast<int>(instance_statuses_.size());
    for (int i = 0; i < size; i++) {
      if (instance_statuses_[i] >= kInstanceStatusInactiveStart &&
          GetInstanceGadgetId(i) == gadget_id) {
        // Re-activate an inactive gadget instance.
        SetInstanceStatus(i, kInstanceStatusActive);
        new_instance_signal_(i);
        return i;
      }
    }

    // Add a pure new instance.
    int instance_id = GetNewInstanceId();
    if (instance_id < 0)
      return instance_id;

    SaveInstanceGadgetId(instance_id, gadget_id);
    new_instance_signal_(instance_id);
    return instance_id;
  }

  bool RemoveGadgetInstance(int instance_id) {
    int size = static_cast<int>(instance_statuses_.size());
    ASSERT(instance_id >= 0 && instance_id < size);
    if (instance_id < 0 || instance_id >= size ||
        instance_statuses_[instance_id] != kInstanceStatusActive)
      return false;

    // Check if this instance is the last instance of this gadget.
    bool is_last_instance = true;
    std::string gadget_id = GetInstanceGadgetId(instance_id);
    for (int i = 0; i < size; i++) {
      if (i != instance_id &&
          instance_statuses_[i] == kInstanceStatusActive &&
          GetInstanceGadgetId(i) == gadget_id) {
        is_last_instance = false;
        break;
      }
    }

    IncrementAndCheckExpirationScores();
    if (is_last_instance) {
      // Only change status to inactive for the last instance of a gadget.
      SetInstanceStatus(instance_id, kInstanceStatusInactiveStart);
    } else {
      // Actually remove the instance.
      ActuallyRemoveInstance(instance_id, false);
    }
    TrimInstanceStatuses();

    remove_instance_signal_(instance_id);
    return true;
  }

  void UpdateGadgetInstances(const char *gadget_id) {
    int size = static_cast<int>(instance_statuses_.size());
    for (int i = 0; i < size; i++) {
      if (instance_statuses_[i] == kInstanceStatusActive &&
          GetInstanceGadgetId(i) == gadget_id) {
        update_instance_signal_(i);
      }
    }
  }

  std::string GetGadgetInstanceOptionsName(int instance_id) {
    return StringPrintf("gadget-%d", instance_id);
  }

  bool EnumerateGadgetInstances(Slot1<bool, int> *callback) {
    int size = static_cast<int>(instance_statuses_.size());
    for (int i = 0; i < size; i++) {
      if (instance_statuses_[i] == kInstanceStatusActive &&
          !(*callback)(i))
        return false;
    }
    return true;
  }

  const GadgetInfo *GetGadgetInfo(const char *gadget_id) {
    const GadgetInfoMap &map = metadata_.GetAllGadgetInfo(); 
    GadgetInfoMap::const_iterator it = map.find(gadget_id);
    return it == map.end() ? NULL : &it->second;
  }

  bool NeedDownloadOrUpdateGadget(const char *gadget_id, bool failure_result) {
    const GadgetInfo *gadget_info = GetGadgetInfo(gadget_id);
    if (!gadget_info) // This should not happen.
      return failure_result;

    GadgetStringMap::const_iterator attr_it =
        gadget_info->attributes.find("type");
    if (attr_it != gadget_info->attributes.end() &&
        attr_it->second != "sidebar") {
      // We only download desktop gadgets.
      return false;
    }

    std::string path(GetDownloadedGadgetPathInternal(gadget_id));
    if (file_manager_->GetLastModifiedTime(path.c_str()) <
        gadget_info->updated_date)
      return true;

    std::string full_path = file_manager_->GetFullPath(path.c_str());
    if (full_path.empty()) // This should not happen.
      return failure_result;
    GadgetStringMap manifest;
    if (!Gadget::GetGadgetManifest(full_path.c_str(), &manifest))
      return failure_result;

    std::string local_version = manifest[kManifestVersion];
    attr_it = gadget_info->attributes.find("version");
    if (attr_it != gadget_info->attributes.end()) {
      std::string remote_version = attr_it->second;
      int compare_result;
      if (CompareVersion(local_version.c_str(), remote_version.c_str(),
                         &compare_result) &&
          compare_result < 0) {
        return true;
      }
    }
    return false;
  }

  static std::string GetDownloadedGadgetPathInternal(const char *gadget_id) {
    std::string path(kDownloadedGadgetsDir);
    path += MakeGoodFileName(gadget_id);
    path += ".gg";
    return path;
  }

  MainLoopInterface *main_loop_;
  OptionsInterface *global_options_;
  FileManagerInterface *file_manager_;
  int64_t last_update_time_, last_try_time_;
  int retry_timeout_;
  int update_timer_;
  GadgetsMetadata metadata_;
  bool full_download_;  // Records the last UpdateGadgetsMetadata mode.

  // Contains the statuses of all active and inactive gadget instances.
  typedef std::vector<int> InstanceStatuses;
  InstanceStatuses instance_statuses_;

  Signal1<void, int> new_instance_signal_;
  Signal1<void, int> remove_instance_signal_;
  Signal1<void, int> update_instance_signal_;
  Signal0<void> metadata_change_signal_;

  static GadgetManager *instance_;
};

GadgetManager *GadgetManager::Impl::instance_ = NULL;

GadgetManager::GadgetManager()
    : impl_(new Impl()) {
}

GadgetManager::~GadgetManager() {
  delete impl_;
}

GadgetManager *GadgetManager::Get() {
  if (!Impl::instance_)
    Impl::instance_ = new GadgetManager();
  return Impl::instance_;
}

void GadgetManager::UpdateGadgetsMetadata(bool full_download) {
  impl_->UpdateGadgetsMetadata(full_download);
}

int GadgetManager::NewGadgetInstance(const char *gadget_id) {
  return impl_->NewGadgetInstance(gadget_id);
}

bool GadgetManager::RemoveGadgetInstance(int instance_id) {
  return impl_->RemoveGadgetInstance(instance_id);
}

void GadgetManager::UpdateGadgetInstances(const char *gadget_id) {
  impl_->UpdateGadgetInstances(gadget_id);
}

std::string GadgetManager::GetGadgetInstanceOptionsName(int instance_id) {
  return impl_->GetGadgetInstanceOptionsName(instance_id);
}

const GadgetInfoMap &GadgetManager::GetAllGadgetInfo() {
  return impl_->metadata_.GetAllGadgetInfo();
}

const GadgetInfo *GadgetManager::GetGadgetInfo(
    const char *gadget_id) {
  return impl_->GetGadgetInfo(gadget_id);
}

const GadgetInfo *GadgetManager::GetGadgetInfoOfInstance(
    int instance_id) {
  std::string gadget_id = impl_->GetInstanceGadgetId(instance_id);
  return gadget_id.empty() ? NULL :GetGadgetInfo(gadget_id.c_str());
}

Connection *GadgetManager::ConnectOnNewGadgetInstance(
    Slot1<void, int> *callback) {
  return impl_->new_instance_signal_.Connect(callback);
}

Connection *GadgetManager::ConnectOnRemoveGadgetInstance(
    Slot1<void, int> *callback) {
  return impl_->remove_instance_signal_.Connect(callback);
}

Connection *GadgetManager::ConnectOnUpdateGadgetInstance(
    Slot1<void, int> *callback) {
  return impl_->update_instance_signal_.Connect(callback);
}

Connection *GadgetManager::ConnectOnGadgetsMetadataChange(
    Slot0<void> *callback) {
  return impl_->metadata_change_signal_.Connect(callback);
}

void GadgetManager::SaveThumbnailToCache(const char *thumbnail_url,
                                         const std::string &data) {
  if (!thumbnail_url || !*thumbnail_url || data.empty())
    return;

  std::string path(kThumbnailCacheDir);
  path += MakeGoodFileName(thumbnail_url);
  impl_->file_manager_->WriteFile(path.c_str(), data, true);
}

uint64_t GadgetManager::GetThumbnailCachedTime(const char *thumbnail_url) {
  if (!thumbnail_url || !*thumbnail_url)
    return 0;

  std::string path(kThumbnailCacheDir);
  path += MakeGoodFileName(thumbnail_url);
  return impl_->file_manager_->GetLastModifiedTime(path.c_str());
}

std::string GadgetManager::LoadThumbnailFromCache(const char *thumbnail_url) {
  if (!thumbnail_url || !*thumbnail_url)
    return std::string();

  std::string path(kThumbnailCacheDir);
  path += MakeGoodFileName(thumbnail_url);
  std::string data;
  if (impl_->file_manager_->ReadFile(path.c_str(), &data))
    return data;
  return std::string();
}

bool GadgetManager::NeedDownloadGadget(const char *gadget_id) {
  return impl_->NeedDownloadOrUpdateGadget(gadget_id, true);
}

bool GadgetManager::NeedUpdateGadget(const char *gadget_id) {
  return impl_->NeedDownloadOrUpdateGadget(gadget_id, false);
}

bool GadgetManager::SaveGadget(const char *gadget_id, const std::string &data) {
  const GadgetInfo *gadget_info = GetGadgetInfo(gadget_id);
  if (!gadget_info) // This should not happen.
    return false;

  GadgetStringMap::const_iterator attr_it =
      gadget_info->attributes.find("checksum");
  if (attr_it != gadget_info->attributes.end()) {
    std::string required_checksum;
    std::string actual_checksum;
    if (!WebSafeDecodeBase64(attr_it->second.c_str(), &required_checksum) ||
        !GenerateSHA1(data, &actual_checksum) ||
        actual_checksum != required_checksum) {
      LOG("Checksum mismatch for %s", gadget_id);
      // This checksum mismatch may be caused by an old version of plugins.xml,
      // So immediately update the metadata to ensure it is the latest. 
      UpdateGadgetsMetadata(true);
      return false;
    }
    DLOG("Checksum OK %s", gadget_id);
  }

  std::string path(Impl::GetDownloadedGadgetPathInternal(gadget_id));
  if (!impl_->file_manager_->WriteFile(path.c_str(), data, true))
    return false;

  impl_->UpdateGadgetInstances(gadget_id);
  return true;
}

std::string GadgetManager::GetDownloadedGadgetPath(const char *gadget_id) {
  return impl_->file_manager_->GetFullPath(
      Impl::GetDownloadedGadgetPathInternal(gadget_id).c_str());
}

static const int kNumVersionParts = 4;

static bool ParseVersion(const char *version,
                         int parsed_version[kNumVersionParts]) {
  char *end_ptr;
  for (int i = 0; i < kNumVersionParts; i++) {
    long v = strtol(version, &end_ptr, 10);
    if (v < 0 || v > SHRT_MAX)
      return false;
    parsed_version[i] = static_cast<int>(v);

    if (i < kNumVersionParts - 1) {
      if (*end_ptr != '.')
        return false;
      version = end_ptr + 1;
    }
  }
  return *end_ptr == '\0';
}

bool GadgetManager::CompareVersion(const char *version1, const char *version2,
                                   int *result) {
  ASSERT(result);
  int parsed_version1[kNumVersionParts], parsed_version2[kNumVersionParts];
  if (ParseVersion(version1, parsed_version1) &&
      ParseVersion(version2, parsed_version2)) {
    for (int i = 0; i < kNumVersionParts; i++) {
      if (parsed_version1[i] < parsed_version2[i]) {
        *result = -1;
        return true;
      }
      if (parsed_version1[i] > parsed_version2[i]) {
        *result = 1;
        return true;
      }
    }

    *result = 0;
    return true;
  }
  return false;
}

}  // namespace ggadget
