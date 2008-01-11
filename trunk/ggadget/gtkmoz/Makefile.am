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

if GGL_BUILD_LIBGGADGET_GTKMOZ

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

noinst_HEADERS          = browser_child.h

gtkmozincludedir	= $(GGL_INCLUDE_DIR)/ggadget/gtkmoz
gtkmozinclude_HEADERS	= browser_element.h

lib_LTLIBRARIES		= libggadget-gtkmoz@GGL_EPOCH@.la

libggadget_gtkmoz@GGL_EPOCH@_la_SOURCES = \
			  browser_element.cc

libggadget_gtkmoz@GGL_EPOCH@_la_CXXFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS)

libggadget_gtkmoz@GGL_EPOCH@_la_CPPFLAGS = \
			  @GTK2_CFLAGS@ \
			  $(PREDEFINED_MACROS)

libggadget_gtkmoz@GGL_EPOCH@_la_LIBADD = \
			  @GTK2_LIBS@

libggadget_gtkmoz@GGL_EPOCH@_la_LDFLAGS = \
			  -version-info $(LIBGGADGET_CURRENT):$(LIBGGADGET_REVISION):$(LIBGGADGET_AGE) \
			  -export-dynamic \
			  -rpath $(libdir)


ggllibexec_PROGRAMS	= gtkmoz_browser_child
ggllibexecdir		= $(GGL_LIBEXEC_DIR)

gtkmoz_browser_child_SOURCES= browser_child.cc ../smjs/json.cc

gtkmoz_browser_child_CXXFLAGS	= $(DEFAULT_COMPILE_FLAGS) \
				  -Wno-non-virtual-dtor

gtkmoz_browser_child_CPPFLAGS	= @GTK2_CFLAGS@ \
				  @GTKMOZEMBED_CFLAGS@ \
				  $(PREDEFINED_MACROS)

gtkmoz_browser_child_LDADD	= @GTK2_LIBS@ \
				  @GTKMOZEMBED_LIBS@

gtkmoz_browser_child_LDFLAGS	= @SPIDERMONKEY_LDFLAGS@

endif
