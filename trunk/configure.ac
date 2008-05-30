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
	[0.9.0],
	[google-gadgets-for-linux-dev@googlegroups.com])

# Requires autoconf version >= 2.50
AC_PREREQ([2.50])

# Specifies a source file in the source directory.
AC_CONFIG_SRCDIR([ggadget/common.h])

# Tells autoconf where to find necessary build scripts and macros.
AC_CONFIG_AUX_DIR([autotools])
AC_CONFIG_MACRO_DIR([autotools])


# Save the user defined CFLAGS and CXXFLAGS, then restore them later.
# So that the value set by AC_PROG_CC and AC_PROG_CXX won't be used,
# even if they are empty.
ggl_save_CFLAGS=$CFLAGS
ggl_save_CXXFLAGS=$CXXFLAGS
ggl_save_LDFLAGS=$LDFLAGS

###########################################################
##          Some definitions of Version macros.          ##
###########################################################

# Version numbers of Google Gadgets for Linux package.
GGL_MAJOR_VERSION=0
GGL_MINOR_VERSION=9
GGL_MICRO_VERSION=0

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

# This string is used in auto update request. It should be updated to the
# time of a release build is made. Its format is yymmdd-HHMMSS.
GGL_VERSION_TIMESTAMP=\"080530-000000\"
AC_SUBST(GGL_VERSION_TIMESTAMP)

# Define the version of Google Desktop Gadget API that this release supports.
GGL_API_MAJOR_VERSION=5
GGL_API_MINOR_VERSION=7
GGL_API_VERSION=\"$GGL_API_MAJOR_VERSION.$GGL_API_MINOR_VERSION.0.0\"
AC_SUBST(GGL_API_MAJOR_VERSION)
AC_SUBST(GGL_API_MINOR_VERSION)
AC_SUBST(GGL_API_VERSION)

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

GGL_BASE_DIR=google-gadgets
AC_SUBST(GGL_BASE_DIR)

# Check build and host system types
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

###########################################################
##                      Config options.                  ##
###########################################################
build_gtk_host=yes
build_qt_host=yes
build_libggadget_dbus=yes
build_libggadget_gtk=yes
build_libggadget_qt=yes
build_gtkmoz_browser_element=yes
build_qtwebkit_browser_element=yes
build_gst_audio_framework=yes
build_gst_mediaplayer_element=yes
build_qt_system_framework=yes
build_gtk_system_framework=yes
build_smjs_script_runtime=yes
build_curl_xml_http_request=yes
build_qt_xml_http_request=yes
build_libxml2_xml_parser=yes
# OS Specific conditions.
build_linux_system_framework=no

AC_ARG_ENABLE([debug],
	      AS_HELP_STRING([--enable-debug],[enable debug]),
	      [enable_debug=$enableval],
	      [enable_debug=no])

AC_ARG_WITH([memcheck-command],
	    AS_HELP_STRING([--with-memcheck-command=CMD],
		[specify a memory check command for running unit tests.]),
	    [memcheck_command=$withval],
	    [memcheck_command=""])

AC_ARG_WITH([gst-audio-sink],
	    AS_HELP_STRING([--with-gst-audio-sink=audiosink],
		[Use a specified audiosink instead of autoaudiosink.]),
	    [default_gst_audio_sink=$withval],
	    [default_gst_audio_sink="autoaudiosink"])

