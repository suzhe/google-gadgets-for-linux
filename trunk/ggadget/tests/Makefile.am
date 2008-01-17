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

EXTRA_DIST		= file_manager_test_data

SUBDIRS = test_modules

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

LDADD			= $(top_builddir)/unittest/libgunit.la \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la

AM_CPPFLAGS		= $(PREDEFINED_MACROS)
AM_CXXFLAGS		= $(DEFAULT_COMPILE_FLAGS)

noinst_HEADERS		= main_loop_test.h \
			  mocked_element.h \
			  mocked_gadget_host.h \
			  mocked_view_host.h \
			  scriptables.h \
			  slots.h

check_PROGRAMS		= color_test \
			  common_test \
			  extension_manager_test \
			  variant_test \
			  slot_test \
			  signal_test \
			  scriptable_helper_test \
			  elements_test \
			  element_factory_test \
			  file_manager_test \
			  math_utils_test \
			  native_main_loop_test \
			  unicode_utils_test \
			  string_utils_test \
			  basic_element_test \
			  module_test \
			  system_utils_test \
			  view_test \
			  xml_dom_test \
			  xml_parser_test \
			  xml_http_request_test

check_LTLIBRARIES	= foo-module.la \
			  bar-module.la

color_test_SOURCES		= color_test.cc
common_test_SOURCES		= common_test.cc
extension_manager_test_SOURCES	= extension_manager_test.cc
variant_test_SOURCES		= variant_test.cc
slot_test_SOURCES		= slot_test.cc slots.cc
signal_test_SOURCES		= signal_test.cc slots.cc
scriptable_helper_test_SOURCES	= scriptable_helper_test.cc scriptables.cc
elements_test_SOURCES		= elements_test.cc
element_factory_test_SOURCES	= element_factory_test.cc
file_manager_test_SOURCES	= file_manager_test.cc
math_utils_test_SOURCES		= math_utils_test.cc
native_main_loop_test_SOURCES	= native_main_loop_test.cc
unicode_utils_test_SOURCES	= unicode_utils_test.cc
string_utils_test_SOURCES	= string_utils_test.cc
basic_element_test_SOURCES	= basic_element_test.cc
module_test_SOURCES		= module_test.cc
system_utils_test_SOURCES	= system_utils_test.cc
view_test_SOURCES		= view_test.cc
xml_dom_test_SOURCES		= xml_dom_test.cc
xml_parser_test_SOURCES		= xml_parser_test.cc

xml_http_request_test_SOURCES	= xml_http_request_test.cc
xml_http_request_test_LDADD	= -lpthread \
				  $(top_builddir)/unittest/libgunit.la \
				  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la

foo_module_la_SOURCES		= test_module.cc
foo_module_la_CPPFLAGS		= $(PREDEFINED_MACROS) \
				  -DMODULE_NAME=foo-module \
				  -DMODULE_NAME_UNDERSCORE=foo_module
foo_module_la_LIBADD		= $(top_builddir)/unittest/libgunit.la \
				  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la
foo_module_la_LDFLAGS		= -avoid-version \
				  -rpath $(libdir) \
				  -module

bar_module_la_SOURCES		= test_module.cc
bar_module_la_CPPFLAGS		= $(PREDEFINED_MACROS) \
				  -DMODULE_NAME=bar-module \
				  -DMODULE_NAME_UNDERSCORE=bar_module
bar_module_la_LIBADD		= $(top_builddir)/unittest/libgunit.la \
				  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la
bar_module_la_LDFLAGS		= -avoid-version \
				  -rpath $(libdir) \
				  -module

file_manager_test_data.dest: file_manager_test_data
	(rm -fr $@; cp -R $< $@; find $@ -name '.*' -exec rm -fr {} \;) || true

if GGL_HAVE_ZIP
noinst_DATA = file_manager_test_data.gg
file_manager_test_data.gg: file_manager_test_data.dest
	(cd $<; find . -type f | $(ZIP) -r -u $(abs_builddir)/$@ -@)
endif

TESTS_ENVIRONMENT	= $(LIBTOOL) --mode=execute $(MEMCHECK_COMMAND)
TESTS 			= $(check_PROGRAMS)
