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

ADD_SUBDIRECTORY(graphics)
ADD_SUBDIRECTORY(scripts)
# ADD_SUBDIRECTORY(sysdeps)
ADD_SUBDIRECTORY(tests)

PKGCONFIG_EX(libcurl 7.16.0 LIBCURL_LIBRARIES)

SET(SRCS
  basic_element.cc
  element_factory.cc
  elements.cc
  file_manager.cc
  gadget.cc
  image.cc
  math_utils.cc
  scriptable_event.cc
  scriptable_helper.cc
  scriptable_options.cc
  signal.cc
  string_utils.cc
  texture.cc
  unicode_utils.cc
  variant.cc
  view.cc
  windows_locales.cc
  xml_http_request.cc
  xml_utils.cc

  button_element.cc
  div_element.cc
  img_element.cc
  scrollbar_element.cc
)

SET(LIBS
  tinyxml
  unzip
  ${LIBCURL_LIBRARIES}
)

ADD_LIBRARY(ggadget SHARED ${SRCS})
TARGET_LINK_LIBRARIES(ggadget ${LIBS})
OUTPUT_LIBRARY(ggadget)
