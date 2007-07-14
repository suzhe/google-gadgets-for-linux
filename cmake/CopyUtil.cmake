#
# Copyright 2007 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
#
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Some commands under different platforms.
IF(WIN32)
  SET(CP_CMD copy)
ELSE(WIN32)
  SET(CP_CMD cp -a)
ENDIF(WIN32)

#! Copy the file to the specified location.
#! @param SOURCE the full path of the file.
#! @param DEST_DIR the full path of the destination directory.
#! @paramoptional NEW_NAME copy to the new directory with the new name.
MACRO(COPY_FILE SOURCE DEST_DIR)
  FILE(MAKE_DIRECTORY ${DEST_DIR})
  IF(${ARGC} GREATER 2)
    SET(DEST_NAME ${ARGV2})
  ELSE(${ARGC} GREATER 2)
    GET_FILENAME_COMPONENT(DEST_NAME ${SOURCE} NAME)
  ENDIF(${ARGC} GREATER 2)
  ADD_CUSTOM_TARGET(${DEST_NAME}_copy ALL
    DEPENDS ${DEST_DIR}/${DEST_NAME})
  FILE(TO_NATIVE_PATH ${SOURCE} NATIVE_TARGET_LOCATION)
  FILE(TO_NATIVE_PATH ${DEST_DIR}/${DEST_NAME} NATIVE_NEW_TARGET_LOCATION)
  ADD_CUSTOM_COMMAND(OUTPUT ${DEST_DIR}/${DEST_NAME}
    COMMAND ${CP_CMD} ${NATIVE_TARGET_LOCATION} ${NATIVE_NEW_TARGET_LOCATION})
ENDMACRO(COPY_FILE SOURCE DEST_DIR)

