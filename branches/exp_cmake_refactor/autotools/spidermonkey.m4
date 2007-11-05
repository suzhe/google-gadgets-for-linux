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
# GGL_CHECK_SPIDERMONKEY([MINIMUM-VERSION],
#                        [ACTION-IF-FOUND],
#                        [ACTION-IF-NOT-FOUND])
# Check for SpiderMonkey library, if SpiderMonkey library with version greater
# than MINIMUM-VERSION is found, ACTION-IF-FOUND will be executed,
# and SPIDERMONKEY_CPPFLAGS, SPIDERMONKEY_LIBS, SPIDERMONKEY_LDFLAGS
# will be set accordingly,
# otherwise, ACTION-IF-NOT-FOUND will be executed.
#
# MINIMUM-VERSION shall be 150 for version 1.5, or 160 for version 1.6, etc.


AC_DEFUN([GGL_CHECK_SPIDERMONKEY], [
AC_ARG_ENABLE([smjs-threadsafe],
	      AS_HELP_STRING([--enable-smjs-threadsafe],
		[use threadsafe enabled spidermonkey library]),
	      [enable_smjs_threadsafe=$enableval],
	      [enable_smjs_threadsafe=no])

AC_ARG_WITH([smjs-libdir],
	    AS_HELP_STRING([--with-smjs-libdir=DIR],
		[specify where to find spidermonkey library]),
	    [smjs_libdir=$withval],
	    [smjs_libdir=""])

AC_ARG_WITH([smjs-incdir],
	    AS_HELP_STRING([--with-smjs-incdir=DIR],
		[specify where to find spidermonkey header files]),
	    [smjs_incdir=$withval],
	    [smjs_incdir=""])

min_smjs_version=ifelse([$1], , 160, [$1])

AC_MSG_CHECKING([for SpiderMonkey version >= $min_smjs_version])

ac_save_CPPFLAGS="$CPPFLAGS"
ac_save_LIBS="$LIBS"
ac_save_LDFLAGS="$LDFLAGS"

smjs_CPPFLAGS=""
smjs_LDFLAGS=""
smjs_LIBS=""

case $host_os in
  *beos* )
    smjs_CPPFLAGS="$smjs_CPPFLAGS -DXP_BEOS"
    ;;
  *os2* )
    smjs_CPPFLAGS="$smjs_CPPFLAGS -DXP_OS2"
    ;;
  *interix* | *mks* | *winnt* | *cygwin* | *mingw* )
    smjs_CPPFLAGS="$smjs_CPPFLAGS -DXP_WIN"
    ;;
  * )
    smjs_CPPFLAGS="$smjs_CPPFLAGS -DXP_UNIX"
    ;;
esac

if test "x$enable_smjs_threadsafe" = "xyes" ; then
  smjs_CPPFLAGS="$smjs_CPPFLAGS -DJS_THREADSAFE"
fi

if test "x$smjs_incdir" != "x" ; then
  smjs_CPPFLAGS="$smjs_CPPFLAGS -I$smjs_incdir"
elif test -f "$prefix/include/js/jsapi.h" ; then
  smjs_CPPFLAGS="$smjs_CPPFLAGS -I$prefix/include/js"
elif test -f "/usr/include/js/jsapi.h" ; then
  smjs_CPPFLAGS="$smjs_CPPFLAGS -I/usr/include/js"
elif test -f "/usr/local/include/js/jsapi.h" ; then
  smjs_CPPFLAGS="$smjs_CPPFLAGS -I/usr/local/include/js"
elif test -f "/opt/local/include/js/jsapi.h" ; then
  smjs_CPPFLAGS="$smjs_CPPFLAGS -I/opt/local/include/js"
fi

if test "x$smjs_libdir" != "x" ; then
  smjs_LDFLAGS="$smjs_LDFLAGS -L$smjs_libdir"
fi

CPPFLAGS="$smjs_CPPFLAGS $CPPFLAGS"
LDFLAGS="$smjs_LDFLAGS $LDFLAGS"

for checklib in js smjs; do
  smjs_LIBS="-l$checklib"
  LIBS="$smjs_LIBS $LIBS"

  AC_LINK_IFELSE([[
#include<jsapi.h>
#include<jsconfig.h>

#if JS_VERSION < $min_smjs_version
#error "SpiderMonkey version is too low."
#endif

int main() {
  JSRuntime *runtime;
  runtime = JS_NewRuntime(1048576);
  JS_DestroyRuntime(runtime);
  JS_ShutDown();
  return 0;
}
  ]],
  [ac_have_smjs=yes],
  [ac_have_smjs=no])

  if test "x$ac_have_smjs" = "xyes"; then
    break
  fi
  LIBS="$ac_save_LIBS"
done

if test "x$ac_have_smjs" = "xyes" ; then
  SPIDERMONKEY_CPPFLAGS="$smjs_CPPFLAGS"
  SPIDERMONKEY_LIBS="$smjs_LIBS"
  SPIDERMONKEY_LDFLAGS="$smjs_LDFLAGS"
  AC_SUBST(SPIDERMONKEY_CPPFLAGS)
  AC_SUBST(SPIDERMONKEY_LIBS)
  AC_SUBST(SPIDERMONKEY_LDFLAGS)
  AC_MSG_RESULT([yes (CPPFLAGS=$smjs_CPPFLAGS  LIBS=$smjs_LIBS  LDFLAGS=$smjs_LDFLAGS)])
  ifelse([$2], , :, [$2])
else
  AC_MSG_RESULT([no])
  ifelse([$3], , :, [$3])
fi

CPPFLAGS=$ac_save_CPPFLAGS
LIBS=$ac_save_LIBS
LDFLAGS=$ac_save_LDFLAGS
])
