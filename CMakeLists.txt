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

PROJECT(google_gadgets_for_linux)

IF("${CMAKE_BUILD_TYPE}" STREQUAL "Debug"
    AND EXISTS ${CMAKE_SOURCE_DIR}/CTestConfig.cmake)
  INCLUDE(Dart)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "Debug"
    AND EXISTS ${CMAKE_SOURCE_DIR}/CTestConfig.cmake)

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
INCLUDE(Configure)
INCLUDE(CopyUtils)
INCLUDE(GenerateOutput)
INCLUDE(PkgConfigEx)
INCLUDE(TestSuite)
INCLUDE(ZipUtils)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

ADD_SUBDIRECTORY(unittest)
ADD_SUBDIRECTORY(ggadget)
ADD_SUBDIRECTORY(third_party)
