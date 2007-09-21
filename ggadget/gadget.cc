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

#include "gadget.h"
#include "file_manager.h"
#include "gadget_consts.h"
#include "string_utils.h"
#include "script_context_interface.h"
#include "view.h"
#include "xml_utils.h"

namespace ggadget {

class Gadget::Impl {
 public:   
  Impl(ScriptRuntimeInterface *script_runtime,
       ElementFactoryInterface *element_factory,
       Gadget *owner)
      : script_runtime_(script_runtime),
        file_manager_(new FileManager()),
        main_(new View(owner, element_factory)),
        options_(new View(owner, element_factory)) {
  }

  ~Impl() { }

  const char *GetManifestInfo(const char *key) {
    GadgetStringMap::const_iterator it = manifest_info_map_.find(key);
    if (it == manifest_info_map_.end())
      return NULL;
    return it->second.c_str();
  }

  bool InitFromPath(const char *base_path) {
    if (!file_manager_->Init(base_path))
      return false;

    std::string manifest_contents;
    std::string manifest_path;
    if (!file_manager_->GetXMLFileContents(kGadgetGManifest,
                                           &manifest_contents,
                                           &manifest_path))
      return false;
    if (!ParseXMLIntoXPathMap(manifest_contents.c_str(),
                              manifest_path.c_str(),
                              kGadgetTag,
                              &manifest_info_map_))
      return false;
    // TODO: Is it necessary to check the required fields in manifest?

    if (SetupView(main_, kMainXML))
      return false;

    SetupView(options_, kOptionsXML);  // Ignore any error.

    // Start running the main view.
    return true;
  }

  bool SetupView(ViewInterface *view, const char *filename) {
    std::string main_xml_contents;
    std::string main_xml_path;
    return file_manager_->GetXMLFileContents(kMainXML,
                                             &main_xml_contents,
                                             &main_xml_path) &&
           SetupViewFromXML(main_,
                            main_xml_contents.c_str(),
                            main_xml_path.c_str());
  }

  ScriptRuntimeInterface *script_runtime_;
  FileManagerInterface *file_manager_;
  ViewInterface *main_;
  ViewInterface *options_;
  GadgetStringMap manifest_info_map_;
};

Gadget::Gadget(ScriptRuntimeInterface *script_runtime,
               ElementFactoryInterface *element_factory)
    : impl_(new Impl(script_runtime, element_factory, this)) {
}

Gadget::~Gadget() { 
  delete impl_;
  impl_ = NULL;
}

ViewInterface* Gadget::GetMainView() {
  return impl_->main_;
}

ViewInterface* Gadget::GetOptionsView() {
  return impl_->options_;
}

FileManagerInterface *Gadget::GetFileManager() {
  return impl_->file_manager_;
}

bool Gadget::InitFromPath(const char *base_path) {
  return impl_->InitFromPath(base_path);
}

const char *Gadget::GetManifestInfo(const char *key) {
  return impl_->GetManifestInfo(key);
}

} // namespace ggadget
