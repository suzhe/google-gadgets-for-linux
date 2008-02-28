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

SUBDIRS = . gadget

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

gadget_browser_LTLIBRARIES = gadget-browser-script-utils.la
gadget_browserdir	= $(GGL_RESOURCE_DIR)/gadget-browser

gadget_browser_script_utils_la_SOURCES = gadget_browser_script_utils.cc

gadget_browser_script_utils_la_CXXFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS)

gadget_browser_script_utils_la_CPPFLAGS = \
			  $(PREDEFINED_MACROS)

gadget_browser_script_utils_la_LIBADD = \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la

gadget_browser_script_utils_la_LDFLAGS = \
			  -avoid-version \
			  -module
