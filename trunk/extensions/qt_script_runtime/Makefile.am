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

if GGL_BUILD_QT_SCRIPT_RUNTIME
SUBDIRS = . tests

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

noinst_HEADERS		= converter.h \
			  js_function_slot.h \
			  js_script_runtime.h \
			  js_script_context.h \
			  json.h

extension_LTLIBRARIES	= qt-script-runtime.la
extensiondir		= $(GGL_MODULE_DIR)

qt_script_runtime_la_SOURCES = \
			  qt_script_runtime.cc \
			  converter.cc \
			  js_function_slot.cc \
			  js_script_context.cc \
			  js_script_runtime.cc \
			  json.cc

qt_script_runtime_la_CXXFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS) \
			  -Wno-conversion

qt_script_runtime_la_CPPFLAGS = \
			  @QT4_CFLAGS@ \
			  $(PREDEFINED_MACROS)

qt_script_runtime_la_LIBADD = \
			  @QT4_LIBS@ \
			  @QT4SCRIPT_LIBS@ \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
			  $(top_builddir)/ggadget/qt/libggadget-qt@GGL_EPOCH@.la \
			  $(top_builddir)/ggadget/js/libggadget-js@GGL_EPOCH@.la

qt_script_runtime_la_LDFLAGS = \
			  -avoid-version \
			  -module
endif
