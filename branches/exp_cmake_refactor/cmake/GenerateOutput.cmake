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
  MACRO(DECORATE_EXECUTABLE _target_prefix _output)
      SET(${_output} ${_target_prefix})
  ENDMACRO(DECORATE_EXECUTABLE _target_prefix _output)
  MACRO(OUTPUT_EXECUTABLE _target_name)
    OUTPUT_TARGET(${_target_name} /)
  ENDMACRO(OUTPUT_EXECUTABLE _target_name)
  MACRO(OUTPUT_LIBRARY _target_name)
    OUTPUT_TARGET(${_target_name} /)
  ENDMACRO(OUTPUT_LIBRARY _target_name)
ELSE(WIN32)
  MACRO(DECORATE_EXECUTABLE _target_prefix _output)
      SET(${_output} ${_target_prefix}.bin)
  ENDMACRO(DECORATE_EXECUTABLE _target_prefix _output)
  MACRO(OUTPUT_EXECUTABLE _target_prefix)
    INSTALL(TARGETS ${_target_prefix}.bin DESTINATION bin)
    INSTALL(PROGRAMS ${CMAKE_SOURCE_DIR}/cmake/bin_wrapper.sh
        DESTINATION bin RENAME ${_target_prefix})
  ENDMACRO(OUTPUT_EXECUTABLE _target_prefix)
  MACRO(OUTPUT_LIBRARY _target_name)
    INSTALL(TARGETS ${_target_name} DESTINATION lib)
  ENDMACRO(OUTPUT_LIBRARY _target_name)
ENDIF(WIN32)
