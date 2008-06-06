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

IF(GGL_BUILD_GTK_HOST)

APPLY_CONFIG(CAIRO)
APPLY_CONFIG(GTK2)

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/output/lib)

SET(SRCS
  main.cc
  sidebar_gtk_host.cc
  simple_gtk_host.cc
)

ADD_EXECUTABLE(ggl ${SRCS})
OUTPUT_EXECUTABLE(ggl)

TARGET_LINK_LIBRARIES(ggl
  ggadget
  ggadget-gtk
  ${CAIRO_LIBRARIES}
  ${GTK2_LIBRARIES}
)

ENDIF(GGL_BUILD_GTK_HOST)
