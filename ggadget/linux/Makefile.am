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

# Putting . into SBUDIRS list to force building current directory first.
SUBDIRS = . tests

if GGL_HOST_LINUX

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

noinst_HEADERS		= file_system.h \
			  machine.h \
			  memory.h \
			  network.h \
			  perfmon.h \
			  power.h \
			  process.h \
			  wireless.h \
			  wireless_access_point.h

linuxincludedir		= $(includedir)/ggl@GGL_EPOCH@/ggadget/linux
linuxinclude_HEADERS	= framework.h

lib_LTLIBRARIES		= libggadget-linux@GGL_EPOCH@.la

libggadget_linux@GGL_EPOCH@_la_SOURCES = \
			  file_system.cc \
			  framework.cc \
			  machine.cc \
			  memory.cc \
			  network.cc \
			  perfmon.cc \
			  power.cc \
			  process.cc \
			  wireless.cc \
			  wireless_access_point.cc

libggadget_linux@GGL_EPOCH@_la_CPPFLAGS =

libggadget_linux@GGL_EPOCH@_la_LIBADD = \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
			  $(top_builddir)/ggadget/dbus/libggadget-dbus@GGL_EPOCH@.la

libggadget_linux@GGL_EPOCH@_la_LDFLAGS = \
			  -version-info $(LIBGGADGET_CURRENT):$(LIBGGADGET_REVISION):$(LIBGGADGET_AGE) \
			  -export-dynamic \
			  -rpath $(libdir)

endif
