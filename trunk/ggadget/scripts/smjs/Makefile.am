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

if GGL_BUILD_LIBGGADGET_SMJS
SUBDIRS = . tests

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

smjsincludedir		= $(includedir)/ggl@GGL_EPOCH@/ggadget/scripts/smjs

smjsinclude_HEADERS	= converter.h \
			  js_function_slot.h \
			  js_script_context.h \
			  js_script_runtime.h \
			  json.h \
			  native_js_wrapper.h

lib_LTLIBRARIES		= libggadget-smjs@GGL_EPOCH@.la

libggadget_smjs@GGL_EPOCH@_la_SOURCES = \
			  converter.cc \
			  js_function_slot.cc \
			  js_script_context.cc \
			  js_script_runtime.cc \
			  json.cc \
			  native_js_wrapper.cc

libggadget_smjs@GGL_EPOCH@_la_CPPFLAGS = \
			  @SPIDERMONKEY_CPPFLAGS@

libggadget_smjs@GGL_EPOCH@_la_LIBADD = \
			  @SPIDERMONKEY_LIBS@ \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la

libggadget_smjs@GGL_EPOCH@_la_LDFLAGS = \
			  -version-info $(LIBGGADGET_CURRENT):$(LIBGGADGET_REVISION):$(LIBGGADGET_AGE) \
			  -export-dynamic \
			  -rpath $(libdir) \
			  @SPIDERMONKEY_LDFLAGS@
endif
