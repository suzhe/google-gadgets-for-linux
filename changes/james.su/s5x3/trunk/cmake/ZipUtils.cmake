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
#! @param _source the full path of the directory.
#! @param _zip_file the full path of the destination zip file.
MACRO(MAKE_ZIP _source _zip_file)
  GET_FILENAME_COMPONENT(MAKE_ZIP_name ${_zip_file} NAME)
  ADD_CUSTOM_TARGET(${MAKE_ZIP_name}_zip ALL
    DEPENDS ${_zip_file})
  FILE(GLOB_RECURSE MAKE_ZIP_files ${_source}/*)
  SET(MAKE_ZIP_relative_files)
  SET(MAKE_ZIP_source_files)
  FOREACH(MAKE_ZIP_filename ${MAKE_ZIP_files})
    IF(NOT ${MAKE_ZIP_filename} MATCHES /\\.svn/)
      FILE(RELATIVE_PATH MAKE_ZIP_relative_name ${_source} ${MAKE_ZIP_filename})
      SET(MAKE_ZIP_source_files ${MAKE_ZIP_source_files} ${MAKE_ZIP_filename})
      SET(MAKE_ZIP_relative_files
        ${MAKE_ZIP_relative_files} ${MAKE_ZIP_relative_name})
    ENDIF(NOT ${MAKE_ZIP_filename} MATCHES /\\.svn/)
  ENDFOREACH(MAKE_ZIP_filename ${MAKE_ZIP_files})
  ADD_CUSTOM_COMMAND(OUTPUT ${_zip_file}
    COMMAND zip -r -u ${_zip_file} ${MAKE_ZIP_relative_files}
    DEPENDS ${MAKE_ZIP_source_files}
    WORKING_DIRECTORY ${_source})
ENDMACRO(MAKE_ZIP _source _zip_file)

MACRO(OUTPUT_GG _dir_name)
  MAKE_ZIP(${CMAKE_CURRENT_SOURCE_DIR}/${_dir_name}
    ${CMAKE_BINARY_DIR}/output/${_dir_name}.gg)
ENDMACRO(OUTPUT_GG _dir_name)

MACRO(TEST_RESOURCE_GG _dir_name)
  MAKE_ZIP(${CMAKE_CURRENT_SOURCE_DIR}/${_dir_name}
    ${CMAKE_CURRENT_BINARY_DIR}/${_dir_name}.gg)
ENDMACRO(TEST_RESOURCE_GG _dir_name)
