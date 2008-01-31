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

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

LIBADD			= $(top_builddir)/unittest/libgunit.la \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la

AM_CXXFLAGS		= $(DEFAULT_COMPILE_FLAGS)

AM_LDFLAGS		= -avoid-version \
			  -rpath $(libdir) \
			  -module

check_LTLIBRARIES	= fake-module.la \
			  tux-module.la

fake_module_la_SOURCES	= ../test_module.cc
fake_module_la_CPPFLAGS	= $(PREDEFINED_MACROS) \
			  -DMODULE_NAME=fake-module \
			  -DMODULE_NAME_UNDERSCORE=fake_module

tux_module_la_SOURCES	= ../test_module.cc
tux_module_la_CPPFLAGS	= $(PREDEFINED_MACROS) \
			  -DMODULE_NAME=tux-module \
			  -DMODULE_NAME_UNDERSCORE=tux_module \
			  -DFRAMEWORK_EXTENSION=1 \
			  -DELEMENT_EXTENSION=1
