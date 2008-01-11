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

EXTRA_DIST		= js_unittest.js

noinst_HEADERS		= gunit-internal-inl.h \
			  gunit-internal.h \
			  gunit.h \
			  gunit_pred_impl.h \
			  gunit_prod.h

noinst_LTLIBRARIES	= libgunit.la

libgunit_la_SOURCES	= gunit.cc

libgunit_la_CXXFLAGS	= $(DEFAULT_COMPILE_FLAGS)
libgunit_la_CPPFLAGS	= $(PREDEFINED_MACROS)

if !GGL_HOST_WINDOWS
libgunit_la_CPPFLAGS	+= -DOS_LINUX
endif
