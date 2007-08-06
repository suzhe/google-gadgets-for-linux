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

SET(PRODUCT_NAME libggadget)

ADD_DEFINITIONS(-DUNIX
  -DPREFIX=\\\"${CMAKE_INSTALL_PREFIX}\\\"
  -DPRODUCT_NAME=\\\"${PRODUCT_NAME}\\\")

IF(EXISTS ${CMAKE_SOURCE_DIR}/CTestConfig.cmake)
  INCLUDE(CTest)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
ENDIF(EXISTS ${CMAKE_SOURCE_DIR}/CTestConfig.cmake)

IF(UNIX)
  SET(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -fPIC -Wall -Werror -Wno-non-virtual-dtor"
  )
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Wall -Werror")
  SET(PROJECT_RESOURCE_DIR share/${PRODUCT_NAME})
ELSE(UNIX)
  SET(PROJECT_RESOURCE_DIR resource)
ENDIF(UNIX)
SET(CMAKE_SKIP_RPATH ON)
ENABLE_TESTING()

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
INCLUDE(CopyUtil)
INCLUDE(GenerateOutput)
INCLUDE(TestSuite)

IF("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  ADD_DEFINITIONS(-D_DEBUG)
ELSE("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  ADD_DEFINITIONS(-DNDEBUG)
ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

ADD_SUBDIRECTORY(unittest)
ADD_SUBDIRECTORY(ggadget)
