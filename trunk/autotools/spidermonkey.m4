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
#
# GGL_TRY_SMJS([MINIMUM-VERSION], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# Check if specified SpiderMonkey library is available.
# CPPFLAGS, CFLAGS, LIBS and LDFLAGS must be set appropriately before calling
# this function.
# ggl_extra_smjs_cppflags will be set appropriately.
#


AC_DEFUN([GGL_CHECK_SPIDERMONKEY], [
AC_ARG_WITH([smjs-cppflags],
	    AS_HELP_STRING([--with-smjs-cppflags=CPPFLAGS],
		[specify optional C precompile flags for spidermonkey library.]
		[JS_THREADSAFE flag will be detected automatically.]),
	    [smjs_cppflags=$withval],
	    [smjs_cppflags=""])

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

ggl_save_CPPFLAGS="$CPPFLAGS"
ggl_save_LIBS="$LIBS"
ggl_save_LDFLAGS="$LDFLAGS"

smjs_CPPFLAGS="$smjs_cppflags"
smjs_LDFLAGS=""
smjs_LIBS=""

# if both incdir and libdir are not specified, then try to detect it by
# pkg-config.
if test "x$smjs_incdir" = "x" -a "x$smjs_libdir" = "x"; then
  PKG_CHECK_MODULES([PKGSMJS], [xulrunner-js], [has_pkg_smjs=yes],
    [PKG_CHECK_MODULES([PKGSMJS], [firefox2-js], [has_pkg_smjs=yes],
      [PKG_CHECK_MODULES([PKGSMJS], [firefox-js], [has_pkg_smjs=yes],
        [has_pkg_smjs=no])])])
fi

if test "x$has_pkg_smjs" = "xyes"; then
  smjs_CPPFLAGS="$PKGSMJS_CFLAGS $smjs_CPPFLAGS"
  smjs_LIBS=$PKGSMJS_LIBS

  CPPFLAGS="$smjs_CPPFLAGS $ggl_save_CPPFLAGS"
  LIBS="$smjs_LIBS $ggl_save_LIBS"
  LDFLAGS="$smjs_LDFLAGS $ggl_save_LDFLAGS"
  GGL_TRY_SMJS([$min_smjs_version], [smjs_found=yes], [smjs_found=no])
fi

if test "x$smjs_found" != "xyes"; then
  case $host_os in
    *beos* ) smjs_CPPFLAGS="$smjs_CPPFLAGS -DXP_BEOS" ;;
    *os2* ) smjs_CPPFLAGS="$smjs_CPPFLAGS -DXP_OS2" ;;
    * ) smjs_CPPFLAGS="$smjs_CPPFLAGS -DXP_UNIX" ;;
  esac

  if test "x$smjs_incdir" != "x" ; then
    smjs_CPPFLAGS="$smjs_CPPFLAGS -I$smjs_incdir"
  else
    for smjs_dir in js smjs mozjs; do
      for base_dir in "$prefix/include" \
                      "/usr/include" \
                      "/usr/local/include" \
                      "/opt/local/include"; do
        if test -f "$base_dir/$smjs_dir/jsapi.h"; then
          smjs_CPPFLAGS="$smjs_CPPFLAGS -I$base_dir/$smjs_dir"
          break 2
        fi
      done
    done
  fi

  if test "x$smjs_libdir" != "x" ; then
    smjs_LDFLAGS="$smjs_LDFLAGS -L$smjs_libdir"
  fi

  CPPFLAGS="$smjs_CPPFLAGS $ggl_save_CPPFLAGS"
  LDFLAGS="$smjs_LDFLAGS $ggl_save_LDFLAGS"
  for checklib in js smjs mozjs; do
    smjs_LIBS="-l$checklib"
    LIBS="$smjs_LIBS $ggl_save_LIBS"
    GGL_TRY_SMJS([$min_smjs_version], [smjs_found=yes], [smjs_found=no])
    if test "x$smjs_found" = "xyes"; then
      break
    fi
  done
fi

if test "x$smjs_found" = "xyes" ; then
  smjs_CPPFLAGS="$smjs_CPPFLAGS $ggl_extra_smjs_cppflags"
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

CPPFLAGS=$ggl_save_CPPFLAGS
LIBS=$ggl_save_LIBS
LDFLAGS=$ggl_save_LDFLAGS
])


AC_DEFUN([GGL_TRY_SMJS], [
ggl_try_smjs_save_CPPFLAGS="$CPPFLAGS"

CPPFLAGS="-DJS_THREADSAFE $ggl_try_smjs_save_CPPFLAGS"

# First try with -DJS_THREADSAFE.
AC_LINK_IFELSE([[
  #include<jsapi.h>
  #include<jsconfig.h>

  #if JS_VERSION < $1
  #error "SpiderMonkey version is too low."
  #endif

  int main() {
    JSRuntime *runtime;
    runtime = JS_NewRuntime(1048576);
    JS_DestroyRuntime(runtime);
    JS_BeginRequest(0);
    JS_ShutDown();
    return 0;
  }
]],
[ggl_try_smjs_threadsafe=yes],
[ggl_try_smjs_threadsafe=no])

# Second try without -DJS_THREADSAFE.
CPPFLAGS="$ggl_try_smjs_save_CPPFLAGS"
AC_LINK_IFELSE([[
  #include<jsapi.h>
  #include<jsconfig.h>

  #if JS_VERSION < $1
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
[ggl_try_smjs_normal=yes],
[ggl_try_smjs_normal=no])

if test "x$ggl_try_smjs_threadsafe" = "xyes"; then
  ggl_extra_smjs_cppflags="-DJS_THREADSAFE"
  ifelse([$2], , :, [$2])
elif test "x$ggl_try_smjs_normal" = "xyes"; then
  ggl_extra_smjs_cppflags=""
  ifelse([$2], , :, [$2])
else
  ifelse([$3], , :, [$3])
fi

CPPFLAGS=$ggl_try_smjs_save_CPPFLAGS
])
