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

if GGL_BUILD_GTK_HOST

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

noinst_HEADERS		= gadget_browser_host.h \
			  gtk_host_base.h \
			  simple_gtk_host.h \
			  sidebar_gtk_host.h \
			  standalone_gtk_host.h

bin_PROGRAMS		= ggl-gtk

ggl_gtk_SOURCES		= main.cc \
			  gtk_host_base.cc \
			  sidebar_gtk_host.cc \
			  simple_gtk_host.cc \
			  standalone_gtk_host.cc


ggl_gtk_CPPFLAGS 	= @GTK2_CFLAGS@ \
			  $(PREDEFINED_MACROS) \
			  -DGGL_APP_NAME=\"ggl-gtk\"

ggl_gtk_CXXFLAGS	= $(DEFAULT_COMPILE_FLAGS)

ggl_gtk_LDADD	= @GTK2_LIBS@ \
		  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
		  $(top_builddir)/ggadget/gtk/libggadget-gtk@GGL_EPOCH@.la \
		  $(top_builddir)/ggadget/npapi/libggadget-npapi@GGL_EPOCH@.la

ggl_gtk_LDFLAGS = -R$(libdir)

# TODO: install these files into correct path.
all-local:
	[ ! -f $(top_builddir)/extensions/gtkmoz_browser_element/gtkmoz-browser-child ] || cp $(top_builddir)/extensions/gtkmoz_browser_element/gtkmoz-browser-child .
	[ ! -f $(top_builddir)/resources/resources.gg ] || cp $(top_builddir)/resources/resources.gg .

noinst_SCRIPTS=ggl-gtk.sh

.PHONY: ggl-gtk.sh

ggl-gtk.sh: ggl-gtk
	(echo '#!/bin/sh' > $@; \
	 echo 'GGL_MODULE_PATH=`pwd`' >> $@; \
	 echo 'for i in $(abs_top_builddir)/extensions/*; do' >> $@; \
	 echo '  if test -d $$i; then' >> $@; \
	 echo '    GGL_MODULE_PATH=$$GGL_MODULE_PATH:$$i' >> $@; \
	 echo '  fi' >> $@; \
	 echo 'done' >> $@; \
	 echo 'export GGL_MODULE_PATH' >> $@; \
	 echo '$(LIBTOOL) --mode=execute $$MEMCHECK_COMMAND $(abs_builddir)/ggl-gtk $$@' >> $@; \
	 chmod 0755 $@)

desktopdir = $(datadir)/applications
desktop_in_files = ggl-gtk.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

ggl-gtk.desktop : ggl-gtk.desktop.in
	echo $(bindir)
	$(SED) -e 's|@bindir[@]|$(bindir)|g' -e 's|@datadir[@]|$(datadir)|g' $< > $@.tmp
	$(top_builddir)/xdg/intl-desktop-file.sh $@.tmp $@
	rm -f $@.tmp

EXTRA_DIST = $(desktop_in_files) CMakeLists.txt
CLEANFILES = $(desktop_DATA)

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

endif
