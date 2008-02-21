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

if GGL_BUILD_LIBGGADGET_GTK

# Putting . into SBUDIRS list to force building current directory first.
SUBDIRS = . tests


INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

if GGL_HAVE_RSVG
  RSVG_IMAGE_H=rsvg_image.h
  RSVG_IMAGE_CC=rsvg_image.cc
endif

noinst_HEADERS		= cairo_canvas.h \
			  cairo_font.h \
			  gtk_key_convert.h \
			  gtk_menu_impl.h \
			  pixbuf_image.h \
			  $(RSVG_IMAGE_H)

gtkincludedir		= $(GGL_INCLUDE_DIR)/ggadget/gtk
gtkinclude_HEADERS	= cairo_graphics.h \
			  gadget_view_widget.h \
			  gtk_main_loop.h \
			  gtk_gadget_host.h \
			  gtk_view_host.h \
			  pixbuf_utils.h

lib_LTLIBRARIES		= libggadget-gtk@GGL_EPOCH@.la

libggadget_gtk@GGL_EPOCH@_la_SOURCES = \
			  cairo_canvas.cc \
			  cairo_font.cc \
			  cairo_graphics.cc \
			  gadget_view_widget.cc \
			  gtk_main_loop.cc \
			  gtk_gadget_host.cc \
			  gtk_key_convert.cc \
			  gtk_menu_impl.cc \
			  gtk_view_host.cc \
			  pixbuf_image.cc \
			  pixbuf_utils.cc \
			  $(RSVG_IMAGE_CC)

libggadget_gtk@GGL_EPOCH@_la_CXXFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS)

libggadget_gtk@GGL_EPOCH@_la_CPPFLAGS = \
			  @CAIRO_CFLAGS@ \
			  @PANGO_CFLAGS@ \
			  @GTK2_CFLAGS@ \
			  @RSVG_CFLAGS@ \
			  $(PREDEFINED_MACROS)

libggadget_gtk@GGL_EPOCH@_la_LIBADD = \
			  @CAIRO_LIBS@ \
			  @PANGO_LIBS@ \
			  @GTK2_LIBS@ \
			  @RSVG_LIBS@ \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la

libggadget_gtk@GGL_EPOCH@_la_LDFLAGS = \
			  -version-info $(LIBGGADGET_CURRENT):$(LIBGGADGET_REVISION):$(LIBGGADGET_AGE) \
			  -export-dynamic

pkgconfigdir= $(libdir)/pkgconfig
pkgconfig_DATA= libggadget-gtk@GGL_EPOCH@.pc
endif
