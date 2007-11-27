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
SUBDIRS = . gtk linux smjs tests

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)


gglincludedir		= $(includedir)/ggl@GGL_EPOCH@/ggadget

noinst_HEADERS		= element_factory_impl.h

gglinclude_HEADERS	= anchor_element.h \
			  audioclip_interface.h \
			  basic_element.h \
			  button_element.h \
			  canvas_interface.h \
			  checkbox_element.h \
			  color.h \
			  common.h \
			  combobox_element.h \
			  content_item.h \
			  details_view.h \
			  display_window.h \
			  div_element.h \
			  edit_element.h \
			  edit_interface.h \
			  element_factory.h \
			  element_factory_interface.h \
			  element_interface.h \
			  elements.h \
			  event.h \
			  file_manager.h \
			  file_manager_interface.h \
			  file_system_interface.h \
			  font_interface.h \
			  framework_interface.h \
			  gadget.h \
			  gadget_consts.h \
			  gadget_host_interface.h \
			  gadget_interface.h \
			  ggadget.h \
			  graphics_interface.h \
			  image.h \
			  img_element.h \
			  item_element.h \
			  label_element.h \
			  listbox_element.h \
			  math_utils.h \
			  menu_interface.h \
			  options_interface.h \
			  progressbar_element.h \
			  scoped_ptr.h \
			  script_context_interface.h \
			  script_runtime_interface.h \
			  scriptable_array.h \
			  scriptable_binary_data.h \
			  scriptable_delegator.h \
			  scriptable_event.h \
			  scriptable_file_system.h \
			  scriptable_framework.h \
			  scriptable_helper.h \
			  scriptable_interface.h \
			  scriptable_menu.h \
			  scriptable_options.h \
			  scrollbar_element.h \
			  signals.h \
			  slot.h \
			  string_utils.h \
			  sysdeps.h \
			  text_frame.h \
			  texture.h \
			  unicode_utils.h \
			  variant.h \
			  view.h \
			  view_host_interface.h \
			  view_interface.h \
			  windows_locales.h \
			  xml_dom.h \
			  xml_dom_interface.h \
			  xml_http_request.h \
			  xml_http_request_interface.h \
			  xml_utils.h

lib_LTLIBRARIES		= libggadget@GGL_EPOCH@.la

libggadget@GGL_EPOCH@_la_SOURCES = \
			  anchor_element.cc \
			  basic_element.cc \
			  button_element.cc \
			  checkbox_element.cc \
			  combobox_element.cc \
			  details_view.cc \
			  display_window.cc \
			  div_element.cc \
			  edit_element.cc \
			  element_factory.cc \
			  elements.cc \
			  file_manager.cc \
			  gadget.cc \
			  image.cc \
			  img_element.cc \
			  item_element.cc \
			  label_element.cc \
			  listbox_element.cc \
			  math_utils.cc \
			  progressbar_element.cc \
			  scriptable_array.cc \
			  scriptable_event.cc \
			  scriptable_file_system.cc \
			  scriptable_framework.cc \
			  scriptable_helper.cc \
			  scriptable_menu.cc \
			  scriptable_options.cc \
			  scrollbar_element.cc \
			  signals.cc \
			  slot.cc \
			  string_utils.cc \
			  text_frame.cc \
			  texture.cc \
			  unicode_utils.cc \
			  variant.cc \
			  view.cc \
			  windows_locales.cc \
			  xml_dom.cc \
			  xml_http_request.cc \
			  xml_utils.cc


libggadget@GGL_EPOCH@_la_CPPFLAGS = \
			  @LIBCURL_CPPFLAGS@ \
			  @XML_CPPFLAGS@

libggadget@GGL_EPOCH@_la_LIBADD = \
			  @LIBCURL@ \
			  @XML_LIBS@ \
			  $(top_builddir)/third_party/unzip/libunzip.la

libggadget@GGL_EPOCH@_la_LDFLAGS = \
			  -version-info $(LIBGGADGET_CURRENT):$(LIBGGADGET_REVISION):$(LIBGGADGET_AGE) \
			  -export-dynamic \
			  -rpath $(libdir)


