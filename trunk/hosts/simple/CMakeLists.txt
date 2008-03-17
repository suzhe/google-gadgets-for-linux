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

PKGCONFIG_EX(cairo 1.0 CAIRO_LIBRARIES)
PKGCONFIG_EX(gtk+-2.0 2.8 GTK_LIBRARIES)
PKGCONFIG_EX(fontconfig 2.2 FC_LIBRARIES)

IF(UNIX)
  SET(SMJS_DEFINITIONS ${SMJS_DEFINITIONS} -DXP_UNIX)
ENDIF(UNIX)

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/third_party/smjs/js/src)
LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/output/lib)
ADD_DEFINITIONS(${SMJS_DEFINITIONS})

SET(SRCS
  main.cc
  simple_gtk_host.cc
)

ADD_EXECUTABLE(ggl ${SRCS})
OUTPUT_EXECUTABLE(ggl)

TARGET_LINK_LIBRARIES(ggl
  ggadget
  ggadget_gtk
  ${CAIRO_LIBRARIES}
  ${GTK_LIBRARIES}
  ${FC_LIBRARIES}
)
