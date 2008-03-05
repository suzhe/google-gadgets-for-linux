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

if GGL_BUILD_LIBGGADGET_QT

# Putting . into SBUDIRS list to force building current directory first.
#SUBDIRS = . tests


INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

lib_LTLIBRARIES		= libggadget-qt@GGL_EPOCH@.la

libggadget_qt@GGL_EPOCH@_la_SOURCES = \
				qt_menu.cc \
			  qt_gadget_widget.cc \
			  qt_gadget_host.cc \
			  qt_view_host.cc \
			  qt_canvas.cc \
			  qt_image.cc \
			  qt_font.cc \
			  qt_graphics.cc \
			  qt_main_loop.cc

libggadget_qt@GGL_EPOCH@_la_CXXFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS)

libggadget_qt@GGL_EPOCH@_la_CPPFLAGS = \
			  @QT4_CFLAGS@ \
			  $(PREDEFINED_MACROS)

libggadget_qt@GGL_EPOCH@_la_LIBADD = \
			  @QT4_LIBS@ \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la

libggadget_qt@GGL_EPOCH@_la_LDFLAGS = \
			  -version-info $(LIBGGADGET_CURRENT):$(LIBGGADGET_REVISION):$(LIBGGADGET_AGE) \
			  -export-dynamic \
			  -rpath $(libdir)

pkgconfigdir= $(libdir)/pkgconfig
pkgconfig_DATA= libggadget-qt@GGL_EPOCH@.pc

qt_main_loop.moc:qt_main_loop.cc
	moc $< -o $@
qt_menu.moc:qt_menu.cc
	moc $< -o $@

endif
