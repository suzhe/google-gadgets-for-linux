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

#! Use xml2-config to get information of libxml2.
#!
#! If succeeds,
#!   - Include directories of that package will be appended to
#!     @c INCLUDE_DIRECTORIES;
#!   - Other cflags will be appended to @c CMAKE_C_FLAGS and @c CMAKE_CXX_FLAGS;
#!   - Link flags (other than -l flags) will be appended to
#!     @c CMAKE_EXTRA_LINK_FLAGS;
#!
#! @param _min_version minimum version required. 
#! @param[out] _libraries return the list of library names.
#! @paramoptional[out] _found return if the required library is found.  If this
#!     parameter is not provided, an error message will shown and cmake quits.
MACRO(LIBXML2_CONFIG _min_version _libraries)
  SET(${_libraries})
  FIND_PROGRAM(LIBXML2_CONFIG_executable NAMES xml2-config PATHS /usr/local/bin)
# TODO: real things.
  INCLUDE_DIRECTORIES(/usr/include/libxml2)
  SET(${_libraries} xml2;z;m)
# LIKE_DIRECTORIES(/usr/lib)
  IF(${ARGC} GREATER 3)
    SET(${ARGV3} FALSE)
  ENDIF(${ARGC} GREATER 3)
ENDMACRO(LIBXML2_CONFIG _min_version _libraries)
