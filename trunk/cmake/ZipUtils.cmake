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

#! Make a zip file containing all the files in the directory.
#! @param SOURCE the full path of the directory.
#! @param ZIP_FILE the full path of the destination zip file.
MACRO(MAKE_ZIP SOURCE ZIP_FILE)
  GET_FILENAME_COMPONENT(ZIP_NAME ${ZIP_FILE} NAME)
  ADD_CUSTOM_TARGET(${ZIP_NAME}_zip ALL
    DEPENDS ${ZIP_FILE})
  FILE(GLOB_RECURSE FILES ${SOURCE}/*)
  SET(RELATIVE_FILES)
  SET(SOURCE_FILES)
  FOREACH(FILENAME ${FILES})
    IF(NOT ${FILENAME} MATCHES /\\.svn/)
      FILE(RELATIVE_PATH RELATIVE_NAME ${SOURCE} ${FILENAME})
      SET(SOURCE_FILES ${SOURCE_FILES} ${FILENAME})
      SET(RELATIVE_FILES ${RELATIVE_FILES} ${RELATIVE_NAME})
    ENDIF(NOT ${FILENAME} MATCHES /\\.svn/)
  ENDFOREACH(FILENAME ${FILES})
  ADD_CUSTOM_COMMAND(OUTPUT ${ZIP_FILE}
    COMMAND zip -r -u ${ZIP_FILE} ${RELATIVE_FILES}
    DEPENDS ${SOURCE_FILES}
    WORKING_DIRECTORY ${SOURCE})
ENDMACRO(MAKE_ZIP SOURCE ZIP_FILE)

MACRO(OUTPUT_GG DIR_NAME)
  MAKE_ZIP(${CMAKE_CURRENT_SOURCE_DIR}/${DIR_NAME}
    ${CMAKE_BINARY_DIR}/output/${DIR_NAME}.gg)
ENDMACRO(OUTPUT_GG DIR_NAME)

MACRO(TEST_RESOURCE_GG DIR_NAME)
  MAKE_ZIP(${CMAKE_CURRENT_SOURCE_DIR}/${DIR_NAME}
    ${CMAKE_CURRENT_BINARY_DIR}/${DIR_NAME}.gg)
ENDMACRO(TEST_RESOURCE_GG DIR_NAME)
