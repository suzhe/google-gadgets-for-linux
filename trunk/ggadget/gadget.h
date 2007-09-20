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

#ifndef GGADGET_GADGET_H__
#define GGADGET_GADGET_H__

#include "common.h"
#include "gadget_interface.h"

namespace ggadget {

namespace internal {
  class GadgetImpl;
}

class ScriptRuntimeInterface;
class FileManagerInterface;
   
/**
 * Interface for representing a Gadget in the Desktop Gadget API.
 */
class Gadget: public GadgetInterface {
 public:   
  Gadget(ScriptRuntimeInterface *script_runtime,
         FileManagerInterface *file_manager,
         ViewInterface *main_view,
         ViewInterface *options_view);

  virtual ViewInterface *GetMainView();
   
  virtual ViewInterface *GetOptionsView();

  virtual const char *GetManifestInfo(const char *key);

  bool InitFromPath(const char *base_path);

  virtual ~Gadget();

 private:
  internal::GadgetImpl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(Gadget);
};

} // namespace ggadget

#endif // GGADGET_GADGET_H__
