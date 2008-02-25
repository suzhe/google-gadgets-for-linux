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

#ifndef GGADGET_GADGET_METADATA_H__
#define GGADGET_GADGET_METADATA_H__

#include <time.h>
#include <map>
#include <ggadget/common.h>
#include <ggadget/string_utils.h>

namespace ggadget {

class FileManagerInterface;
class XMLHttpRequestInterface;
template <typename R, typename P1> class Slot1;

// TODO: Define other platform specific URLs, and let version come from
// configuration.
const char kPluginsXMLRequestPrefix[] =
    "http://desktop2.google.com/desktop/plugins.xml?platform=linux&cv=5.7.0.0";

class GadgetMetadata {
 public:
  struct GadgetInfo {
    GadgetInfo() : updated_date(0) { }
    GadgetStringMap attributes;
    GadgetStringMap titles;
    GadgetStringMap descriptions;
    time_t updated_date;
  };
  typedef std::map<std::string, GadgetInfo> GadgetInfoMap;

 public:
  /**
   * Constructs a @c GadgetMetaData instance. The cached plugins.xml file will
   * be loaded into memory if it exists.
   * @param plugins_xml_path path of the local cached plugins.xml file.
   */
  GadgetMetadata(const char *plugins_xml_path);
  ~GadgetMetadata();

 public:
  /**
   * Asynchronously updates gadget metadata from the server. After a successful
   * download, the updated data will be saved into the local plugins.xml file. 
   * @param full_download if @c true, a full download will be performed.
   * @param request a newly created XMLHttpRequestInterface instance. This
   *     parameter is provided to ease unittest.
   * @param on_done (optional) will be called when this request is done. The
   *     bool parameter indicates whether the request is successful. If this
   *     parameter is provided, the caller must ensure the slot is available
   *     during the request or the life of this GadgetMetadata object.
   */ 
  void UpdateFromServer(bool full_download, XMLHttpRequestInterface *request,
                        Slot1<void, bool> *on_done);

  const GadgetInfoMap &GetAllGadgetInfo() const;

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(GadgetMetadata);
};

} // namespace ggadget

#endif // GGADGET_GADGET_METADATA_H__
