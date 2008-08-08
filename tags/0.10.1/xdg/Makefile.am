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

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

noinst_PROGRAMS		= intl-desktop-file

intl_desktop_file_SOURCES= intl_desktop_file.cc
intl_desktop_file_CPPFLAGS= $(PREDEFINED_MACROS)
intl_desktop_file_CXXFLAGS= $(DEFAULT_COMPILE_FLAGS)
intl_desktop_file_LDADD= $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la
intl_desktop_file_LDFLAGS = -R$(libdir)

noinst_SCRIPTS=intl-desktop-file.sh

.PHONY: intl-desktop-file.sh

intl-desktop-file.sh: intl-desktop-file
	(echo '#!/bin/sh' > $@; \
	 echo 'GGL_MODULE_PATH=`pwd`' >> $@; \
	 echo 'for i in $(abs_top_builddir)/extensions/*; do' >> $@; \
	 echo '  if test -d $$i; then' >> $@; \
	 echo '    GGL_MODULE_PATH=$$GGL_MODULE_PATH:$$i' >> $@; \
	 echo '  fi' >> $@; \
	 echo 'done' >> $@; \
	 echo 'export GGL_MODULE_PATH' >> $@; \
	 echo '$(abs_builddir)/intl-desktop-file $$@' >> $@; \
	 chmod 0755 $@)

# To make sure our mime type has higher priority.
xml_files = 00-google-gadgets.xml

mimedir = $(datadir)/mime/packages
mime_DATA = $(xml_files)

EXTRA_DIST = $(xml_files)

if ENABLE_UPDATE_MIME_DATABASE
install-data-hook:
	$(UPDATE_MIME_DATABASE) "$(DESTDIR)$(datadir)/mime"

uninstall-hook:
	$(UPDATE_MIME_DATABASE) "$(DESTDIR)$(datadir)/mime"
endif
