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

PKGCONFIG_EX(gtk+-2.0 2.6 GTK_LIBRARIES)
PKGCONFIG_EX(libcurl 7.16.0 LIBCURL_LIBRARIES)

ADD_EXECUTABLE(xml_http_request_test
  xml_http_request_test.cc
  ${CMAKE_SOURCE_DIR}/hosts/simple/gtk_cairo_host.cc
  ${CMAKE_SOURCE_DIR}/hosts/simple/xml_http_request.cc
)
TARGET_LINK_LIBRARIES(xml_http_request_test
  pthread
  gunit
  ggadget
  ${GTK_LIBRARIES}
  ${LIBCURL_LIBRARIES}
)
TEST_WRAPPER(xml_http_request_test TRUE)
TEST_RESOURCE_DIR(xml_http_request_test_data)
