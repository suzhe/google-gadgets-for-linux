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
#

if GGL_BUILD_QT_HOST

MOC_FILES		= qt_host_internal.moc

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

noinst_HEADERS		= qt_host.h \
			  qt_host_internal.h \
			  gadget_browser_host.h

bin_PROGRAMS		= ggl-qt

ggl_qt_SOURCES		= main.cc \
			  qt_host.cc

nodist_ggl_qt_SOURCES	= $(MOC_FILES)

ggl_qt_CPPFLAGS 	= @QT4_CFLAGS@ \
			  $(PREDEFINED_MACROS) \
			  -DGGL_APP_NAME=\"ggl-qt\"

ggl_qt_CXXFLAGS		= $(DEFAULT_COMPILE_FLAGS)

ggl_qt_LDADD		= @QT4_LIBS@ \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
			  $(top_builddir)/ggadget/qt/libggadget-qt@GGL_EPOCH@.la

ggl_qt_LDFLAGS		= -R$(libdir)

BUILT_SOURCES		= $(MOC_FILES)
qt_host_internal.moc:qt_host_internal.h
	$(MOC) $< -o $@

# TODO: install these files into correct path.
all-local:
	[ ! -f $(top_builddir)/resources/resources.gg ] || cp $(top_builddir)/resources/resources.gg .
#	[ ! -f $(top_builddir)/extensions/gtkmoz_browser_element/gtkmoz_browser_child ] || cp $(top_builddir)/extensions/gtkmoz_browser_element/gtkmoz_browser_child .

noinst_SCRIPTS=ggl-qt.sh

.PHONY: ggl-qt.sh

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

desktopdir = $(datadir)/applications
desktop_in_files = ggl-qt.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

ggl-qt.desktop : ggl-qt.desktop.in
	echo $(bindir)
	$(SED) -e 's|@bindir[@]|$(bindir)|g' -e 's|@datadir[@]|$(datadir)|g' $< > $@.tmp
	$(top_builddir)/xdg/intl-desktop-file.sh $@.tmp $@
	rm -f $@.tmp

EXTRA_DIST = $(desktop_in_files) CMakeLists.txt

if ENABLE_UPDATE_DESKTOP_DATABASE
install-data-hook:
	@if test -z "$(DESTDIR)"; then \
	  $(UPDATE_DESKTOP_DATABASE) "$(DESTDIR)$(desktopdir)"; \
	fi

uninstall-hook:
	@if test -z "$(DESTDIR)"; then \
	  $(UPDATE_DESKTOP_DATABASE) "$(DESTDIR)$(desktopdir)"; \
	fi
endif

CLEANFILES 		= $(desktop_DATA) $(MOC_FILES)

endif
