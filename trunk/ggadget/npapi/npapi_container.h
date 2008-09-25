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

#ifndef GGADGET_NPAPI_NPAPI_CONTAINER_H__
#define GGADGET_NPAPI_NPAPI_CONTAINER_H__

#include <ggadget/npapi/npapi_plugin.h>
#include <ggadget/basic_element.h>

namespace ggadget {
namespace npapi {

class NPContainer {
 public:
  NPContainer();
  ~NPContainer();

  NPPlugin *CreatePlugin(const char *mime_type, BasicElement *element,
                         bool xembed, ToolkitType toolkit,
                         int argc, char *argn[], char *argv[]);
  bool DestroyPlugin(NPPlugin *plugin);

 private:

  /** Create a new NPPlugin object. It's a helper for CreatePlugin. */
  static NPPlugin *DoNewPlugin(const char *mime_type, BasicElement *element,
                               const char *name, const char *description,
                               void *instance, void *plugin_funcs,
                               bool xembed, ToolkitType toolkit);

  /** Delete a NPPlugin object. It's a helper for DestroyPlugin. */
  static void DoDeletePlugin(NPPlugin *plugin);

 private:
  class Impl;
  Impl *impl_;
};

NPContainer *GetGlobalNPContainer();

} // namespace npapi
} // namespace ggadget

#endif // GGADGET_NPAPI_NPAPI_CONTAINER_H__