AC_ARG_ENABLE([gtk-host],
	      AS_HELP_STRING([--disable-gtk-host],
			     [don't build gtk host]),
	      [build_gtk_host=$enableval],
	      [build_gtk_host=yes])

AC_ARG_ENABLE([qt-host],
	      AS_HELP_STRING([--disable-qt-host],
			     [don't build qt host]),
	      [build_qt_host=$enableval],
	      [build_qt_host=yes])

AC_ARG_ENABLE([libggadget-dbus],
	      AS_HELP_STRING([--disable-libggadget-dbus],
			     [don't build libggadget-dbus]),
	      [build_libggadget_dbus=$enableval],
	      [build_libggadget_dbus=yes])

AC_ARG_ENABLE([libggadget-gtk],
	      AS_HELP_STRING([--disable-libggadget-gtk],
			     [don't build libggadget-gtk]),
	      [build_libggadget_gtk=$enableval],
	      [build_libggadget_gtk=yes])

AC_ARG_ENABLE([libggadget-qt],
	      AS_HELP_STRING([--disable-libggadget-qt],
			     [don't build libggadget-qt]),
	      [build_libggadget_qt=$enableval],
	      [build_libggadget_qt=yes])

AC_ARG_ENABLE([gtkmoz-browser-element],
	      AS_HELP_STRING([--disable-gtkmoz-browser-element],
			     [don't build gtkmoz-browser-element]),
	      [build_gtkmoz_browser_element=$enableval],
	      [build_gtkmoz_browser_element=yes])

AC_ARG_ENABLE([gst-audio-framework],
	      AS_HELP_STRING([--disable-gst-audio-framework],
			     [don't build gst-audio-framework]),
	      [build_gst_audio_framework=$enableval],
	      [build_gst_audio_framework=yes])

AC_ARG_ENABLE([gst-mediaplayer-element],
	      AS_HELP_STRING([--disable-gst-mediaplayer-element],
			     [don't build gst-mediaplayer-element]),
	      [build_gst_mediaplayer_element=$enableval],
	      [build_gst_mediaplayer_element=yes])

AC_ARG_ENABLE([qt-system-framework],
	      AS_HELP_STRING([--disable-qt-system-framework],
			     [don't build qt-system-framework]),
	      [build_qt_system_framework=$enableval],
	      [build_qt_system_framework=yes])

AC_ARG_ENABLE([qtwebkit-browser-element],
	      AS_HELP_STRING([--disable-qtwebkit-browser-element],
			     [don't build qtwebkit-browser-element]),
	      [build_qtwebkit_browser_element=$enableval],
	      [build_qtwebkit_browser_element=yes])

AC_ARG_ENABLE([gtk-system-framework],
	      AS_HELP_STRING([--disable-gtk-system-framework],
			     [don't build gtk-system-framework]),
	      [build_gtk_system_framework=$enableval],
	      [build_gtk_system_framework=yes])

AC_ARG_ENABLE([smjs-script-runtime],
	      AS_HELP_STRING([--disable-smjs-script-runtime],
			     [don't build smjs-script-runtime]),
	      [build_smjs_script_runtime=$enableval],
	      [build_smjs_script_runtime=yes])

AC_ARG_ENABLE([curl-xml-http-request],
	      AS_HELP_STRING([--disable-curl-xml-http-request],
			     [don't build curl-xml-http-request]),
	      [build_curl_xml_http_request=$enableval],
	      [build_curl_xml_http_request=yes])

AC_ARG_ENABLE([qt-xml-http-request],
	      AS_HELP_STRING([--disable-qt-xml-http-request],
			     [don't build qt-xml-http-request]),
	      [build_qt_xml_http_request=$enableval],
	      [build_qt_xml_http_request=yes])

AC_ARG_ENABLE([libxml2-xml-parser],
	      AS_HELP_STRING([--disable-libxml2-xml-parser],
			     [don't build libxml2-xml-parser]),
	      [build_libxml2_xml_parser=$enableval],
	      [build_libxml2_xml_parser=yes])

if test x$build_libggadget_gtk != xyes; then
  build_gtk_host=no
fi

if test x$build_libggadget_qt != xyes; then
  build_qt_host=no
  build_qtwebkit_browser_element=no
  build_qt_xml_http_request=no
  build_qt_system_framework=no
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
  DEFAULT_COMPILE_FLAGS="-g -O0 -Werror -Wall -Wconversion"
  DEBUG_MACROS="_DEBUG";
else
  DEFAULT_COMPILE_FLAGS="-O2 -Werror -Wall -Wconversion"
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

case $host_os in
  *interix* | *mks* | *winnt* | *cygwin* | *mingw* )
    ggl_host_type=windows
    PREDEFINED_MACROS="$PREDEFINED_MACROS -DGGL_HOST_WINDOWS=1"
    GGL_PLATFORM_SHORT=\"win\"
    GGL_PLATFORM=\"windows\"
    ;;
  *darwin* )
    ggl_host_type=macosx
    PREDEFINED_MACROS="$PREDEFINED_MACROS -DGGL_HOST_MACOSX=1"
    GGL_PLATFORM_SHORT=\"mac\"
    GGL_PLATFORM=\"mac\"
    ;;
  *linux* )
    ggl_host_type=linux
    PREDEFINED_MACROS="$PREDEFINED_MACROS -DGGL_HOST_LINUX=1"
    build_linux_system_framework=yes
    GGL_PLATFORM_SHORT=\"linux\"
    GGL_PLATFORM=\"linux\"
    ;;
  *solaris* )
    ggl_host_type=solaris
    PREDEFINED_MACROS="$PREDEFINED_MACROS -DGGL_HOST_SOLARIS=1"
    ;;
  * )
    ggl_host_type=unknown
    ;;
esac

AC_SUBST(GGL_PLATFORM_SHORT)
AC_SUBST(GGL_PLATFORM)

AM_CONDITIONAL([GGL_HOST_WINDOWS], [test x$ggl_host_type = xwindows])
AM_CONDITIONAL([GGL_HOST_MACOSX], [test x$ggl_host_type = xmacosx])
AM_CONDITIONAL([GGL_HOST_LINUX], [test x$ggl_host_type = xlinux])
AM_CONDITIONAL([GGL_HOST_SOLARIS], [test x$ggl_host_type = xsolaris])

AC_SUBST(PREDEFINED_MACROS)
AC_SUBST(DEFAULT_COMPILE_FLAGS)

###########################################################
##                  Start Configuration.                 ##
###########################################################

# Init automake stuff
AM_INIT_AUTOMAKE([tar-ustar])

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

# Check zlib
GGL_CHECK_ZLIB([1.2.3], [has_zlib=yes], [has_zlib=no])

# Check pthread lib
AC_CHECK_LIB([pthread], [pthread_create], [has_pthread=yes], [has_pthread=no])

# Check Xlib
AC_PATH_XTRA
if test "x$no_x" != "xyes"; then
  PREDEFINED_MACROS="$PREDEFINED_MACROS -DHAVE_X11=1"
  X_LIBS="$X_LIBS $X_PRE_LIBS -lX11"
  AC_SUBST(X_CFLAGS)
  AC_SUBST(X_LIBS)
  AC_SUBST(X_PRE_LIBS)
fi

# Check qt4
if test x$ggl_host_type = xlinux; then
  PKG_CHECK_MODULES(QT4, [QtGui >= 4.3, QtCore >= 4.3, QtOpenGL >= 4.3,
			  QtNetwork >= 4.3],
		    [has_qt4=yes
		     MOC=`$PKG_CONFIG --variable=moc_location QtCore`],
		     [has_qt4=no])
  PKG_CHECK_MODULES(QT4WEBKIT, [QtWebKit],
		    [has_qtwebkit=yes],
		    [has_qtwebkit=no])
elif test x$ggl_host_type = xmacosx; then
  # There is no pkgconfig for Qt on MacOSX, just do header check.
  AC_CHECK_HEADERS([QtCore/QtCore QtGui/QtGui QtOpenGL/QtOpenGL \
		    QtNetwork/QtNetwork],
		   [has_qt4=yes], [has_qt4=no])
  AC_CHECK_HEADERS([QtWebKit/QtWebKit],
		   [has_qtwebkit=yes], [has_qtwebkit=no])
  if test x$has_qt4 = xyes; then
    QT4_LIBS="-framework OpenGL -framework AGL -framework QtGui \
              -framework Carbon -framework AppKit -framework QtCore \
	      -framework QtNetwork \
              -framework ApplicationServices -lz -lm"
    AC_PATH_PROG([MOC], [moc])
  fi
  if test x$has_qtwebkit = xyes; then
    QT4WEBKIT_LIBS="-$QT4_LIBS -framework QtWebKit"
  fi
else
  AC_MSG_WARN([Qt support is not available for $ggl_host_type])
fi
AC_SUBST(MOC)

# Check gtk2 related libraries.
PKG_CHECK_MODULES(GTK2,
		  [cairo >= 1.0.0
		   pango  >= 1.10.0
		   gtk+-2.0 >= 2.8.0
		   gdk-2.0 >= 2.8.0
		   gthread-2.0 >= 2.8.0],
		   [has_gtk2=yes], [has_gtk2=no])

# Check gstreamer-0.10 related libraries.
PKG_CHECK_MODULES(GSTREAMER,
		  [gstreamer-0.10 >= 0.10.0, gstreamer-plugins-base-0.10 >= 0.10.0],
		  [has_gstreamer_plugins_base=yes], [has_gstreamer_plugins_base=no])
if test x$has_gstreamer_plugins_base = xyes; then
  CFLAGS="$CFLAGS $GSTREAMER_CFLAGS"
  CXXFLAGS="$CFLAGS $GSTREAMER_CFLAGS"
  LDFLAGS="$LDFLAGS $GSTREAMER_LIBS -L/usr/lib/gstreamer-0.10 -L/usr/local/lib/gstreamer-0.10"
  AC_CHECK_HEADERS([gst/video/gstvideosink.h], [has_gstreamer_video=yes], [has_gstreamer_video=no])
  if test x$has_gstreamer_video = xyes; then
    GSTVIDEOSINK_LIBRARIES="-lgstvideo-0.10"
    AC_SUBST(GSTVIDEOSINK_LIBRARIES)
  fi
  CFLAGS=$ggl_save_CFLAGS
  CXXFLAGS=$ggl_save_CXXFLAGS
  LDFLAGS=$ggl_save_LDFLAGS
fi

# Check libcurl.
LIBCURL_CHECK_CONFIG([yes], [7.15.0], [has_libcurl=yes], [has_libcurl=no])

# Check openssl.
PKG_CHECK_MODULES(OPENSSL, [openssl >= 0.9.8a], [has_openssl=yes], [has_openssl=no])

# Check libxml2.
AM_PATH_XML2([2.4.0], [has_libxml2=yes], [has_libxml2=no])

# Check spidermonkey library
GGL_CHECK_SPIDERMONKEY([160], [has_spidermonkey=yes], [has_spidermonkey=no])

# Check D-Bus library
PKG_CHECK_MODULES(DBUS, dbus-1 >= 1.0.2, [has_dbus=yes], [has_dbus=no])

if test x$has_dbus = xyes; then
  CFLAGS="$CFLAGS $DBUS_CFLAGS"
  LDFLAGS="$LDFLAGS $DBUS_LIBS"
  AC_CHECK_FUNCS([dbus_watch_get_unix_fd dbus_watch_get_fd])
  CFLAGS=$ggl_save_CFLAGS
  LDFLAGS=$ggl_save_LDFLAGS
fi

# Check librsvg
PKG_CHECK_MODULES(RSVG, librsvg-2.0 >= 2.14.0, [has_rsvg=yes], [has_rsvg=no])

# Check gtkmozembed
PKG_CHECK_MODULES(GTKMOZEMBED, [xulrunner-gtkmozembed xulrunner-js],
		  [has_gtkmozembed=xulrunner-gtkmozembed],
  [PKG_CHECK_MODULES(GTKMOZEMBED, [firefox2-gtkmozembed firefox2-js],
		     [has_gtkmozembed=firefox2-gtkmozembed],
    [PKG_CHECK_MODULES(GTKMOZEMBED, [firefox-gtkmozembed firefox-js],
		       [has_gtkmozembed=firefox-gtkmozembed],
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

# Check necessary functions
AC_CHECK_FUNCS([mkdtemp tempnam])

###########################################################
##                  Process Config options.              ##
###########################################################

if test x$has_zlib != xyes; then
  AC_MSG_ERROR([Library zlib is not available.])
fi

if test x$has_libxml2 != xyes -a x$build_libxml2_xml_parser = xyes; then
  AC_MSG_WARN([Library xml2 is not available, libxml2-xml-parser extension won't be built.])
  build_libxml2_xml_parser=no
fi

if test x$has_libcurl != xyes -o x$has_openssl != xyes; then
  if test x$build_curl_xml_http_request = xyes; then
    AC_MSG_WARN([Library curl or openssl is not available, curl-xml-http-request extension won't be built.])
    build_curl_xml_http_request=no
  fi
fi

if test x$has_gstreamer_plugins_base != xyes -a x$build_gst_audio_framework = xyes; then
  AC_MSG_WARN([Library gstreamer_plugins_base(>=0.10.0) is not available, gst-audio-framework extension will not be built.])
  build_gst_audio_framework=no
fi

if test x$build_gst_mediaplayer_element = xyes; then
  build_gst_mediaplayer_element=no
  if test x$has_gstreamer_plugins_base = xyes; then
    if test x$has_gstreamer_video = xyes; then
      build_gst_mediaplayer_element=yes
    else
      AC_MSG_WARN([Libgstreamer-plugins-base-dev is not available, gst-mediaplayer-element extension will not be built.])
    fi
  else
    AC_MSG_WARN([Library gstreamer_plugins_base(>=0.10.0) is not available, gst-mediaplayer-element extension will not be built.])
  fi
fi

if test x$has_gtk2 != xyes; then
  AC_MSG_WARN([Library gtk-2.0 is not available, gtk-system-framework extension and gtk host won't be built.])
  build_gtk_host=no
  build_gtk_system_framework=no
  build_libggadget_gtk=no
fi

if test x$has_qt4 != xyes; then
  AC_MSG_WARN([Library qt-4.3 or above is not available, qt host and libggadget-qt won't be built])
  build_qt_host=no
  build_libggadget_qt=no
  build_qt_system_framework=no
  build_qt_xml_http_request=no
fi

if test x$has_qtwebkit != xyes; then
  AC_MSG_WARN([Library qtwebkit or qt-4.4 and above is needed to build qtwebkit_browser_element])
  build_qtwebkit_browser_element=no
fi

if test x$has_spidermonkey != xyes -a x$build_smjs_script_runtime = xyes; then
  AC_MSG_WARN([Library SpiderMonkey is not available, smjs-script-runtime extension won't be built.])
  build_smjs_script_runtime=no
fi

if test x$has_dbus != xyes -a x$build_libggadget_dbus = xyes; then
  AC_MSG_WARN([Library D-Bus is not available, libggadget-dbus won't be built.])
  build_libggadget_dbus=no
fi

if test x$has_gtkmozembed = xno -a x$build_gtkmoz_browser_element = xyes; then
  AC_MSG_WARN([Library GtkMozEmbed is not available, gtkmoz-browser-element extension won't be built.])
  build_gtkmoz_browser_element=no
else
  MOZILLA_FIVE_HOME=`$PKG_CONFIG --variable=libdir $has_gtkmozembed`
  PREDEFINED_MACROS="$PREDEFINED_MACROS \
		     -DMOZILLA_FIVE_HOME=\\\"$MOZILLA_FIVE_HOME\\\""
fi

AM_CONDITIONAL(
  [GGL_BUILD_LIBGGADGET_GTK],
  [test x$build_libggadget_gtk = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_LIBGGADGET_QT],
  [test x$build_libggadget_qt = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_QT_HOST],
  [test x$build_qt_host = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_LIBGGADGET_DBUS],
  [test x$build_libggadget_dbus = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_GTKMOZ_BROWSER_ELEMENT],
  [test x$build_gtkmoz_browser_element = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_QTWEBKIT_BROWSER_ELEMENT],
  [test x$build_qtwebkit_browser_element = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_GST_AUDIO_FRAMEWORK],
  [test x$build_gst_audio_framework = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_GST_MEDIAPLAYER_ELEMENT],
  [test x$build_gst_mediaplayer_element = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_GTK_SYSTEM_FRAMEWORK],
  [test x$build_gtk_system_framework = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_QT_SYSTEM_FRAMEWORK],
  [test x$build_qt_system_framework = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_QT_XML_HTTP_REQUEST],
  [test x$build_qt_xml_http_request = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_SMJS_SCRIPT_RUNTIME],
  [test x$build_smjs_script_runtime = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_CURL_XML_HTTP_REQUEST],
  [test x$build_curl_xml_http_request = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_LIBXML2_XML_PARSER],
  [test x$build_libxml2_xml_parser = xyes])

AM_CONDITIONAL(
  [GGL_BUILD_GTK_HOST],
  [test x$build_gtk_host = xyes])

AM_CONDITIONAL([GGL_HAVE_ZIP], [test x$ZIP != x])
AM_CONDITIONAL([GGL_HAVE_RSVG], [test x$has_rsvg = xyes])

if test x$ZIP = x; then
  AC_MSG_WARN([Can't generate resource file bundle without zip command.])
fi

#Check memory check command
if test x$memcheck_command != x; then
  AC_CHECK_PROGS([MEMCHECK_COMMAND], [$memcheck_command])
  AC_SUBST(MEMCHECK_COMMAND)
fi

#Gstreamer audiosink
if test x$default_gst_audio_sink != x; then
  AC_DEFINE_UNQUOTED([GGL_DEFAULT_GST_AUDIO_SINK], ["$default_gst_audio_sink"],
		     [Default Gstreamer audio sink.])
fi

#pthread lib
if test x$has_pthread = xyes; then
  PTHREAD_LIBS="-lpthread"
  AC_SUBST(PTHREAD_LIBS)
  PREDEFINED_MACROS="$PREDEFINED_MACROS -DHAVE_PTHREAD=1"
  AC_SUBST(PREDEFINED_MACROS)
fi

# Clear the default CFLAGS and CXXFLAGS
CFLAGS=$ggl_save_CFLAGS
CXXFLAGS=$ggl_save_CXXFLAGS
LDFLAGS=$ggl_save_LDFLAGS

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
		 extensions/Makefile
		 extensions/dbus_script_class/Makefile
		 extensions/default_framework/Makefile
		 extensions/gtk_edit_element/Makefile
		 extensions/qt_edit_element/Makefile
		 extensions/gtkmoz_browser_element/Makefile
		 extensions/qtwebkit_browser_element/Makefile
		 extensions/gst_audio_framework/Makefile
		 extensions/gst_mediaplayer_element/Makefile
		 extensions/gtk_system_framework/Makefile
		 extensions/qt_system_framework/Makefile
		 extensions/linux_system_framework/Makefile
		 extensions/linux_system_framework/tests/Makefile
		 extensions/smjs_script_runtime/Makefile
		 extensions/smjs_script_runtime/tests/Makefile
		 extensions/qt_xml_http_request/Makefile
		 extensions/curl_xml_http_request/Makefile
		 extensions/libxml2_xml_parser/Makefile
		 extensions/default_options/Makefile
		 extensions/default_options/tests/Makefile
		 extensions/google_gadget_manager/Makefile
		 extensions/google_gadget_manager/gadget_browser/Makefile
		 extensions/google_gadget_manager/tests/Makefile
		 ggadget/sysdeps.h
		 ggadget/Makefile
		 ggadget/tests/Makefile
		 ggadget/tests/test_modules/Makefile
		 ggadget/dbus/Makefile
		 ggadget/dbus/tests/Makefile
		 ggadget/qt/Makefile
		 ggadget/qt/tests/Makefile
		 ggadget/gtk/Makefile
		 ggadget/gtk/tests/Makefile
		 hosts/Makefile
		 hosts/gtk/Makefile
		 hosts/qt/Makefile
		 resources/Makefile
		 third_party/Makefile
		 third_party/codesighs/Makefile
		 third_party/editline/Makefile
		 third_party/unzip/Makefile
		 unittest/Makefile
		 rss_gadget/Makefile
		 igoogle_gadget/Makefile
		 ggadget/libggadget-1.0.pc
		 ggadget/dbus/libggadget-dbus-1.0.pc
		 ggadget/gtk/libggadget-gtk-1.0.pc
		 ggadget/qt/libggadget-qt-1.0.pc
		 ])
AC_OUTPUT

AC_MSG_RESULT([
Build options:
  Version                       $GGL_VERSION
  Install prefix                $prefix
  Install included libltdl      $enable_ltdl_install
  Build shared libs             $enable_shared
  Build static libs             $enable_static
  Enable debug                  $enable_debug
  Host type                     $ggl_host_type

 Libraries:
  GTK SVG Support               $has_rsvg
  Build libggadget-gtk          $build_libggadget_gtk
  Build libggadget-qt           $build_libggadget_qt
  Build libggadget-dbus         $build_libggadget_dbus

 Extensions:
  Build dbus-script-class       $build_libggadget_dbus
  Build gtk-edit-element        $build_libggadget_gtk
  Build gtkmoz-browser-element  $build_gtkmoz_browser_element
  Build qtwebkit-browser        $build_qtwebkit_browser_element
  Build gst-audio-framework     $build_gst_audio_framework
  Build gst-mediaplayer-element $build_gst_mediaplayer_element
  Build gtk-system-framework    $build_gtk_system_framework
  Build qt-system-framework     $build_qt_system_framework
  Build linux-system-framework  $build_linux_system_framework
  Build smjs-script-runtime     $build_smjs_script_runtime
  Build curl-xml-http-request   $build_curl_xml_http_request
  Build qt-xml-http-request     $build_qt_xml_http_request
  Build libxml2-xml-parser      $build_libxml2_xml_parser

 Hosts:
  Build gtk host                $build_gtk_host
  Build qt host                 $build_qt_host
])

