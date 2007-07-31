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

# Some commands under different platforms.
IF(WIN32)
  SET(CP_CMD copy)
  MACRO(TEST_WRAPPER WRAPPEE_BASE)
    # TODO: not implemented yet.
  ENDMACRO(TEST_WRAPPER)
ELSE(WIN32)
  SET(SED_CMD sed)
  SET(CP_CMD cp)
  SET(TEST_WRAPPER_NAME test_wrapper.sh)
  SET(MAKE_EXECUTABLE chmod +x)
  MACRO(TEST_WRAPPER WRAPPEE_BASE)
    GET_TARGET_PROPERTY(WRAPPEE_LOCATION ${WRAPPEE_BASE} LOCATION)
    ADD_CUSTOM_TARGET("${WRAPPEE_BASE}_wrap" ALL
      DEPENDS ${WRAPPEE_LOCATION}.sh)
    ADD_CUSTOM_COMMAND(OUTPUT ${WRAPPEE_LOCATION}.sh
      COMMAND ${SED_CMD}
          -e "s,%%LIBRARY_PATH%%,${CMAKE_BINARY_DIR}/output/lib,g"
          -e "s,%%WRAPPEE_BASE%%,${WRAPPEE_BASE},g"
          -e "s,%%CMAKE_CURRENT_BINARY_DIR,${CMAKE_CURRENT_BINARY_DIR},g"
          ${CMAKE_SOURCE_DIR}/utils/${TEST_WRAPPER_NAME}
          > ${WRAPPEE_LOCATION}.sh
      COMMAND ${MAKE_EXECUTABLE} ${WRAPPEE_LOCATION}.sh
      DEPENDS ${CMAKE_SOURCE_DIR}/utils/${TEST_WRAPPER_NAME})
    IF(${ARGC} GREATER 1)
      ADD_TEST(${WRAPPEE_BASE} ${WRAPPEE_LOCATION}.sh)
    ENDIF(${ARGC} GREATER 1)
  ENDMACRO(TEST_WRAPPER)

  SET(JS_TEST_WRAPPER_NAME js_test_wrapper.sh)
  MACRO(JS_TEST_WRAPPER WRAPPEE_SHELL WRAPPEE_JS)
    ADD_CUSTOM_TARGET("${WRAPPEE_JS}_wrap" ALL
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${WRAPPEE_JS}.sh)
    GET_TARGET_PROPERTY(SHELL_LOCATION ${WRAPPEE_SHELL} LOCATION)
    ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${WRAPPEE_JS}.sh
      COMMAND ${SED_CMD}
          -e "s,%%LIBRARY_PATH%%,${CMAKE_BINARY_DIR}/output/lib,g"
          -e "s,%%SHELL_LOCATION%%,${SHELL_LOCATION},g"
          -e "s,%%WRAPPEE_JS%%,${WRAPPEE_JS},g"
          -e "s,%%CMAKE_SOURCE_DIR%%,${CMAKE_SOURCE_DIR},g"
          -e "s,%%CMAKE_CURRENT_SOURCE_DIR%%,${CMAKE_CURRENT_SOURCE_DIR},g"
          ${CMAKE_SOURCE_DIR}/utils/${JS_TEST_WRAPPER_NAME}
          > ${CMAKE_CURRENT_BINARY_DIR}/${WRAPPEE_JS}.sh
      COMMAND ${MAKE_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/${WRAPPEE_JS}.sh
      DEPENDS ${CMAKE_SOURCE_DIR}/utils/${JS_TEST_WRAPPER_NAME})
    IF(${ARGC} GREATER 2)
      ADD_TEST(${WRAPPEE_JS} ${CMAKE_CURRENT_BINARY_DIR}/${WRAPPEE_JS}.sh)
    ENDIF(${ARGC} GREATER 2)
  ENDMACRO(JS_TEST_WRAPPER)
ENDIF(WIN32)

MACRO(TEST_RESOURCES)
  FOREACH(FILENAME ${ARGN})
    COPY_FILE(${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME} ${CMAKE_CURRENT_BINARY_DIR})
  ENDFOREACH(FILENAME)
ENDMACRO(TEST_RESOURCES)
