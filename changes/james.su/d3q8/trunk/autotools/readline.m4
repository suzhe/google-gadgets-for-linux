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
# GGL_CHECK_READLINE([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# Check for gnu readline library, if readline library is found,
# ACTION-IF-FOUND will be executed,
# and READLINE_CPPFLAGS, READLINE_LIBS, READLINE_LDFLAGS
# will be set accordingly,
# otherwise, ACTION-IF-NOT-FOUND will be executed.


AC_DEFUN([GGL_CHECK_READLINE], [
AC_ARG_WITH([readline-libdir],
	    AS_HELP_STRING([--with-readline-libdir=DIR],
		[specify where to find readline library]),
	    [readline_libdir=$withval],
	    [readline_libdir=""])

AC_ARG_WITH([readline-incdir],
	    AS_HELP_STRING([--with-readline-incdir=DIR],
		[specify where to find readline header files, without 'readline' subdir.]),
	    [readline_incdir=$withval],
	    [readline_incdir=""])

AC_MSG_CHECKING([for readline library.])

ac_save_CPPFLAGS="$CPPFLAGS"
ac_save_LIBS="$LIBS"
ac_save_LDFLAGS="$LDFLAGS"

readline_CPPFLAGS=""
readline_LDFLAGS=""
readline_LIBS="-lreadline -lncurses"

if test "x$readline_incdir" != "x" ; then
  readline_CPPFLAGS="$readline_CPPFLAGS -I$readline_incdir"
elif test -f "$prefix/include/readline.h" ; then
  readline_CPPFLAGS="$readline_CPPFLAGS -I$prefix/include"
elif test -f "/usr/include/readline.h" ; then
  readline_CPPFLAGS="$readline_CPPFLAGS -I/usr/include"
elif test -f "/usr/local/include/readline.h" ; then
  readline_CPPFLAGS="$readline_CPPFLAGS -I/usr/local/include"
elif test -f "/opt/local/include/readline.h" ; then
  readline_CPPFLAGS="$readline_CPPFLAGS -I/opt/local/include"
fi

if test "x$readline_libdir" != "x" ; then
  readline_LDFLAGS="$readline_LDFLAGS -L$readline_libdir"
fi

CPPFLAGS="$readline_CPPFLAGS $CPPFLAGS"
LDFLAGS="$readline_LDFLAGS $LDFLAGS"
LIBS="$readline_LIBS $LIBS"

AC_LINK_IFELSE([[
#include<stdio.h>
#include<readline/readline.h>
#include<readline/history.h>

int main() {
  readline("Hello");
  add_history("Hello");
  return 0;
}
]],
[ac_have_readline=yes],
[ac_have_readline=no])

if test "x$ac_have_readline" = "xyes" ; then
  READLINE_CPPFLAGS="$readline_CPPFLAGS"
  READLINE_LIBS="$readline_LIBS"
  READLINE_LDFLAGS="$readline_LDFLAGS"
  AC_SUBST(READLINE_CPPFLAGS)
  AC_SUBST(READLINE_LIBS)
  AC_SUBST(READLINE_LDFLAGS)
  AC_MSG_RESULT([yes (CPPFLAGS=$readline_CPPFLAGS  LIBS=$readline_LIBS  LDFLAGS=$readline_LDFLAGS)])
  ifelse([$1], , :, [$1])
else
  AC_MSG_RESULT([no])
  ifelse([$2], , :, [$2])
fi

CPPFLAGS=$ac_save_CPPFLAGS
LIBS=$ac_save_LIBS
LDFLAGS=$ac_save_LDFLAGS
])
