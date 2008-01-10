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

if GGL_BUILD_LIBGGADGET_DBUS

# Putting . into SBUDIRS list to force building current directory first.
SUBDIRS = . tests

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir) \
			  -DGGL_MODULE_DIR=\"$(GGL_MODULE_DIR)\" \
			  -DGGL_LIBEXEC_DIR=\"$(GGL_LIBEXEC_DIR)\" \
			  -DGGL_RESOURCE_DIR=\"$(GGL_RESOURCE_DIR)\"

noinst_HEADERS		= dbus_utils.h

dbusincludedir		= $(GGL_INCLUDE_DIR)/ggadget/dbus
dbusinclude_HEADERS	= dbus_proxy.h \
			  scriptable_dbus_object.h

lib_LTLIBRARIES		= libggadget-dbus@GGL_EPOCH@.la

libggadget_dbus@GGL_EPOCH@_la_SOURCES = \
			  dbus_utils.cc \
			  scriptable_dbus_object.cc \
			  dbus_proxy.cc

libggadget_dbus@GGL_EPOCH@_la_CPPFLAGS = \
			  @DBUS_CFLAGS@

libggadget_dbus@GGL_EPOCH@_la_LIBADD = \
			  @DBUS_LIBS@
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la

libggadget_dbus@GGL_EPOCH@_la_LDFLAGS = \
	-version-info $(LIBGGADGET_CURRENT):$(LIBGGADGET_REVISION):$(LIBGGADGET_AGE) \
	-export-dynamic \
	-rpath $(libdir)

pkgconfigdir= $(libdir)/pkgconfig
pkgconfig_DATA= libggadget-dbus@GGL_EPOCH@.pc
endif
