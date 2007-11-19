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

EXTRA_DIST		= resources
if GGL_BUILD_SIMPLE_HOST

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

noinst_HEADERS		=  gadget_view_widget.h \
			   gtk_gadget_host.h \
			   gtk_key_convert.h \
			   gtk_menu_impl.h \
			   gtk_view_host.h \
			   options.h \
			   simplehost_file_manager.h

bin_PROGRAMS		= ggl-simple

ggl_simple_SOURCES	= gadget_view_widget.cc \
			  gtk_gadget_host.cc \
			  gtk_key_convert.cc \
			  gtk_menu_impl.cc \
			  gtk_view_host.cc \
			  main.cc \
			  options.cc \
			  simplehost_file_manager.cc


ggl_simple_CPPFLAGS 	= @CAIRO_CFLAGS@ \
			  @GTK2_CFLAGS@ \
			  @SPIDERMONKEY_CPPFLAGS@

ggl_simple_LDADD	= @CAIRO_LIBS@ \
			  @GTK2_LIBS@ \
			  @SPIDERMONKEY_LIBS@ \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
			  $(top_builddir)/ggadget/gtk/libggadget-gtk@GGL_EPOCH@.la \
			  $(top_builddir)/ggadget/smjs/libggadget-smjs@GGL_EPOCH@.la

ggl_simple_LDFLAGS	= @SPIDERMONKEY_LDFLAGS@
endif
