#
# Copyright 2008 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

IF(GGL_BUILD_LINUX_SYSTEM_FRAMEWORK)

APPLY_CONFIG(X11)

ADD_SUBDIRECTORY(tests)

SET(SRCS
  file_system.cc
  runtime.cc
  machine.cc
  memory.cc
  perfmon.cc
  power.cc
  process.cc
  network.cc
  wireless_access_point.cc
  wireless.cc
  user.cc
)
ADD_LIBRARY(ggadget-linux ${SRCS})

ADD_MODULE(linux-system-framework linux_system_framework.cc)
TARGET_LINK_LIBRARIES(linux-system-framework
  ggadget
  ggadget-dbus
  ggadget-linux
  ${X11_LIBRARIES}
)
OUTPUT_MODULE(linux-system-framework)

ENDIF(GGL_BUILD_LINUX_SYSTEM_FRAMEWORK)
