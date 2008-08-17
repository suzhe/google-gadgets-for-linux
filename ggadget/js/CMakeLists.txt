#
#  Copyright 2008 Google Inc.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#

ADD_SUBDIRECTORY(tests)

ADD_CUSTOM_COMMAND(OUTPUT jscript_massager.cc
  COMMAND flex -o jscript_massager.cc ${CMAKE_CURRENT_SOURCE_DIR}/jscript_massager.l
  MAIN_DEPENDENCY jscript_massager.l )

ADD_LIBRARY(ggadget-js SHARED jscript_massager.cc js_utils.cc)
TARGET_LINK_LIBRARIES(ggadget-js ggadget)

SET_TARGET_PROPERTIES(ggadget-js PROPERTIES
                              VERSION ${GGL_LIB_VERSION}
                              SOVERSION ${GGL_MAJOR_VERSION})

OUTPUT_LIBRARY(ggadget-js)

INSTALL( TARGETS ggadget-js
    RUNTIME DESTINATION ${BIN_INSTALL_DIR}
    LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
)

INSTALL(FILES
  jscript_massager.h
  js_utils.h
  DESTINATION "${INCLUDE_INSTALL_DIR}/js" COMPONENT Devel)
