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

SET(SMJS_DEFINITIONS -DJS_THREADSAFE) # TODO: 
SET(SMJS_LIBRARIES smjs)  # TODO: 
SET(SMJS_LIBRARY )        # TODO:
SET(SMJS_INCLUDE_DIR /usr/include/smjs)     # TODO: 
IF(UNIX)
  SET(SMJS_DEFINITIONS ${SMJS_DEFINITIONS} -DXP_UNIX) # TODO: 
ENDIF(UNIX)

INCLUDE_DIRECTORIES(${SMJS_INCLUDE_DIR})
ADD_DEFINITIONS(${SMJS_DEFINITIONS})

ADD_SUBDIRECTORY(tests)

SET(SRCS
  converter.cc
  native_js_wrapper.cc
)

ADD_LIBRARY(ggadget_smjs SHARED ${SRCS})
TARGET_LINK_LIBRARIES(ggadget_smjs ${SMJS_LIBRARIES})
OUTPUT_LIBRARY(ggadget_smjs)
