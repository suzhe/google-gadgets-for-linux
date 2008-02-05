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

PKGCONFIG_EX(gtk+-2.0 2.8 GTK_LIBRARIES)

SET(LIB_SRCS browser_element.cc)

ADD_LIBRARY(gtkmoz-browser-element SHARED ${LIB_SRCS})
TARGET_LINK_LIBRARIES(gtkmoz-browser-element ggadget ${GTK_LIBRARIES})
OUTPUT_MODULE(gtkmoz-browser-element)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-non-virtual-dtor")

FOREACH(GTKMOZEMBED_PACKAGE_NAME
  firefox-gtkmozembed 
  xulrunner-gtkmozembed 
)
  IF(NOT GTKMOZEMBED_FOUND)
    PKGCONFIG_EX(${GTKMOZEMBED_PACKAGE_NAME}
                 1.5
                 GTKMOZEMBED_LIBRARIES
                 GTKMOZEMBED_FOUND)
  ENDIF(NOT GTKMOZEMBED_FOUND)
ENDFOREACH(GTKMOZEMBED_PACKAGE_NAME
  firefox-gtkmozembed 
  xulrunner-gtkmozembed 
)

IF(NOT GTKMOZEMBED_FOUND)
  MESSAGE(FATAL "gtkmozembed library not found!")
ENDIF(NOT GTKMOZEMBED_FOUND)

SET(CHILD_SRCS
  browser_child.cc
  ${CMAKE_SOURCE_DIR}/extensions/smjs_script_runtime/json.cc
)
ADD_EXECUTABLE(gtkmoz-browser-child ${CHILD_SRCS})
TARGET_LINK_LIBRARIES(gtkmoz-browser-child
  ${GTKMOZEMBED_LIBRARIES}
  ${JS_LIBRARIES}
  ${GTK_LIBRARIES}
)
OUTPUT_EXECUTABLE(gtkmoz-browser-child)
