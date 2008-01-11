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

# Process this file with autoconf to produce a configure script

# Specifies:
#  Package name
#  Version
#  Bug report address
AC_INIT([google-gadgets-for-linux],
	[0.0.1],
	[google-gadgets-for-linux-dev@googlegroups.com])

# Requires autoconf version >= 2.50
AC_PREREQ([2.50])

# Specifies a source file in the source directory.
AC_CONFIG_SRCDIR([ggadget/ggadget.h])

# Tells autoconf where to find necessary build scripts and macros.
AC_CONFIG_AUX_DIR([autotools])
AC_CONFIG_MACRO_DIR([autotools])


# Save the user defined CFLAGS and CXXFLAGS, then restore them later.
# So that the value set by AC_PROG_CC and AC_PROG_CXX won't be used,
# even if they are empty.
ggl_save_CFLAGS=$CFLAGS
ggl_save_CXXFLAGS=$CXXFLAGS

###########################################################
##          Some definitions of Version macros.          ##
###########################################################

# Version numbers of Google Gadgets for Linux package.
GGL_MAJOR_VERSION=0
GGL_MINOR_VERSION=0
GGL_MICRO_VERSION=1

GGL_VERSION=$GGL_MAJOR_VERSION.$GGL_MINOR_VERSION.$GGL_MICRO_VERSION

AC_SUBST(GGL_MAJOR_VERSION)
AC_SUBST(GGL_MINOR_VERSION)
AC_SUBST(GGL_MICRO_VERSION)
AC_SUBST(GGL_VERSION)


# libtool versioning for libggadget

# Increment if the interface has additions, changes, removals.
LIBGGADGET_CURRENT=0

# Increment any time the source changes; set to 0 if you increment
# LIBGGADGET_CURRENT
LIBGGADGET_REVISION=0

# Increment if any interfaces have been added; set to 0
# if any interfaces have been removed. removal has
# precedence over adding, so set to 0 if both happened.
LIBGGADGET_AGE=0

AC_SUBST(LIBGGADGET_CURRENT)
AC_SUBST(LIBGGADGET_REVISION)
AC_SUBST(LIBGGADGET_AGE)

# Define a string for the earliest version that this release has
# binary compatibility with. This is used for module locations.
#
LIBGGADGET_BINARY_VERSION=1.0.0
AC_SUBST(LIBGGADGET_BINARY_VERSION)

AC_DEFINE_UNQUOTED(LIBGGADGET_BINARY_VERSION, "$LIBGGADGET_BINARY_VERSION",
		   [The binary version of Gadgets runtime library.])
AC_DEFINE_UNQUOTED(GGL_VERSION, "$GGL_VERSION",
		   [The release version of Google Gadgets for Linux.])
AC_DEFINE_UNQUOTED(GGL_MAJOR_VERSION, $GGL_MAJOR_VERSION,
		   [The Major version of Google Gadgets for Linux.])
AC_DEFINE_UNQUOTED(GGL_MINOR_VERSION, $GGL_MINOR_VERSION,
		   [The Minor version of Google Gadgets for Linux.])
AC_DEFINE_UNQUOTED(GGL_MICRO_VERSION, $GGL_MICRO_VERSION,
		   [The Micro version of Google Gadgets for Linux.])

# Define a string for the earliest version that this release cannot
# coexist with. This is used for directory hierarchies.
#
GGL_EPOCH=-1.0
AC_SUBST(GGL_EPOCH)

GGL_BASE_DIR=ggl$GGL_EPOCH
AC_SUBST(GGL_BASE_DIR)

###########################################################
##                      Config options.                  ##
###########################################################
AC_ARG_ENABLE([debug],
	      AS_HELP_STRING([--enable-debug],[enable debug]),
	      [enable_debug=$enableval],
	      [enable_debug=no])

AC_ARG_WITH([memcheck-command],
	    AS_HELP_STRING([--with-memcheck-command=CMD],
		[specify a memory check command for running unit tests.]),
	    [memcheck_command=$withval],
	    [memcheck_command=""])

###########################################################
##                  Start Configuration.                 ##
###########################################################

# Init automake stuff
AM_INIT_AUTOMAKE

# Check build and host system types
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

# Initializes libtool
AC_LIBLTDL_INSTALLABLE
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL
AC_SUBST(LIBLTDL)
AC_SUBST(LTDLINCL)
AC_SUBST(LIBTOOL_DEPS)
AC_CONFIG_SUBDIRS([libltdl])
AM_CONDITIONAL(GGL_BUILD_LIBLTDL, [test x$enable_ltdl_install = xyes])

