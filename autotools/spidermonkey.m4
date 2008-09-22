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

smjs_save_CPPFLAGS="$CPPFLAGS"
smjs_save_LIBS="$LIBS"
smjs_save_LDFLAGS="$LDFLAGS"

smjs_CPPFLAGS="$smjs_cppflags"
smjs_LDFLAGS=""
smjs_LIBS=""

# if both incdir and libdir are not specified, then try to detect it by
# pkg-config.
has_pkg_smjs=no
if test "x$smjs_incdir" = "x" -a "x$smjs_libdir" = "x"; then
  PKG_CHECK_MODULES([PKGSMJS], [mozilla-js], [has_pkg_smjs=mozilla-js],
    [PKG_CHECK_MODULES([PKGSMJS], [xulrunner-js], [has_pkg_smjs=xulrunner-js],
      [PKG_CHECK_MODULES([PKGSMJS], [firefox2-js], [has_pkg_smjs=firefox2-js],
        [PKG_CHECK_MODULES([PKGSMJS], [firefox-js], [has_pkg_smjs=firefox-js],
          [PKG_CHECK_MODULES([PKGSMJS], [libjs], [has_pkg_smjs=libjs],
            [has_pkg_smjs=no])])])])])
fi

if test "x$has_pkg_smjs" != "xno"; then
# Fix bogus mozilla-js.pc of xulrunner 1.9 pre release version.
  if (echo $PKGSMJS_CFLAGS | grep '/stable') > /dev/null 2>&1; then
    smjs_incdir=`$PKG_CONFIG --variable=includedir $has_pkg_smjs`
    if test -f $smjs_incdir/unstable/jsapi.h; then
      PKGSMJS_CFLAGS="$PKGSMJS_CFLAGS -I$smjs_incdir/unstable"
    fi
  fi
  smjs_CPPFLAGS="$PKGSMJS_CFLAGS $smjs_CPPFLAGS"
  smjs_LIBS=$PKGSMJS_LIBS

  smjs_libdir=`$PKG_CONFIG --variable=libdir $has_pkg_smjs`
  if test "x$smjs_libdir" = "x"; then
# Some distributions doesn't have libdir, so try to extract libdir from LIBS.
# @<:@ and @:>@ will be replace by [ and ] by M4. Really evil.
    smjs_libdir=`echo $PKGSMJS_LIBS | sed -e 's/.*-L\(@<:@^ @:>@*\) .*/\1/'`
    if test ! -e "$smjs_libdir"; then
      smjs_libdir=""
    elif test -e "$smjs_libdir/libmozjs.so"; then
      # Try to find out the correct xulrunner path.
      MOZJS_PATH=`readlink -f $smjs_libdir/libmozjs.so`
      smjs_libdir=`dirname $MOZJS_PATH`
    fi
  fi

  CPPFLAGS="$smjs_CPPFLAGS"
  LIBS="$smjs_LIBS"
  LDFLAGS="$smjs_LDFLAGS"
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

  CPPFLAGS="$smjs_CPPFLAGS"
  LDFLAGS="$smjs_LDFLAGS"
  for checklib in js smjs mozjs; do
    smjs_LIBS="-l$checklib"
    LIBS="$smjs_LIBS"
    GGL_TRY_SMJS([$min_smjs_version], [smjs_found=yes], [smjs_found=no])
    if test "x$smjs_found" = "xyes"; then
      break
    fi
  done
fi

if test "x$smjs_found" = "xyes" ; then
  smjs_CPPFLAGS="$smjs_CPPFLAGS $ggl_extra_smjs_cppflags"
  if test "x$smjs_libdir" != "x" -a "x$smjs_libdir" != "x$libdir"; then
    smjs_LDFLAGS="$smjs_LDFLAGS -R$smjs_libdir"
  fi
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

CPPFLAGS=$smjs_save_CPPFLAGS
LIBS=$smjs_save_LIBS
LDFLAGS=$smjs_save_LDFLAGS
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
  ggl_has_smjs=yes
elif test "x$ggl_try_smjs_normal" = "xyes"; then
  ggl_extra_smjs_cppflags=""
  ggl_has_smjs=yes
else
  ggl_has_smjs=no
fi

# test if MOZILLA_1_8_BRANCH macro is necessary.
if test "x$ggl_has_smjs" = "xyes"; then
CPPFLAGS="$ggl_try_smjs_save_CPPFLAGS $ggl_extra_smjs_cppflags"
CPPFLAGS="$CPPFLAGS -DMOZILLA_1_8_BRANCH"
AC_RUN_IFELSE([[
// This file is used to test if MOZILLA_1_8_BRANCH should be defined to use
// the SpiderMonkey library.
// It will fail to execute (crash or return error code) if MOZILLA_1_8_BRANCH
// macro is not defined but the library was compiled with the flag.

#include <jsapi.h>

static JSBool f(JSContext *c, JSObject *o, uintN ac, jsval *av, jsval *r) {
  return JS_TRUE;
}

int main() {
  JSRuntime *rt;
  JSContext *cx;
  JSObject *obj;
  JSFunctionSpec funcs[] = {
    { "a", f, 5, JSFUN_HEAVYWEIGHT, 0 },
    { "b", f, 5, JSFUN_HEAVYWEIGHT, 0 },
    { NULL, NULL, 0, 0, 0 },
  };

  rt = JS_NewRuntime(1048576);
  cx = JS_NewContext(rt, 8192);
  obj = JS_NewObject(cx, 0, 0, 0);
  JS_SetGlobalObject(cx, obj);
  // If MOZILLA_1_8_BRANCH is not properly defined, this JS_DefineFunctions
  // may crash or define only the first function because of the different
  // sizes of the nargs and flags fields.
  JS_DefineFunctions(cx, obj, funcs);
  jsval v;
  if (!JS_GetProperty(cx, obj, "b", &v) || !JSVAL_IS_OBJECT(v) ||
      !JS_ObjectIsFunction(cx, JSVAL_TO_OBJECT(v)) ||
      JS_GetFunctionFlags(JS_ValueToFunction(cx, v)) != JSFUN_HEAVYWEIGHT)
    return 1;

  JS_DestroyContext(cx);
  JS_DestroyRuntime(rt);
  JS_ShutDown();
  return 0;
}
]],
[ggl_try_smjs_18_branch=yes],
[ggl_try_smjs_18_branch=no],
[
  AC_MSG_WARN([Can't determine the status of MOZILLA_1_8_BRANCH
               macro in cross compile mode.])
  ggl_try_smjs_18_branch=no
])
fi

if test "x$ggl_try_smjs_18_branch" = "xyes"; then
  ggl_extra_smjs_cppflags="$ggl_extra_smjs_cppflags -DMOZILLA_1_8_BRANCH"
fi

if test "x$ggl_has_smjs" = "xyes"; then
  ifelse([$2], , :, [$2])
else
  ifelse([$3], , :, [$3])
fi

CPPFLAGS=$ggl_try_smjs_save_CPPFLAGS
])
