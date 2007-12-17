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

EXTRA_DIST		= dom1_test.js \
			  json_test.js \
			  wrapper_test.js

if GGL_HAVE_READLINE
INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

AM_CPPFLAGS		= @READLINE_CPPFLAGS@ \
			  @SPIDERMONKEY_CPPFLAGS@

AM_LDFLAGS		= @READLINE_LDFLAGS@ \
			  @SPIDERMONKEY_LDFLAGS@

LDADD			= @READLINE_LIBS@ \
			  @SPIDERMONKEY_LIBS@ \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
			  $(top_builddir)/ggadget/smjs/libggadget-smjs@GGL_EPOCH@.la

check_PROGRAMS		= js_shell \
			  wrapper_test_shell \
			  dom_test_shell \
			  jscript_massager_test \
			  massager

js_shell_SOURCES	= js_shell.cc \
			  init_custom_objects_stub.cc

wrapper_test_shell_SOURCES = js_shell.cc \
			     wrapper_test_shell.cc \
			     $(top_srcdir)/ggadget/tests/scriptables.cc

dom_test_shell_SOURCES	= js_shell.cc \
			  dom_test_shell.cc

jscript_massager_test_SOURCES = jscript_massager_test.cc

massager_SOURCES	= massager.cc

TESTS_ENVIRONMENT	= @SHELL@
TESTS			= js_shell.sh \
			  wrapper_test_shell.sh \
			  dom_test_shell.sh \
			  jscript_massager_test

js_shell.sh: Makefile
	echo @MEMCHECK_COMMAND@ $(abs_builddir)/js_shell \
	     $(abs_top_srcdir)/unittest/js_unittest.js \
	     $(abs_srcdir)/json_test.js > $@

wrapper_test_shell.sh: Makefile
	echo @MEMCHECK_COMMAND@ $(abs_builddir)/wrapper_test_shell \
	     $(abs_top_srcdir)/unittest/js_unittest.js \
	     $(abs_srcdir)/wrapper_test.js > $@

dom_test_shell.sh: Makefile
	echo @MEMCHECK_COMMAND@ $(abs_builddir)/dom_test_shell \
	     $(abs_top_srcdir)/unittest/js_unittest.js \
	     $(abs_srcdir)/dom1_test.js > $@

endif
