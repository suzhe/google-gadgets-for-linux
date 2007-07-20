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
  MACRO(TEST_WRAPPER WRAPEE_BASE)
    # TODO: not implemented yet.
  ENDMACRO(TEST_WRAPPER)
ELSE(WIN32)
  SET(SED_CMD sed)
  SET(CP_CMD cp)
  SET(TEST_WRAPPER_NAME test_wrapper.sh)
  SET(MAKE_EXECUTABLE chmod +x)
  MACRO(TEST_WRAPPER WRAPEE_BASE)
    ADD_CUSTOM_TARGET("${WRAPEE_BASE}_wrap" ALL
      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${WRAPEE_BASE}.sh)
    ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${WRAPEE_BASE}.sh
      COMMAND ${SED_CMD} -e s,%%LIBRARY_PATH%%,${CMAKE_BINARY_DIR}/output/lib,g ${CMAKE_SOURCE_DIR}/utils/${TEST_WRAPPER_NAME} > ${CMAKE_CURRENT_BINARY_DIR}/${WRAPEE_BASE}.sh
      COMMAND ${MAKE_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/${WRAPEE_BASE}.sh
      DEPENDS ${CMAKE_SOURCE_DIR}/utils/${TEST_WRAPPER_NAME})
    IF(${ARGC} GREATER 1)
      ADD_TEST(${WRAPEE_BASE} ${CMAKE_CURRENT_BINARY_DIR}/${WRAPEE_BASE}.sh)
    ENDIF(${ARGC} GREATER 1)
  ENDMACRO(TEST_WRAPPER)
ENDIF(WIN32)

MACRO(TEST_RESOURCES)
  FOREACH(FILENAME ${ARGN})
    COPY_FILE(${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME} ${CMAKE_CURRENT_BINARY_DIR})
  ENDFOREACH(FILENAME)
ENDMACRO(TEST_RESOURCES)
