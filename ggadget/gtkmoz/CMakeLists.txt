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

PKGCONFIG_EX(gtk+-2.0 2.8 GTK_LIBRARIES)

SET(LIB_SRCS browser_element.cc)

ADD_LIBRARY(ggadget_gtkmoz SHARED ${LIB_SRCS})
TARGET_LINK_LIBRARIES(ggadget_gtkmoz ggadget ${GTK_LIBRARIES})
OUTPUT_LIBRARY(ggadget_gtkmoz)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-non-virtual-dtor")
PKGCONFIG_EX(firefox-gtkmozembed 1.5 GTKMOZEMBED_LIBRARIES)
PKGCONFIG_EX(firefox-js 1.5 JS_LIBRARIES)

SET(CHILD_SRCS
  browser_child.cc
  ../smjs/json.cc
)
ADD_EXECUTABLE(browser_child ${CHILD_SRCS})
TARGET_LINK_LIBRARIES(browser_child
  ${GTKMOZEMBED_LIBRARIES}
  ${JS_LIBRARIES}
  ${GTK_LIBRARIES}
)
OUTPUT_EXECUTABLE(browser_child)
