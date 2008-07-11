
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

#include <cstdio>
#include <ggadget/common.h>
#include <ggadget/main_loop_interface.h>

namespace ggadget {

static MainLoopInterface *ggl_global_main_loop_ = NULL;

bool SetGlobalMainLoop(MainLoopInterface *main_loop) {
  ASSERT(!ggl_global_main_loop_ && main_loop);
  if (!ggl_global_main_loop_ && main_loop) {
    ggl_global_main_loop_ = main_loop;
    return true;
  }
  return false;
}

MainLoopInterface *GetGlobalMainLoop() {
  if (!ggl_global_main_loop_) {
#ifdef _DEBUG
    // Don't use VERIFY, LOG etc. because this function may be called from
    // logger, and the logger has no way to handle this kind of reentrance
    // because it needs the main loop to check if the call is in main thread.
    printf("The global main loop has not been set yet.\n");
#endif
  }
  return ggl_global_main_loop_;
}

} // namespace ggadget
