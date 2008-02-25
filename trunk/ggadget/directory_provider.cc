
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

#include <ggadget/logger.h>
#include <ggadget/directory_provider_interface.h>

namespace ggadget {

static DirectoryProviderInterface *ggl_directory_provider = NULL;

bool SetDirectoryProvider(DirectoryProviderInterface *directory_provider) {
  ASSERT(!ggl_directory_provider && directory_provider);
  if (!ggl_directory_provider && directory_provider) {
    ggl_directory_provider = directory_provider;
    return true;
  }
  return false;
}

DirectoryProviderInterface *GetDirectoryProvider() {
  if (!ggl_directory_provider) {
    VERIFY_M(false, ("The directory provider has not been set yet."));
  }
  return ggl_directory_provider;
}

} // namespace ggadget
