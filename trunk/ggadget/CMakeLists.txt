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

ADD_SUBDIRECTORY(dbus)
ADD_SUBDIRECTORY(gtk)
ADD_SUBDIRECTORY(tests)

SET(SRCS
  basic_element.cc
  canvas_utils.cc
  color.cc
  content_item.cc
  details_view_data.cc
  digest_utils.cc
  display_window.cc
  directory_provider.cc
  element_factory.cc
  elements.cc
  encryptor.cc
  extension_manager.cc
  gadget.cc
  gadget_manager.cc
  locales.cc
  logger.cc
  main_loop.cc
  math_utils.cc
  memory_options.cc
  module.cc
  native_main_loop.cc
  options_factory.cc
  script_runtime_manager.cc
  scriptable_array.cc
  scriptable_event.cc
  scriptable_file_system.cc
  scriptable_framework.cc
  scriptable_helper.cc
  scriptable_image.cc
  scriptable_menu.cc
  scriptable_options.cc
  scriptable_view.cc
  signals.cc
  slot.cc
  string_utils.cc
  system_utils.cc
  texture.cc
  text_frame.cc
  unicode_utils.cc
  variant.cc
  view.cc
  xml_dom.cc
  xml_http_request_factory.cc
  xml_parser.cc
  xml_utils.cc

  anchor_element.cc
  button_element.cc
  checkbox_element.cc
  combobox_element.cc
  contentarea_element.cc
  div_element.cc
  edit_element_base.cc
  img_element.cc
  item_element.cc
  label_element.cc
  listbox_element.cc
  progressbar_element.cc
  scrollbar_element.cc
  scrolling_element.cc

  dir_file_manager.cc
  file_manager_factory.cc
  file_manager_wrapper.cc
  localized_file_manager.cc
  zip_file_manager.cc
)

SET(LIBS
  ltdl
  unzip
)

ADD_LIBRARY(ggadget SHARED ${SRCS})
TARGET_LINK_LIBRARIES(ggadget ${LIBS})
OUTPUT_LIBRARY(ggadget)
