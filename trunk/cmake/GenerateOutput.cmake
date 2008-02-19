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

IF(WIN32)
  MACRO(OUTPUT_EXECUTABLE _target_name)
    OUTPUT_TARGET(${_target_name} /)
  ENDMACRO(OUTPUT_EXECUTABLE _target_name)
  MACRO(OUTPUT_LIBRARY _target_name)
    OUTPUT_TARGET(${_target_name} /)
  ENDMACRO(OUTPUT_LIBRARY _target_name)
ELSE(WIN32)
  MACRO(OUTPUT_EXECUTABLE _target_name)
    OUTPUT_TARGET(${_target_name} bin ${_target_name}.bin)
    COPY_FILE(${CMAKE_SOURCE_DIR}/cmake/bin_wrapper.sh
      ${CMAKE_BINARY_DIR}/output/bin ${_target_name})
  ENDMACRO(OUTPUT_EXECUTABLE _target_name)
  MACRO(OUTPUT_LIBRARY _target_name)
    OUTPUT_TARGET(${_target_name} lib)
  ENDMACRO(OUTPUT_LIBRARY _target_name)
ENDIF(WIN32)

MACRO(OUTPUT_MODULE _target_name)
  SET_TARGET_PROPERTIES(${_target_name} PROPERTIES PREFIX "")
  OUTPUT_TARGET(${_target_name} modules)
ENDMACRO(OUTPUT_MODULE _target_name)
