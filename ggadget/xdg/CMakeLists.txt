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

SET(SRCS
  desktop_entry.cc
  utilities.cc
)

ADD_SUBDIRECTORY(tests)

ADD_DEFINITIONS(-DXDG_PREFIX=_ggl_xdg)
ADD_LIBRARY(ggadget-xdg SHARED ${SRCS})
TARGET_LINK_LIBRARIES(ggadget-xdg
  ggadget
  xdgmime
  )

OUTPUT_LIBRARY(ggadget-xdg)

SET_TARGET_PROPERTIES(ggadget-xdg PROPERTIES
                              VERSION ${GGL_LIB_VERSION}
                              SOVERSION ${GGL_MAJOR_VERSION})

INSTALL( TARGETS ggadget-xdg
    RUNTIME DESTINATION ${BIN_INSTALL_DIR}
    LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
)

INSTALL(FILES
  desktop_entry.h
  utilities.h
  DESTINATION "${INCLUDE_INSTALL_DIR}/xdg" COMPONENT Devel)
