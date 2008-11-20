#
# Copyright 2008 Google Inc.
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

EXTRA_DIST = CMakeLists.txt

SUBDIRS = . blank_gadget gadget

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

designer_LTLIBRARIES	= designer-script-utils.la

designerdir		= $(GGL_RESOURCE_DIR)/designer

designer_script_utils_la_SOURCES = designer_script_utils.cc

designer_script_utils_la_CXXFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS)

designer_script_utils_la_CPPFLAGS = \
			  $(PREDEFINED_MACROS)

designer_script_utils_la_LIBADD = \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la

designer_script_utils_la_LDFLAGS = \
			  -avoid-version \
			  -module
