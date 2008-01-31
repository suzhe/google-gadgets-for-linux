
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

#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <map>
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
    ASSERT_M(false, ("The global main loop has not been set yet."));
  }
  return ggl_global_main_loop_;
}

} // namespace ggadget