# Initialize pkg-config
PKG_PROG_PKG_CONFIG([0.20])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX

# Checks for zip command.
AC_PATH_PROG([ZIP], [zip])

# Do compile test for C and C++ source code.
AC_LANG(C)
AC_LANG(C++)

# Checks for standard header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdint.h],[has_stdint=yes],[has_stdint=no])

if test x$has_stdint = xno; then
  AC_MSG_ERROR([Google Gadgets for Linux depends on stdint.h])
fi

# Check necessary data types
AC_TYPE_SIZE_T
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long int)
AC_CHECK_SIZEOF(size_t)
AC_CHECK_SIZEOF(double)

GGL_SIZEOF_INT=$ac_cv_sizeof_int
GGL_SIZEOF_LONG_INT=$ac_cv_sizeof_long_int
GGL_SIZEOF_SIZE_T=$ac_cv_sizeof_size_t
GGL_SIZEOF_DOUBLE=$ac_cv_sizeof_double

AC_SUBST(GGL_SIZEOF_INT)
AC_SUBST(GGL_SIZEOF_LONG_INT)
AC_SUBST(GGL_SIZEOF_SIZE_T)
AC_SUBST(GGL_SIZEOF_DOUBLE)

if test x$ac_cv_type_size_t != xyes; then
  AC_MSG_ERROR([No type size_t, but google Gadgets for Linux needs it!])
fi

# check for bytesex stuff
AC_C_BIGENDIAN(
  [AC_DEFINE(GGL_BIG_ENDIAN,1,[Big endian])],
  [AC_DEFINE(GGL_LITTLE_ENDIAN,1,[Little endian])]
)

# Check necessary libraries

# Check gnu readline lib, for smjs unit tests.
GGL_CHECK_READLINE([has_readline=yes], [has_readline=no])

# Check zlib
GGL_CHECK_ZLIB([1.2.3], [has_zlib=yes], [has_zlib=no])

# Check pango related libraries.
PKG_CHECK_MODULES(PANGO, [pango  >= 1.10.0], [has_pango=yes], [has_pango=no])

# Check cairo.
PKG_CHECK_MODULES(CAIRO, [cairo >= 1.0.0], [has_cairo=yes], [has_cairo=no])

# Check gtk+-2.0 related libraries.
PKG_CHECK_MODULES(GTK2, [gtk+-2.0 >= 2.8.0], [has_gtk2=yes], [has_gtk2=no])

# Check gdk-2.0 related libraries.
PKG_CHECK_MODULES(GDK2, [gdk-2.0 >= 2.8.0], [has_gdk2=yes], [has_gdk2=no])

# Check gstreamer-0.10 related libraries.
PKG_CHECK_MODULES(GSTREAMER, [gstreamer-0.10 >= 0.10.0], [has_gstreamer=yes], [has_gstreamer=no])

# Check libcurl.
LIBCURL_CHECK_CONFIG([yes], [7.16.0], [has_libcurl=yes], [has_libcurl=no])

# Check libxml2.
AM_PATH_XML2([2.4.0], [has_libxml2=yes], [has_libxml2=no])

# Check spidermonkey library
GGL_CHECK_SPIDERMONKEY([160], [has_spidermonkey=yes], [has_spidermonkey=no])

# Check D-Bus library
PKG_CHECK_MODULES(DBUS, dbus-1 >= 1.0.2, [has_dbus=yes], [has_dbus=no])

# Check librsvg
PKG_CHECK_MODULES(RSVG, librsvg-2.0 >= 2.14.0, [has_rsvg=yes], [has_rsvg=no])

# Check gtkmozembed
PKG_CHECK_MODULES(GTKMOZEMBED, [xulrunner-gtkmozembed xulrunner-js], [has_gtkmozembed=yes],
  [PKG_CHECK_MODULES(GTKMOZEMBED, [firefox2-gtkmozembed firefox2-js], [has_gtkmozembed=yes],
    [PKG_CHECK_MODULES(GTKMOZEMBED, [firefox-gtkmozembed firefox-js], [has_gtkmozembed=yes],
      [has_gtkmozembed=no])])])

if test x$has_dbus = xyes; then
  if test -z "$DBUS_SYSTEM_BUS_SOCKET"; then
    # use default
    DBUS_SYSTEM_BUS_SOCKET="unix:path=/var/run/dbus/system_bus_socket"
  fi
  AC_DEFINE_UNQUOTED(DBUS_SYSTEM_BUS_SOCKET, ["$DBUS_SYSTEM_BUS_SOCKET"],
                     [dbus system bus socket path])
  AC_DEFINE(HAVE_DBUS_LIBRARY,1,[Have dbus library])
