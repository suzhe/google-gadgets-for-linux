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

ADD_DEFINITIONS(-DUNIT_TEST)

MACRO(UNIT_TEST TEST_NAME)
  IF(${ARGC} GREATER 1)
    SET(FILES ${ARGV})
    LIST(REMOVE_AT FILES 0)
    ADD_EXECUTABLE(${TEST_NAME} ${TEST_NAME}.cc ${FILES})
  ELSE(${ARGC} GREATER 1)
    ADD_EXECUTABLE(${TEST_NAME} ${TEST_NAME}.cc)
  ENDIF(${ARGC} GREATER 1)
  TARGET_LINK_LIBRARIES(${TEST_NAME} gunit ggadget)
  TEST_WRAPPER(${TEST_NAME} TRUE)
ENDMACRO(UNIT_TEST TEST_NAME)

UNIT_TEST(slot_test slots.cc)
UNIT_TEST(signal_test slots.cc)
UNIT_TEST(static_scriptable_test scriptables.cc)
UNIT_TEST(elements_test)
UNIT_TEST(element_factory_test)
UNIT_TEST(utility_test)
UNIT_TEST(unicode_utils_test)