#! Copy the directory to the specified location.
#! @param SOURCE the full path of the directory.
#! @param DEST_DIR the full path of the destination directory.
MACRO(COPY_DIR SOURCE DEST_DIR)
  FILE(GLOB_RECURSE FILES RELATIVE ${SOURCE} ${SOURCE}/*)
  GET_FILENAME_COMPONENT(SOURCE_DIR_NAME ${SOURCE} NAME)
  FOREACH(FILENAME ${FILES})
    GET_FILENAME_COMPONENT(DEST_FILE_DIR ${DEST_DIR}/${SOURCE_DIR_NAME}/${FILENAME} PATH)
    COPY_FILE(${SOURCE}/${FILENAME} ${DEST_FILE_DIR})
  ENDFOREACH(FILENAME ${FILES})
ENDMACRO(COPY_DIR SOURCE DEST_DIR)

#! Copy the target to the specified location.
#! @param TARGET_NAME the name of the target.
#! @param DEST_DIR the full path of the destination directory.
#! @paramoptional NEW_NAME copy to the new directory with the new name.
MACRO(COPY_TARGET TARGET_NAME DEST_DIR)
  FILE(MAKE_DIRECTORY ${DEST_DIR})
  GET_TARGET_PROPERTY(TARGET_LOCATION ${TARGET_NAME} LOCATION)
  GET_FILENAME_COMPONENT(TARGET_OUTPUT ${TARGET_LOCATION} NAME)
  FILE(TO_NATIVE_PATH ${TARGET_LOCATION} NATIVE_TARGET_LOCATION)
  FILE(TO_NATIVE_PATH ${DEST_DIR} NATIVE_NEW_TARGET_LOCATION)
  IF(${ARGC} GREATER 2)
    ADD_CUSTOM_TARGET(${ARGV2}_copy ALL
      DEPENDS ${DEST_DIR}/${ARGV2})
    ADD_CUSTOM_COMMAND(OUTPUT ${DEST_DIR}/${ARGV2}
      COMMAND ${CP_CMD} ${NATIVE_TARGET_LOCATION} ${NATIVE_NEW_TARGET_LOCATION}/${ARGV2}
      DEPENDS ${TARGET_NAME})
  ELSE(${ARGC} GREATER 2)
    ADD_CUSTOM_TARGET(${TARGET_OUTPUT}_copy ALL
      DEPENDS ${DEST_DIR}/${TARGET_OUTPUT})
    ADD_CUSTOM_COMMAND(OUTPUT ${DEST_DIR}/${TARGET_OUTPUT}
      COMMAND ${CP_CMD} ${NATIVE_TARGET_LOCATION} ${NATIVE_NEW_TARGET_LOCATION}/${TARGET_OUTPUT}
      DEPENDS ${TARGET_NAME})
      # Linux requires the symbolic link mechanism for .so files.
      IF(NOT WIN32)
        GET_TARGET_PROPERTY(TARGET_SOVERSION ${TARGET_NAME} SOVERSION)
        GET_TARGET_PROPERTY(TARGET_VERSION ${TARGET_NAME} VERSION)
        # SOVERSION symbolic link.
        IF(NOT ${TARGET_SOVERSION} STREQUAL NOTFOUND)
          ADD_CUSTOM_TARGET(${TARGET_OUTPUT}.${TARGET_SOVERSION}_copy ALL
            DEPENDS ${DEST_DIR}/${TARGET_OUTPUT}.${TARGET_SOVERSION})
          ADD_CUSTOM_COMMAND(OUTPUT ${DEST_DIR}/${TARGET_OUTPUT}.${TARGET_SOVERSION}
            COMMAND ${CP_CMD} ${NATIVE_TARGET_LOCATION}.${TARGET_SOVERSION} ${NATIVE_NEW_TARGET_LOCATION}/${TARGET_OUTPUT}.${TARGET_SOVERSION}
            DEPENDS ${TARGET_NAME})
        ENDIF(NOT ${TARGET_SOVERSION} STREQUAL NOTFOUND)
        # VERSION symbolic link.
        IF(NOT ${TARGET_VERSION} STREQUAL NOTFOUND)
          ADD_CUSTOM_TARGET(${TARGET_OUTPUT}.${TARGET_VERSION}_copy ALL
            DEPENDS ${DEST_DIR}/${TARGET_OUTPUT}.${TARGET_VERSION})
          ADD_CUSTOM_COMMAND(OUTPUT ${DEST_DIR}/${TARGET_OUTPUT}.${TARGET_VERSION}
            COMMAND ${CP_CMD} ${NATIVE_TARGET_LOCATION}.${TARGET_VERSION} ${NATIVE_NEW_TARGET_LOCATION}/${TARGET_OUTPUT}.${TARGET_VERSION}
            DEPENDS ${TARGET_NAME})
        ENDIF(NOT ${TARGET_VERSION} STREQUAL NOTFOUND)
      ENDIF(NOT WIN32)
  ENDIF(${ARGC} GREATER 2)
ENDMACRO(COPY_TARGET)

#! Copy the file to the specified location under ${CMAKE_BINARY_DIR}/output.
#! @param SOURCE the full path of the file.
#! @param DEST_DIR the full path of the destination directory.
#! @paramoptional NEW_NAME copy to the new directory with the new name.
MACRO(OUTPUT_FILE SOURCE DEST_DIR)
  IF(${ARGC} GREATER 2)
    COPY_FILE(${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE} ${CMAKE_BINARY_DIR}/output/${DEST_DIR} ${ARGV2})
  ELSE(${ARGC} GREATER 2)
    COPY_FILE(${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE} ${CMAKE_BINARY_DIR}/output/${DEST_DIR})
  ENDIF(${ARGC} GREATER 2)
ENDMACRO(OUTPUT_FILE SOURCE DEST_DIR)

#! Copy the directory to the specified location under ${CMAKE_BINARY_DIR}/output.
#! @param SOURCE the full path of the directory.
#! @param DEST_DIR the full path of the destination directory.
MACRO(OUTPUT_DIR SOURCE DEST_DIR)
  COPY_DIR(${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE} ${CMAKE_BINARY_DIR}/output/${DEST_DIR} ${ARGV2})
ENDMACRO(OUTPUT_DIR SOURCE DEST_DIR)

#! Copy the target to the specified location under ${CMAKE_BINARY_DIR}/output.
#! @param TARGET_NAME the name of the target.
#! @param DEST_DIR the full path of the destination directory.
#! @paramoptional NEW_NAME copy to the new directory with the new name.
MACRO(OUTPUT_TARGET TARGET_NAME DEST_DIR)
  IF(${ARGC} GREATER 2)
    COPY_TARGET(${TARGET_NAME} ${CMAKE_BINARY_DIR}/output/${DEST_DIR} ${ARGV2})
  ELSE(${ARGC} GREATER 2)
    COPY_TARGET(${TARGET_NAME} ${CMAKE_BINARY_DIR}/output/${DEST_DIR})
  ENDIF(${ARGC} GREATER 2)
ENDMACRO(OUTPUT_TARGET TARGET_NAME DEST_DIR)
