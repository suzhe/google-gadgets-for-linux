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

IF(GGL_BUILD_LIBGGADGET_DBUS)

APPLY_CONFIG(DBUS)

ADD_SUBDIRECTORY(tests)

SET(SRCS
  dbus_utils.cc
  dbus_proxy.cc
)
ADD_LIBRARY(ggadget-dbus SHARED ${SRCS})
TARGET_LINK_LIBRARIES(ggadget-dbus ${DBUS_LIBRARIES})
SET_TARGET_PROPERTIES(ggadget-dbus PROPERTIES
                              VERSION ${GGL_LIB_VERSION}
                              SOVERSION ${GGL_MAJOR_VERSION})

OUTPUT_LIBRARY(ggadget-dbus)

install( TARGETS ggadget-dbus
    RUNTIME DESTINATION ${BIN_INSTALL_DIR}
    LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
)

ENDIF(GGL_BUILD_LIBGGADGET_DBUS)
