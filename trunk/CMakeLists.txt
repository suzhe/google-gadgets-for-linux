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

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
INCLUDE(Configure)

SET(LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)" )
SET(EXEC_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE PATH  "Base directory for executables and libraries" FORCE)

## the following are directories where stuff will be installed to
SET(BIN_INSTALL_DIR "${EXEC_INSTALL_PREFIX}/bin" CACHE PATH "The binary install dir (default prefix/bin)" FORCE)
SET(LIB_INSTALL_DIR "${EXEC_INSTALL_PREFIX}/lib${LIB_SUFFIX}" CACHE PATH "The subdirectory relative to the install prefix where libraries will be installed (default is /lib${LIB_SUFFIX})" FORCE)
SET(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include/ggadget" CACHE PATH "The subdirectory to the header prefix" FORCE)

ADD_SUBDIRECTORY(unittest)
ADD_SUBDIRECTORY(ggadget)
ADD_SUBDIRECTORY(hosts)
ADD_SUBDIRECTORY(third_party)
ADD_SUBDIRECTORY(extensions)
ADD_SUBDIRECTORY(gadgets)

ADD_DIR_TO_ZIP(resources output/bin/resources.gg)

INSTALL( FILES "${CMAKE_BINARY_DIR}/output/bin/resources.gg"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/share/google-gadgets"
)

INSTALL( FILES "${CMAKE_SOURCE_DIR}/resources/google-gadgets.png"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/share/pixmaps"
)
