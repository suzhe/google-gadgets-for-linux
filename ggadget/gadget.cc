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
#include "gadget_impl.h"
#include "element_factory_interface.h"
#include "file_manager_interface.h"
#include "gadget_consts.h"
#include "string_utils.h"
#include "script_context_interface.h"
#include "view_interface.h"
#include "xml_utils.h"

namespace ggadget {

namespace internal {

GadgetImpl::GadgetImpl(ScriptRuntimeInterface *script_runtime,
                       FileManagerInterface *file_manager,
                       ViewInterface *main_view,
                       ViewInterface *options_view)
    : script_runtime_(script_runtime),
      file_manager_(file_manager),
      main_(main_view),
      options_(options_view) {
}

GadgetImpl::~GadgetImpl() {
}

bool GadgetImpl::SetupView(ViewInterface *view, const char *file_name) {
  std::string main_xml_contents;
  std::string main_xml_path;
  return file_manager_->GetXMLFileContents(kMainXML,
                                           &main_xml_contents,
                                           &main_xml_path) &&
         SetupViewFromXML(main_,
                          main_xml_contents.c_str(),
                          main_xml_path.c_str());
}

bool GadgetImpl::InitFromPath(const char *base_path) {
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

const char *GadgetImpl::GetManifestInfo(const char *key) {
  GadgetStringMap::const_iterator it = manifest_info_map_.find(key);
  if (it == manifest_info_map_.end())
    return NULL;
  return it->second.c_str();
}

} // namespace internal

using internal::GadgetImpl;

Gadget::Gadget(ScriptRuntimeInterface *script_runtime,
               FileManagerInterface *file_manager,
               ViewInterface *main_view,
               ViewInterface *options_view)
    : impl_(new GadgetImpl(script_runtime, file_manager,
                           main_view, options_view)) {
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

bool Gadget::InitFromPath(const char *base_path) {
  return impl_->InitFromPath(base_path);
}

const char *Gadget::GetManifestInfo(const char *key) {
  return impl_->GetManifestInfo(key);
}

} // namespace ggadget
