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

GET_CONFIG(xulrunner-js 1.8 SMJS SMJS_FOUND)
IF(SMJS_FOUND)
  MESSAGE("Using xulrunner-js as the JavaScript library")
ELSE(SMJS_FOUND)
  GET_CONFIG(firefox2-js 2.0 SMJS SMJS_FOUND)
  IF(SMJS_FOUND)
    MESSAGE("Using firefox2-js as the JavaScript library")
  ELSE(SMJS_FOUND)
    GET_CONFIG(firefox-js 1.5 SMJS SMJS_FOUND)
    IF(SMJS_FOUND)
      MESSAGE("Using firefox-js as the JavaScript library")
    ENDIF(SMJS_FOUND)
  ENDIF(SMJS_FOUND)
ENDIF(SMJS_FOUND)

MACRO(TRY_COMPILE_SMJS _definitions _link_flags)
  TRY_COMPILE(TRY_COMPILE_SMJS_RESULT
    ${CMAKE_BINARY_DIR}/configure
    ${CMAKE_SOURCE_DIR}/cmake/test_js_threadsafe.c
    CMAKE_FLAGS
      -DINCLUDE_DIRECTORIES:STRING=${SMJS_INCLUDE_DIR}
      -DLINK_DIRECTORIES:STRING=${SMJS_LINK_DIR}
      -DCMAKE_EXE_LINKER_FLAGS:STRING=${_link_flags}
      -DLINK_LIBRARIES:STRING=${SMJS_LIBRARIES}
    COMPILE_DEFINITIONS ${_definitions}
    OUTPUT_VARIABLE TRY_COMPILE_SMJS_OUTPUT)
  # MESSAGE("'${TRY_COMPILE_SMJS_RESULT}' '${TRY_COMPILE_SMJS_OUTPUT}'")
ENDMACRO(TRY_COMPILE_SMJS _definitions _link_flags)

IF(SMJS_FOUND)
  IF(UNIX)
    SET(SMJS_DEFINITIONS "${SMJS_DEFINITIONS} -DXP_UNIX")
  ENDIF(UNIX)
  
  # TODO: Make this configurable or automatically test this.
  SET(SMJS_DEFINITIONS "${SMJS_DEFINITIONS} -DMOZILLA_1_8_BRANCH")
  
  TRY_COMPILE_SMJS(
    "${SMJS_DEFINITIONS} -DMIN_SMJS_VERSION=160 -DJS_THREADSAFE"
    "${CMAKE_EXE_LINKER_FLAGS} ${SMJS_LINK_FLAGS}")

  IF(TRY_COMPILE_SMJS_RESULT)
    SET(SMJS_DEFINITIONS "${SMJS_DEFINITIONS} -DJS_THREADSAFE")
  ELSE(TRY_COMIPLE_SMJS_RESULT) 
    TRY_COMPILE_SMJS(
      "${SMJS_DEFINITIONS} -DMIN_SMJS_VERSION=160"
      "${CMAKE_EXE_LINKER_FLAGS} ${SMJS_LINK_FLAGS}")
    IF(NOT TRY_COMPILE_SMJS_RESULT) 
      SET(SMJS_FOUND FALSE)
      MESSAGE("Failed to try run SpiderMonkey, the library version may be too low.")
    ENDIF(NOT TRY_COMPILE_SMJS_RESULT) 
  ENDIF(TRY_COMPILE_SMJS_RESULT)
ENDIF(SMJS_FOUND)
