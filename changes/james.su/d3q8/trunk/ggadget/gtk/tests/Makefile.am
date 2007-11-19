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

EXTRA_DIST		= 120day.png \
			  base.png \
			  kitty419.jpg \
			  testmask.png

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

AM_CPPFLAGS		= @CAIRO_CFLAGS@ \
			  @PANGO_CFLAGS@ \
			  @GDK2_CFLAGS@

LDADD			= @CAIRO_LIBS@ \
			  @PANGO_LIBS@ \
			  @GDK2_LIBS@ \
			  $(top_builddir)/unittest/libgunit.la \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
			  $(top_builddir)/ggadget/gtk/libggadget-gtk@GGL_EPOCH@.la


check_PROGRAMS		= cairo_canvas_test \
			  cairo_graphics_test \
			  basic_element_draw_test

cairo_canvas_test_SOURCES	= cairo_canvas_test.cc
cairo_graphics_test_SOURCES	= cairo_graphics_test.cc
basic_element_draw_test_SOURCES	= basic_element_draw_test.cc

TESTS_ENVIRONMENT	= @MEMCHECK_COMMAND@
TESTS 			= $(check_PROGRAMS)
