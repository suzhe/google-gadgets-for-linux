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

if GGL_BUILD_QT_HOST

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

noinst_HEADERS		=

bin_PROGRAMS		= ggl-qt

ggl_qt_SOURCES	= main.cc

ggl_qt_CPPFLAGS 	= @QT4_CFLAGS@ \
			  @SPIDERMONKEY_CPPFLAGS@ \
			  $(PREDEFINED_MACROS)

ggl_qt_CXXFLAGS	= $(DEFAULT_COMPILE_FLAGS)

ggl_qt_LDADD	= @QT4_LIBS@ \
			  @SPIDERMONKEY_LIBS@ \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
			  $(top_builddir)/ggadget/qt/libggadget-qt@GGL_EPOCH@.la \
			  $(SYSDEPS_LIB)

ggl_qt_LDFLAGS	= @SPIDERMONKEY_LDFLAGS@

# TODO: install these files into correct path.
#all-local:
#	[ ! -f $(top_builddir)/extensions/gtkmoz_browser_element/gtkmoz_browser_child ] || cp $(top_builddir)/extensions/gtkmoz_browser_element/gtkmoz_browser_child .
#	[ ! -f $(top_builddir)/resources/ggl_resources.bin ] || cp $(top_builddir)/resources/ggl_resources.bin .

noinst_SCRIPTS=ggl-qt.sh

.PHONY: ggl-qt.sh

ggl-qt.sh: ggl-qt
	(echo GGL_MODULE_PATH=$(abs_top_builddir)/extensions/dbus_script_class:$(abs_top_builddir)/extensions/gtkmoz_browser_element\
	 $(LIBTOOL) --mode=execute $(abs_builddir)/ggl-qt '$$@' > $@)

main.moc:main.cc
	moc $< -o $@

ggl-qt.sh: ggl-qt
	(echo '#!/bin/sh' > $@; \
	 echo 'GGL_MODULE_PATH=`pwd`' >> $@; \
	 echo 'for i in $(abs_top_builddir)/extensions/*; do' >> $@; \
	 echo '  if test -d $$i; then' >> $@; \
	 echo '    GGL_MODULE_PATH=$$GGL_MODULE_PATH:$$i' >> $@; \
	 echo '  fi' >> $@; \
	 echo 'done' >> $@; \
	 echo 'export GGL_MODULE_PATH' >> $@; \
	 echo '$(LIBTOOL) --mode=execute $$MEMCHECK_COMMAND $(abs_builddir)/ggl-qt $$@' >> $@; \
	 chmod 0755 $@)

endif
