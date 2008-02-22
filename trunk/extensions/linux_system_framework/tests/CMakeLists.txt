#
#  Copyright 2008 Google Inc.
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

SET(LIBS
  gunit
  ggadget
  ggadget_dbus
  ggadget_linux_for_test
)

ADD_EXECUTABLE(machine_test machine_test.cc)
TARGET_LINK_LIBRARIES(machine_test ${LIBS})
TEST_WRAPPER(machine_test TRUE)

ADD_EXECUTABLE(network_test network_test.cc)
TARGET_LINK_LIBRARIES(network_test ${LIBS})
TEST_WRAPPER(network_test TRUE)

ADD_EXECUTABLE(power_test power_test.cc)
TARGET_LINK_LIBRARIES(power_test ${LIBS})
TEST_WRAPPER(power_test TRUE)

ADD_EXECUTABLE(wireless_test wireless_test.cc)
TARGET_LINK_LIBRARIES(wireless_test ${LIBS})
TEST_WRAPPER(wireless_test TRUE)
