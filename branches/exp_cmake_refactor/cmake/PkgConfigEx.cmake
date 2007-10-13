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

MACRO(REMOVE_TRAILING_NEWLINE _variable)
  STRING(REPLACE "\n" "" ${_variable} ${${_variable}})
ENDMACRO(REMOVE_TRAILING_NEWLINE _variable)

#! Use pkg-config to get information of a library package and update current
#! cmake environment.
#!
#! If succeeds,
#!   - Include directories of that package will be appended to
#!     @c INCLUDE_DIRECTORIES;
#!   - Other cflags will be appended to @c CMAKE_C_FLAGS and @c CMAKE_CXX_FLAGS;
#!   - Link flags (other than -l flags) will be appended to
#!     @c CMAKE_EXTRA_LINK_FLAGS;
#!
#! @param _package the package name.
#! @param _min_version minimum version required. 
#! @param[out] _libraries return the list of library names.
#! @paramoptional[out] _found return if the required library is found.  If this
#!     parameter is not provided, an error message will shown and cmake quits.
MACRO(PKGCONFIG_EX _package _min_version _libraries)
  SET(${_libraries})
  FIND_PROGRAM(PKGCONFIG_EX_executable NAMES pkg-config PATHS /usr/local/bin)
  EXEC_PROGRAM(${PKGCONFIG_EX_executable}
    ARGS ${_package} --atleast-version=${_min_version}
    RETURN_VALUE PKGCONFIG_EX_return_value)
  IF(${PKGCONFIG_EX_return_value} EQUAL 0)
    # Get include directories.
    EXEC_PROGRAM(${PKGCONFIG_EX_executable}
      ARGS ${_package} --cflags-only-I
      OUTPUT_VARIABLE PKGCONFIG_EX_include_dir)
    REMOVE_TRAILING_NEWLINE(PKGCONFIG_EX_include_dir)
    STRING(REGEX REPLACE "^-I" ""
      PKGCONFIG_EX_include_dir ${PKGCONFIG_EX_include_dir})
    STRING(REGEX REPLACE " -I" ";"
      PKGCONFIG_EX_include_dir ${PKGCONFIG_EX_include_dir})
    INCLUDE_DIRECTORIES(${PKGCONFIG_EX_include_dir})

    # Get other cflags other than -I include directories.
    EXEC_PROGRAM(${PKGCONFIG_EX_executable}
      ARGS ${_package} --cflags-only-other
      OUTPUT_VARIABLE PKGCONFIG_EX_cflags)
    REMOVE_TRAILING_NEWLINE(PKGCONFIG_EX_cflags)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${PKGCONFIG_EX_cflags}")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PKGCONFIG_EX_cflags}")

    # Get library names.
    EXEC_PROGRAM(${PKGCONFIG_EX_executable}
      ARGS ${_package} --libs-only-l
      OUTPUT_VARIABLE ${_libraries})
    REMOVE_TRAILING_NEWLINE(${_libraries})
    STRING(REGEX REPLACE "^-l" "" ${_libraries} ${${_libraries}})
    STRING(REGEX REPLACE " -l" ";" ${_libraries} ${${_libraries}})

    # Get -L link flags.
    EXEC_PROGRAM(${PKGCONFIG_EX_executable}
      ARGS ${_package} --libs-only-L
      OUTPUT_VARIABLE PKGCONFIG_EX_Lflags)
    REMOVE_TRAILING_NEWLINE(PKGCONFIG_EX_Lflags)
    STRING(REGEX REPLACE "^-L" ""
      PKGCONFIG_EX_Lflags ${PKGCONFIG_EX_Lflags})
    STRING(REGEX REPLACE " -L" ";"
      PKGCONFIG_EX_Lflags ${PKGCONFIG_EX_Lflags})
    LINK_DIRECTORIES(${PKGCONFIG_EX_Lflags})

    # Get other link flags.
    EXEC_PROGRAM(${PKGCONFIG_EX_executable}
      ARGS ${_package} --libs-only-other
      OUTPUT_VARIABLE PKGCONFIG_EX_libs_other)
    REMOVE_TRAILING_NEWLINE(PKGCONFIG_EX_libs_other)
    SET(CMAKE_EXTRA_LINK_FLAGS
      ${CMAKE_EXTRA_LINK_FLAGS} ${PKGCONFIG_EX_libs_other})

    IF(${ARGC} GREATER 3)
      SET(${ARGV3} TRUE)
    ENDIF(${ARGC} GREATER 3)
  ELSEIF(${ARGC} GREATER 3)
    SET(${ARGV3} FALSE)
  ELSE(${ARGC} GREATER 3)
    MESSAGE(FATAL_ERROR
      "Required package ${_package} version>=${_min_version} not found")
  ENDIF(${PKGCONFIG_EX_return_value} EQUAL 0)
ENDMACRO(PKGCONFIG_EX _package _min_version _libraries)
