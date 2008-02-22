#
#  Copyright 2007 Google Inc.
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

IF(UNIX)
  SET(SMJS_DEFINITIONS ${SMJS_DEFINITIONS} -DXP_UNIX)
ENDIF(UNIX)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcheck-new")

SET(SRCS
  converter.cc
  js_function_slot.cc
  js_native_wrapper.cc
  js_script_context.cc
  js_script_runtime.cc
  jscript_massager.cc
  json.cc
  native_js_wrapper.cc
  smjs_script_runtime.cc
)

SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -module")
ADD_LIBRARY(smjs-script-runtime MODULE ${SRCS})
TARGET_LINK_LIBRARIES(smjs-script-runtime ${JS_LIBRARIES} ggadget)
OUTPUT_MODULE(smjs-script-runtime)

# Make a static library for unittest.
ADD_LIBRARY(ggadget_smjs_for_test ${SRCS})
