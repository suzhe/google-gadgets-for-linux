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

#ifndef GGADGET_GADGET_IMPL_H__
#define GGADGET_GADGET_IMPL_H__

#include <map>
#include <string>
#include "string_utils.h"

namespace ggadget {

class ScriptRuntimeInterface;
class FileManagerInterface;
class ElementfactoryInterface;
class ViewInterface;

namespace internal {
  
/**
 * Interface for representing a Gadget in the Gadget API.
 */
class GadgetImpl {
 public:   
  GadgetImpl(ScriptRuntimeInterface *script_runtime,
             FileManagerInterface *file_manager,
             ViewInterface *main_view,
             ViewInterface *options_view);
  ~GadgetImpl();

  const char *GetManifestInfo(const char *key);

  bool InitFromPath(const char *base_path);
  bool SetupView(ViewInterface *view, const char *filename);

  ScriptRuntimeInterface *script_runtime_;
  FileManagerInterface *file_manager_;
  ViewInterface *main_;
  ViewInterface *options_;
  GadgetStringMap manifest_info_map_;
};

} // namespace internal

} // namespace ggadget

#endif // GGADGET_GADGET_IMPL_H__
