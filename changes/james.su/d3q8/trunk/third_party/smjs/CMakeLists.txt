#
#  Copyright 2007 Google Inc.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#

IF(UNIX)
  SET(SHARED_LIBRARY_PREFIX lib)
  SET(SHARED_LIBRARY_SUFFIX .so)
  SET(BUILD_SPIDER_MONKEY ${CMAKE_SOURCE_DIR}/cmake/build_spidermonkey.sh)
ELSE(UNIX)
  MESSAGE(FATAL_ERROR "Unsupported yet.")
ENDIF(UNIX)

ADD_CUSTOM_TARGET(js ALL
  DEPENDS ${CMAKE_BINARY_DIR}/output/lib/${SHARED_LIBRARY_PREFIX}js${SHARED_LIBRARY_SUFFIX})
ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_BINARY_DIR}/output/lib/${SHARED_LIBRARY_PREFIX}js${SHARED_LIBRARY_SUFFIX}
  COMMAND "${BUILD_SPIDER_MONKEY}" "${CMAKE_BUILD_TYPE}" "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_BINARY_DIR}/output/lib"
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/js/src/jsapi.h)
ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/js/src/jsapi.h
  COMMAND tar zxvf ${CMAKE_CURRENT_BINARY_DIR}/js-1.60.tar.gz
  COMMAND touch ${CMAKE_CURRENT_BINARY_DIR}/js/src/jsapi.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/js-1.60.tar.gz)
ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/js-1.60.tar.gz
  COMMAND wget "http://ftp.mozilla.org/pub/mozilla.org/js/js-1.60.tar.gz"
    -O ${CMAKE_CURRENT_BINARY_DIR}/js-1.60.tar.gz)
