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

# Find the native MLIB includes and library
#
# MLIB_LIBRARIES   - List of fully qualified libraries to link against when
# using math.
# MLIB_FOUND       - Do not attempt to use math if "no" or undefined.

FIND_LIBRARY(MLIB_LIBRARY m
  /usr/lib
  /usr/local/lib
)

IF(MLIB_LIBRARY)
  SET(MLIB_LIBRARIES ${MLIB_LIBRARY})
ELSE(MLIB_LIBRARY)
  SET(MLIB_LIBRARIES "")
ENDIF(MLIB_LIBRARY)

INCLUDE(CheckFunctionExists)
CHECK_FUNCTION_EXISTS(sin SIN_EXISTS)
IF(SIN_EXISTS)
  SET(MLIB_FOUND "YES")
ENDIF(SIN_EXISTS)

MARK_AS_ADVANCED(MLIB_LIBRARY)
