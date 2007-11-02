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

noinst_HEADERS		= crypt.h \
			  ioapi.h \
			  mztools.h \
			  unzip.h \
			  zip.h

noinst_LTLIBRARIES	= libunzip.la

libunzip_la_SOURCES	= ioapi.c \
			  mztools.c \
			  unzip.c \
			  zip.c

libunzip_la_CPPFLAGS	= @ZLIB_CPPFLAGS@
libunzip_la_LDFLAGS	= @ZLIB_LDFLAGS@
libunzip_la_LIBADD	= @ZLIB_LIBS@