fi

if test x$has_rsvg = xyes; then
  AC_DEFINE(HAVE_RSVG_LIBRARY,1,[Have rsvg library])
fi

###########################################################
##                  Process Config options.              ##
###########################################################

build_libggadget_gtk=yes
build_libggadget_smjs=yes
build_simple_host=yes
build_libggadget_dbus=yes
build_libggadget_gtkmoz=yes

if test x$has_libcurl != xyes; then
  AC_MSG_ERROR([Library curl is not available.])
fi

if test x$has_libxml2 != xyes; then
  AC_MSG_ERROR([Library xml2 is not available.])
fi

if test x$has_zlib != xyes; then
  AC_MSG_ERROR([Library zlib is not available.])
fi

if test x$has_gstreamer != xyes; then
  AC_MSG_WARN([Library gstreamer is not available.])
else
  AC_DEFINE(GST_AUDIOCLIP,1,[Use audioclip implementation based on gstreamer])
fi

if test x$has_cairo != xyes -o x$has_pango != xyes -o x$has_gdk2 != xyes; then
  AC_MSG_WARN([Library cairo, pango or gdk-2.0 are not available, libggadget-cairo and simple host won't be built])
  build_libggadget_gtk=no
  build_simple_host=no
fi

if test x$has_gtk2 != xyes; then
  AC_MSG_WARN([Library gtk-2.0 is not available, simple host won't be built])
  build_simple_host=no
fi

if test x$has_spidermonkey != xyes; then
  AC_MSG_WARN([Library SpiderMonkey is not available, libggadget-smjs and simple host won't be built.])
  build_libggadget_smjs=no
  build_simple_host=no
fi

if test x$has_dbus != xyes; then
  AC_MSG_WARN([Library D-Bus is not available, libggadget-dbus won't be built.])
  build_libggadget_dbus=no
fi

if test x$has_gtkmozembed != xyes; then
  AC_MSG_WARN([Library GtkMozEmbed is not available, libggadget-gtkmoz won't be built.])
  build_libggadget_gtkmoz=no
fi

AM_CONDITIONAL(
  [GGL_BUILD_LIBGGADGET_GTK],
  [test x$build_libggadget_gtk = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_LIBGGADGET_SMJS],
  [test x$build_libggadget_smjs = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_SIMPLE_HOST],
  [test x$build_simple_host = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_LIBGGADGET_DBUS],
  [test x$build_libggadget_dbus = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_LIBGGADGET_GTKMOZ],
  [test x$build_libggadget_gtkmoz = xyes])

AM_CONDITIONAL([GGL_HAVE_ZIP], [test x$ZIP != x])
AM_CONDITIONAL([GGL_HAVE_READLINE], [test x$has_readline = xyes])
AM_CONDITIONAL([GGL_HAVE_RSVG], [test x$has_rsvg = xyes])

if test x$ZIP = x; then
  AC_MSG_WARN([Can't generate resource file bundle without zip command.])
fi

# OS Specific conditions.
case $host_os in
  *interix* | *mks* | *winnt* | *cygwin* | *mingw* )
    ggl_host_type=windows
    AC_DEFINE(GGL_HOST_WINDOWS,1,[Windows host])
    ;;
  *darwin* )
    ggl_host_type=macosx
    AC_DEFINE(GGL_HOST_MACOSX,1,[Mac OS X host])
    ;;
  *linux* )
    ggl_host_type=linux
    AC_DEFINE(GGL_HOST_LINUX,1,[Linux host])
    ;;
  *solaris* )
    ggl_host_type=solaris
    AC_DEFINE(GGL_HOST_SOLARIS,1,[Solaris host])
    ;;
  * )
    ggl_host_type=unknown
    ;;
esac

AM_CONDITIONAL([GGL_HOST_WINDOWS], [test x$ggl_host_type = xwindows])
AM_CONDITIONAL([GGL_HOST_MACOSX], [test x$ggl_host_type = xmacosx])
AM_CONDITIONAL([GGL_HOST_LINUX], [test x$ggl_host_type = xlinux])
AM_CONDITIONAL([GGL_HOST_SOLARIS], [test x$ggl_host_type = xsolaris])

#Check memory check command
if test x$memcheck_command != x; then
  AC_CHECK_PROGS([MEMCHECK_COMMAND], [$memcheck_command])
  AC_SUBST(MEMCHECK_COMMAND)
fi

###########################################################
##                      Default dirs.                    ##
###########################################################
GGL_MODULE_DIR=$libdir/$GGL_BASE_DIR/modules
GGL_INCLUDE_DIR=$includedir/$GGL_BASE_DIR
GGL_SYSDEPS_INCLUDE_DIR=$libdir/$GGL_BASE_DIR/include
GGL_LIBEXEC_DIR=$libdir/$GGL_BASE_DIR
GGL_RESOURCE_DIR=$datadir/$GGL_BASE_DIR

AC_SUBST(GGL_MODULE_DIR)
AC_SUBST(GGL_INCLUDE_DIR)
AC_SUBST(GGL_SYSDEPS_INCLUDE_DIR)
AC_SUBST(GGL_LIBEXEC_DIR)
AC_SUBST(GGL_RESOURCE_DIR)

###########################################################
##                 Default compiler options.             ##
###########################################################
if test x$enable_debug = xyes; then
  AC_DEFINE(GGL_ENABLE_DEBUG,1,[Enable debug])
  DEFAULT_COMPILE_FLAGS="-g -O0 -Werror -Wall"
  DEBUG_MACROS="_DEBUG";
else
  DEFAULT_COMPILE_FLAGS="-O2 -Werror -Wall"
  DEBUG_MACROS="NDEBUG";
fi

# Enable stdc const macros by default, like INT64_C etc.
PREDEFINED_MACROS="-D__STDC_CONSTANT_MACROS \
		   -D$DEBUG_MACROS \
		   -DGGL_MODULE_DIR=\\\"$GGL_MODULE_DIR\\\" \
		   -DGGL_INCLUDE_DIR=\\\"$GGL_INCLUDE_DIR\\\" \
		   -DGGL_SYSDEPS_INCLUDE_DIR=\\\"$GGL_SYSDEPS_INCLUDE_DIR\\\" \
		   -DGGL_LIBEXEC_DIR=\\\"$GGL_LIBEXEC_DIR\\\" \
		   -DGGL_RESOURCE_DIR=\\\"$GGL_RESOURCE_DIR\\\""

AC_SUBST(PREDEFINED_MACROS)
AC_SUBST(DEFAULT_COMPILE_FLAGS)

# Clear the default CFLAGS and CXXFLAGS
CFLAGS=$ggl_save_CFLAGS
CXXFLAGS=$ggl_save_CXXFLAGS

###########################################################
##                      Output files.                    ##
###########################################################
AC_SUBST(ac_aux_dir)

# Workaround a bug in autoconf 2.59, which doesn't export these variables.
AC_SUBST(abs_srcdir)
AC_SUBST(abs_builddir)
AC_SUBST(abs_top_srcdir)
AC_SUBST(abs_top_builddir)

# Specifies configurable headers.
AC_CONFIG_HEADERS([config.h])

# Specifies configurable files.
AC_CONFIG_FILES([Makefile
		 ggadget/sysdeps.h
		 ggadget/Makefile
		 ggadget/tests/Makefile
		 ggadget/tests/test_modules/Makefile
		 ggadget/dbus/Makefile
		 ggadget/dbus/tests/Makefile
		 ggadget/gtk/Makefile
		 ggadget/gtk/tests/Makefile
		 ggadget/linux/Makefile
		 ggadget/linux/tests/Makefile
		 ggadget/gtkmoz/Makefile
		 ggadget/smjs/Makefile
		 ggadget/smjs/tests/Makefile
		 hosts/Makefile
		 hosts/simple/Makefile
		 resources/Makefile
		 third_party/Makefile
		 third_party/unzip/Makefile
		 unittest/Makefile
		 ggadget/libggadget-1.0.pc
		 ggadget/dbus/libggadget-dbus-1.0.pc
		 ggadget/gtk/libggadget-gtk-1.0.pc
		 ggadget/linux/libggadget-linux-1.0.pc
		 ggadget/smjs/libggadget-smjs-1.0.pc
		 ])
AC_OUTPUT

AC_MSG_RESULT([
Build options:
  Version                  $GGL_VERSION
  Install prefix           $prefix
  Install included libltdl $enable_ltdl_install
  Build shared libs        $enable_shared
  Build static libs        $enable_static
  Enable debug             $enable_debug
  Host type                $ggl_host_type
  SVG Support              $has_rsvg
  Build libggadget-gtk     $build_libggadget_gtk
  Build libggadget-smjs    $build_libggadget_smjs
  Build libggadget-dbus    $build_libggadget_dbus
  Build libggadget-gtkmoz  $build_libggadget_gtkmoz
  Build simple host        $build_simple_host
])

