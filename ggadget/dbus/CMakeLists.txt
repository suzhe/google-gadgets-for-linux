#
# Copyright 2007 Google Inc.
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

ADD_SUBDIRECTORY(tests)

PKGCONFIG_EX(dbus-1 1.0 DBUS_LIBRARIES)

SET(SRCS
  dbus_utils.cc
  dbus_proxy.cc
)
ADD_LIBRARY(ggadget-dbus SHARED ${SRCS})
TARGET_LINK_LIBRARIES(ggadget-dbus ${DBUS_LIBRARIES})
OUTPUT_LIBRARY(ggadget-dbus)
