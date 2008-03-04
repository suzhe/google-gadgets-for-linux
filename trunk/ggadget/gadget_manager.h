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

#ifndef GGADGET_GADGET_MANAGER_H__
#define GGADGET_GADGET_MANAGER_H__

#include <stdint.h>
#include <ggadget/common.h>
#include <ggadget/gadgets_metadata.h>

namespace ggadget {

class Connection;
template <typename R, typename P1> class Slot1;
template <typename R> class Slot0;

/**
 * Manages gadgets.
 * Notes about gadget id and gadget instance id:
 *   - gadget id: the string id of a gadget (now the plugin shortname is used).
 *     It's stored in @c GadgetMetadata::GadgetInfo::id at runtime.
 *   - gadget instance id: an integer serial number of a gadget instance. One
 *     gadget can have multiple instances.
 */
class GadgetManager {
 public:
  /**
   * Forces an update of gadget metadata.
   * @param full_download whether the update should be full of incremental
   *     download.
   */
  void UpdateGadgetsMetadata(bool full_download);

  /**
   * Creates an instance of a gadget.
   * @param gadget_id
   * @return the gadget instance id (>=0) of the new instance, or -1 on error.
   */
  int NewGadgetInstance(const char *gadget_id);

  /**
   * Removes a gadget instance.
   * @param instance_id
   * @return @c true if succeeded.
   */
  bool RemoveGadgetInstance(int instance_id);

  /**
   * Updates a running gadget instances by reloading the gadget file.
   * Normally this is called after a gadget file is just upgraded.
   */
  void UpdateGadgetInstances(const char *gadget_id);

  /**
   * Returns name of options used to create the @c OptionsInterface instance
   * for a gadget instance.
   */
  std::string GetGadgetInstanceOptionsName(int instance_id);

  /**
   * Returns the current gadgets metadata.
   */
  const GadgetInfoMap &GetAllGadgetInfo();

  /**
   * Returns the current metadata for a gadget.
   */
  const GadgetInfo *GetGadgetInfo(const char *gadget_id);

  /**
   * Get the corresponding gadget info for a instance.
   * @param instance_id
   * @return the gadget info, or NULL if the instance is not found.
   */
  const GadgetInfo *GetGadgetInfoOfInstance(int instance_id);

  /**
   * Enunerates all gadgets instances. The callback will receive an int
   * parameter which is the gadget instance id, and can return true if it
   * wants the enumeration to continue, or false to break the enumeration.
   */  
  bool EnumerateGadgetInstances(Slot1<bool, int> *callback);

  /**
   * Gadget thumbnail caching management.
   */
  void SaveThumbnailToCache(const char *thumbnail_url, const std::string &data);
  uint64_t GetThumbnailCachedTime(const char *thumbnail_url);
  std::string LoadThumbnailFromCache(const char *thumbnail_url);

  /**
   * Checks if the gadget need to be downloaded.
   */
  bool NeedDownloadGadget(const char *gadget_id);

  /**
   * Checks if the gadget need to be updated.
   */
  bool NeedUpdateGadget(const char *gadget_id);

  /**
   * Saves gadget file content to the file system.
   * @param gadget_id
   * @param data the binary gadget file content.
   * @return @c true if succeeds.
   */
  bool SaveGadget(const char *gadget_id, const std::string &data);

  /**
   * Get the full path of the downloaded gadget file for a gadget.
   */
  std::string GetDownloadedGadgetPath(const char *gadget_id);

 public:
  /**
   * Connects to signals when a gadget instance is added, to be removed or
   * should be updated. The int parameter of the callback is the gadget
   * instance id.
   */
  Connection *ConnectOnNewGadgetInstance(Slot1<void, int> *callback);
  Connection *ConnectOnRemoveGadgetInstance(Slot1<void, int> *callback);
  Connection *ConnectOnUpdateGadgetInstance(Slot1<void, int> *callback);

  /**
   * Listens to the signal fired when the metadata of gadgets changes.
   */
  Connection *ConnectOnGadgetsMetadataChange(Slot0<void> *callback);

 public:
  /** Get the singleton instance of GadgetManager. */
  static GadgetManager *Get();

 public:
  /**
   * Compares two gadget versions.
   * @param version1 version string in "x.x.x.x" format where 'x' is an integer.
   * @param version2
   * @param[out] result on success: -1 if version1 < version2, 0 if
   *     version1 == version2, or 1 if version1 > version2.
   * @return @c false if @a version1 or @a version2 is invalid version string,
   *     or @c true on success.
   */
  static bool CompareVersion(const char *version1, const char *version2,
                             int *result);

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(GadgetManager);

  GadgetManager();
  ~GadgetManager();
};

}

#endif // GGADGET_GADGET_MANAGER_H__
