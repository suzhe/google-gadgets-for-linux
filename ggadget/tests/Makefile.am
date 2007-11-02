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

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

LDADD			= $(top_builddir)/unittest/libgunit.la \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la

# TODO: file_manager_test
check_PROGRAMS		= common_test \
			  variant_test \
			  slot_test \
			  signal_test \
			  scriptable_helper_test \
			  elements_test \
			  element_factory_test \
			  file_manager_test \
			  math_utils_test \
			  unicode_utils_test \
			  string_utils_test \
			  basic_element_test \
			  view_test \
			  xml_dom_test \
			  xml_utils_test \
			  xml_http_request_test

common_test_SOURCES		= common_test.cc
variant_test_SOURCES		= variant_test.cc
slot_test_SOURCES		= slot_test.cc slots.cc
signal_test_SOURCES		= signal_test.cc slots.cc
scriptable_helper_test_SOURCES	= scriptable_helper_test.cc scriptables.cc
elements_test_SOURCES		= elements_test.cc
element_factory_test_SOURCES	= element_factory_test.cc
file_manager_test_SOURCES	= file_manager_test.cc
math_utils_test_SOURCES		= math_utils_test.cc
unicode_utils_test_SOURCES	= unicode_utils_test.cc
string_utils_test_SOURCES	= string_utils_test.cc
basic_element_test_SOURCES	= basic_element_test.cc
view_test_SOURCES		= view_test.cc
xml_dom_test_SOURCES		= xml_dom_test.cc
xml_utils_test_SOURCES		= xml_utils_test.cc

xml_http_request_test_SOURCES	= xml_http_request_test.cc
xml_http_request_test_LDADD	= -lpthread \
				  $(top_builddir)/unittest/libgunit.la \
				  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la

if GGL_HAVE_ZIP
noinst_DATA		= file_manager_test_data.gg
file_manager_test_data.gg:
	(\
	  cd $(abs_srcdir)/file_manager_test_data/;\
	  find . -type f \! -name '.*' |\
	  $(ZIP) -r -u $(abs_builddir)/file_manager_test_data.gg -@\
	)
endif

TESTS 			= $(check_PROGRAMS)
