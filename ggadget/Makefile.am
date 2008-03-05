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

# Putting . into SBUDIRS list to force building current directory first.
SUBDIRS = . tests
if GGL_BUILD_LIBGGADGET_GTK
SUBDIRS += gtk
endif

if GGL_BUILD_LIBGGADGET_QT
SUBDIRS += qt
endif

if GGL_BUILD_LIBGGADGET_DBUS
SUBDIRS += dbus
endif

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

gglincludedir		= $(GGL_INCLUDE_DIR)/ggadget
gglsysdepsincludedir	= $(GGL_SYSDEPS_INCLUDE_DIR)/ggadget

gglinclude_HEADERS	= anchor_element.h \
			  audioclip_interface.h \
			  basic_element.h \
			  button_element.h \
			  canvas_interface.h \
			  canvas_utils.h \
			  checkbox_element.h \
			  color.h \
			  common.h \
			  combobox_element.h \
			  content_item.h \
			  contentarea_element.h \
			  details_view.h \
			  dir_file_manager.h \
			  directory_provider_interface.h \
			  display_window.h \
			  div_element.h \
			  edit_element_base.h \
			  element_factory.h \
			  elements.h \
			  encryptor_interface.h \
			  event.h \
			  extension_manager.h \
			  file_manager_factory.h \
			  file_manager_interface.h \
			  file_manager_wrapper.h \
			  file_system_interface.h \
			  font_interface.h \
			  framework_interface.h \
			  gadget.h \
			  gadget_consts.h \
			  gadget_host_interface.h \
			  gadget_interface.h \
			  gadget_manager.h \
			  gadgets_metadata.h \
			  ggadget.h \
			  graphics_interface.h \
			  image_interface.h \
			  img_element.h \
			  item_element.h \
			  label_element.h \
			  listbox_element.h \
			  localized_file_manager.h \
			  main_loop_interface.h \
			  native_main_loop.h \
			  logger.h \
			  math_utils.h \
			  memory_options.h \
			  menu_interface.h \
			  module.h \
			  options_interface.h \
			  progressbar_element.h \
			  registerable_interface.h \
			  scoped_ptr.h \
			  script_context_interface.h \
			  script_runtime_interface.h \
			  script_runtime_manager.h \
			  scriptable_array.h \
			  scriptable_binary_data.h \
			  scriptable_event.h \
			  scriptable_file_system.h \
			  scriptable_framework.h \
			  scriptable_helper.h \
			  scriptable_holder.h \
			  scriptable_image.h \
			  scriptable_interface.h \
			  scriptable_map.h \
			  scriptable_menu.h \
			  scriptable_options.h \
			  scrollbar_element.h \
			  scrolling_element.h \
			  signals.h \
			  slot.h \
			  string_utils.h \
			  system_utils.h \
			  text_frame.h \
			  texture.h \
			  unicode_utils.h \
			  variant.h \
			  view.h \
			  view_host_interface.h \
			  view_interface.h \
			  windows_locales.h \
			  xml_dom_interface.h \
			  xml_dom.h \
			  xml_http_request_interface.h \
			  xml_parser_interface.h \
			  xml_utils.h \
			  zip_file_manager.h

nodist_gglsysdepsinclude_HEADERS= sysdeps.h
BUILT_SOURCES=$(gglsysdepsinclude_HEADERS)

lib_LTLIBRARIES		= libggadget@GGL_EPOCH@.la

libggadget@GGL_EPOCH@_la_SOURCES = \
			  anchor_element.cc \
			  basic_element.cc \
			  button_element.cc \
			  canvas_utils.cc \
			  checkbox_element.cc \
			  color.cc \
			  combobox_element.cc \
			  content_item.cc \
			  contentarea_element.cc \
			  details_view.cc \
			  directory_provider.cc \
			  dir_file_manager.cc \
			  display_window.cc \
			  div_element.cc \
			  edit_element_base.cc \
			  element_factory.cc \
			  elements.cc \
			  encryptor.cc \
			  extension_manager.cc \
			  file_manager_factory.cc \
			  file_manager_wrapper.cc \
			  gadget.cc \
			  gadget_manager.cc \
			  gadgets_metadata.cc \
			  img_element.cc \
			  item_element.cc \
			  label_element.cc \
			  listbox_element.cc \
			  localized_file_manager.cc \
			  logger.cc \
			  main_loop.cc \
			  math_utils.cc \
			  memory_options.cc \
			  module.cc \
			  native_main_loop.cc \
			  options_factory.cc \
			  progressbar_element.cc \
			  script_runtime_manager.cc \
			  scriptable_array.cc \
			  scriptable_event.cc \
			  scriptable_file_system.cc \
			  scriptable_framework.cc \
			  scriptable_helper.cc \
			  scriptable_image.cc \
			  scriptable_menu.cc \
			  scriptable_options.cc \
			  scrollbar_element.cc \
			  scrolling_element.cc \
			  signals.cc \
			  slot.cc \
			  string_utils.cc \
			  system_utils.cc \
			  text_frame.cc \
			  texture.cc \
			  unicode_utils.cc \
			  variant.cc \
			  view.cc \
			  windows_locales.cc \
			  xml_dom.cc \
			  xml_http_request_factory.cc \
			  xml_parser.cc \
			  xml_utils.cc \
			  zip_file_manager.cc


libggadget@GGL_EPOCH@_la_CPPFLAGS = \
			  @LTDLINCL@ \
			  $(PREDEFINED_MACROS)

libggadget@GGL_EPOCH@_la_CXXFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS)

libggadget@GGL_EPOCH@_la_LIBADD = \
			  $(top_builddir)/third_party/unzip/libunzip.la

libggadget@GGL_EPOCH@_la_LDFLAGS = \
			  -version-info $(LIBGGADGET_CURRENT):$(LIBGGADGET_REVISION):$(LIBGGADGET_AGE) \
			  -export-dynamic \
			  @LIBLTDL@

pkgconfigdir= $(libdir)/pkgconfig
pkgconfig_DATA= libggadget@GGL_EPOCH@.pc
