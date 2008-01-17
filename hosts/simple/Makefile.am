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

if GGL_BUILD_SIMPLE_HOST

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

noinst_HEADERS		=

bin_PROGRAMS		= ggl-simple

ggl_simple_SOURCES	= main.cc

ggl_simple_CPPFLAGS 	= @CAIRO_CFLAGS@ \
			  @GTK2_CFLAGS@ \
			  @SPIDERMONKEY_CPPFLAGS@ \
			  $(PREDEFINED_MACROS)

ggl_simple_CXXFLAGS	= $(DEFAULT_COMPILE_FLAGS)

if GGL_HOST_LINUX
SYSDEPS_LIB=$(top_builddir)/ggadget/linux/libggadget-linux@GGL_EPOCH@.la
endif

ggl_simple_LDADD	= @CAIRO_LIBS@ \
			  @GTK2_LIBS@ \
			  @SPIDERMONKEY_LIBS@ \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
			  $(top_builddir)/ggadget/gtk/libggadget-gtk@GGL_EPOCH@.la \
			  $(top_builddir)/ggadget/smjs/libggadget-smjs@GGL_EPOCH@.la \
			  $(SYSDEPS_LIB)

ggl_simple_LDFLAGS	= @SPIDERMONKEY_LDFLAGS@

# TODO: install these files into correct path.
all-local:
	[ ! -f $(top_builddir)/extensions/gtkmoz_browser_element/gtkmoz_browser_child ] || cp $(top_builddir)/extensions/gtkmoz_browser_element/gtkmoz_browser_child .
	[ ! -f $(top_builddir)/resources/ggl_resources.bin ] || cp $(top_builddir)/resources/ggl_resources.bin .

noinst_SCRIPTS=ggl-simple.sh

.PHONY: ggl-simple.sh

ggl-simple.sh: ggl-simple
	(echo GGL_MODULE_PATH=$(abs_top_builddir)/extensions/dbus_script_class:$(abs_top_builddir)/extensions/gtkmoz_browser_element\
	 $(LIBTOOL) --mode=execute $(abs_builddir)/ggl-simple '$$@' > $@)

endif
