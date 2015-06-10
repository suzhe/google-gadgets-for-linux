

# Introduction #

This page describe the method of building Google Gadgets for Linux from source code. Following instruction apply to Google Gadgets for Linux 0.10.4.

# Preparation #

Before building this project, you need do some preparation work, for example, downloading project source code, installing dependent packages and prepare build system.

## Download source code ##

You can get the source code from the svn repository. Please follow the instruction on [Source](http://code.google.com/p/google-gadgets-for-linux/source) page to check it out.

For end users, the best way is to download a release source package from our [download](http://code.google.com/p/google-gadgets-for-linux/downloads/list) page.


## Dependent packages ##

This project depends on various external libraries and applications, they must be available in your system before building this project. For different Linux distributions and operating systems, the packages name and installation method might be different. Following are information for the most popular distributions. If you know what packages are required for other distributions and operating systems, feel free to tell us and we will update this page to include them.

### Ubuntu 8.04 and 8.10 series ###

If you are using Ubuntu 8.04 or 8.10 series, including all of their variants, such Kubuntu etc., you need install following mandatory packages:
  * build-essential
  * zip
  * flex
  * desktop-file-utils
  * shared-mime-info
  * zlib1g-dev
  * libxml2-dev
  * libdbus-1-dev
  * libx11-dev
  * libxt-dev
  * libltdl3-dev (for Ubuntu 8.04) or libltdl7-dev (for Ubuntu 8.10)
  * libgstreamer-plugins-base0.10-dev
  * network-manager-dev
  * libstartup-notification0-dev
  * xulrunner-1.9-dev (Mandatory for GTK version, optional for Qt version)

If you want to use GUI and sidebar based on GTK library, you need following packages:
  * libgtk2.0-dev
  * librsvg2-dev
  * libcurl4-gnutls-dev or libcurl4-openssl-dev

If you want to use GUI based on QT library, you need following packages:
  * libqt4-dev

If you want to build source code checked out from svn trunk, you also need following packages:
  * autoconf
  * automake
  * libtool

Please note that, QT based GUI is not fully functional on Ubuntu 8.04, because the version of QT library is too low.

You can use apt-get command to install these packages, for example:
```
$ sudo apt-get install build-essential zip flex desktop-file-utils shared-mime-info zlib1g-dev libgtk2.0-dev libxml2-dev libdbus-1-dev librsvg2-dev libltdl3-dev libcurl4-gnutls-dev libgstreamer-plugins-base0.10-dev xulrunner-1.9-dev network-manager-dev libqt4-dev libstartup-notification0-dev
```

You can just copy above command in to a terminal and execute it.

### openSUSE 10.3 and 11.0 ###

If you are using openSUSE 10.3 and later versions, you need install following mandatory packages:
  * gcc-c++
  * zip
  * flex
  * desktop-file-utils
  * shared-mime-info
  * libtool
  * zlib-devel
  * libxml2-devel
  * dbus-1-devel
  * xorg-x11-devel
  * xorg-x11-libXt-devel
  * startup-notification-devel
  * NetworkManager-devel
  * gstreamer-0\_10-plugins-base-devel (for openSUSE 11.0 or above) or gstreamer010-plugins-base-devel (for openSUSE 10.3)
  * mozilla-xulrunner190-devel (for openSUSE 11.0 or above) or mozilla-xulrunner181-devel (for openSUSE 10.3) (Mandatory for GTK version, optional for Qt version)

If you want to use GUI and sidebar based on GTK library, you need following packages:
  * gtk2-devel
  * librsvg-devel
  * libcurl-devel

If you want to use GUI based on QT library, you need following packages:
  * libqt4-devel
  * libQtWebKit-devel (only available since openSUSE 11.0)

If you want to build source code checked out from svn trunk, you also need following packages:
  * autoconf
  * automake

Please note that, QT based GUI is not fully functional on openSUSE 10.3, because the version of QT library is too low.

You can use yast or zypper to install these packages.

### Fedora 8, 9 and 10 ###

If you are using Fedora 8 and later versions, you need install following mandatory packages:
  * gcc-c++
  * zip
  * flex
  * desktop-file-utils
  * shared-mime-info
  * libtool-ltdl-devel
  * zlib-devel
  * libxml2-devel
  * dbus-devel
  * libX11-devel
  * libXt-devel
  * startup-notification-devel
  * NetworkManager-devel
  * gstreamer-plugins-base-devel
  * gstreamer-devel
  * xulrunner-devel and xulrunner-devel-unstable (for Fedora 9 or above), or firefox-devel (for Fedora 8) (Mandatory for GTK version, optional for Qt version)

If you want to use GUI and sidebar based on GTK library, you need following packages:
  * gtk2-devel
  * librsvg2-devel
  * libcurl-devel (for Fedora 9 or above) or curl-devel (for Fedora 8)

If you want to use GUI based on QT library, you need following packages:
  * qt-devel (for Fedora 9 or above) or qt4-devel (for Fedora 8)

If you want to build source code checked out from svn trunk, you also need following packages:
  * autoconf
  * automake
  * libtool

You can use yum command to install these packages.

### Mandriva 2009 ###

If you are using Mandriva 2009, you need install following mandatory packages:
  * gcc-c++
  * zip
  * flex
  * desktop-file-utils
  * shared-mime-info
  * libltdl3-devel
  * zlib1-devel
  * libxml2-devel
  * libdbus-1-devel
  * libnm\_util-devel
  * libstartup-notification-1-devel
  * libgstreamer0.10-devel
  * libgstreamer-plugins-base0.10-devel
  * libxulrunner-devel (Mandatory for GTK version, optional for Qt version)

If you want to use GUI and sidebar based on GTK library, you need following packages:
  * librsvg2-devel
  * libcurl-devel
  * libcairo-devel
  * libgtk+2.0\_0-devel

If you want to use GUI based on QT library, you need following packages:
  * libqt4-devel

If you want to build source code checked out from svn trunk, you also need following packages:
  * autoconf
  * automake
  * libtool

You can use urpmi to install these packages, for example:
```
#urpmi gcc-c++ zip flex desktop-file-utils shared-mime-info libltdl3-devel zlib1-devel libxml2-devel libdbus-1-devel libnm_util-devel libstartup-notification-1-devel libgstreamer0.10-devel libgstreamer-plugins-base0.10-devel libxulrunner-devel librsvg2-devel libcurl-devel libcairo-devel libgtk+2.0_0-devel libqt4-devel autoconf automake libtool
```

## Prepare build system ##

There are two different build systems can be used to build this project, autoconf/automake and cmake. Though cmake build system has been included in official source package since 0.10.3, it's still in experimental stage. So autoconf/automake is still the best choice for most users.

If you want to try out cmake build system, you need install cmake version 2.4 or above first. Most distributions ship cmake package nowadays.

If you want to build source code checked out from svn trunk, you need run autotools/bootstrap.sh script before starting build. For example:

```
$ svn checkout http://google-gadgets-for-linux.googlecode.com/svn/trunk/ ggl-trunk
$ cd ggl-trunk
$ sh autotools/bootstrap.sh
```

# Build #

autoconf/automake based build system is highly recommended for normal users. cmake build system usually uses less time to build this project, but may still contain some problems.

Both build systems support out-of-tree build, that is, the build task can be done in a separated directory instead of inside the source code directory. Then you can remove all temporary files generated by build system easily without touching the source files. This approach is highly recommended.

## Build with autoconf/automake ##

### Invoke configure script ###

You need run configure script to generate makefile before building. For example, you can use following commands to perform an out-of-tree build:

**Prepare source code:**
```
$ tar jxf google-gadgets-for-linux-0.10.3.tar.bz2
$ cd google-gadgets-for-linux-0.10.3
```

Or, if you want to use he bleeding-edge version:
```
$ svn checkout http://google-gadgets-for-linux.googlecode.com/svn/trunk/ ggl-trunk
$ cd ggl-trunk
$ sh autotools/bootstrap.sh
```

**Run configure script out-of-tree:**
```
$ mkdir build
$ cd build
$ ../configure --prefix=/usr
```

It might take one or two minutes to run configure script, if anything goes well, a configure summary will be displayed, such as:
```
Build options:
  Version                       "0.10.3"
  Install prefix                /usr
  Install included libltdl      no
  Build shared libs             yes
  Build static libs             yes
  Enable debug                  no
  Host type                     linux
  OEM brand                     

 Libraries:
  GTK SVG Support               yes
  Build libggadget-gtk          yes
  Build libggadget-qt           yes
  Build libggadget-dbus         yes
  Build libggadget-npapi        yes

 Extensions:
  Build dbus-script-class       yes
  Build gtk-edit-element        yes
  Build gtkmoz-browser-element  yes
  Build qtwebkit-browser        no
  Build gst-audio-framework     yes
  Build gst-video-element       yes
  Build gtk-system-framework    yes
  Build gtk-flash-element       yes
  Build qt-system-framework     yes
  Build linux-system-framework  yes
  Build smjs-script-runtime     yes
  Build qt-script-runtime       no
  Build curl-xml-http-request   yes
  Build qt-xml-http-request     yes
  Build libxml2-xml-parser      yes

 Hosts:
  Build gtk host                yes
  Build qt host                 yes
```

The result might be different on different system. If some dependency libraries are missing, corresponding build option might be set to "no". For example, above result shows that my QT library may be too old to support qtwebkit-browser and qt-script-runtime extensions.

Among above results, The most important are "Build gtk host" and "Build qt host", if non of these options are "yes", then means some mandatory dependent packages might be missing. You need at least one host to be built.

The "--prefix" parameter of configure script controls the target directory where you want to install the project. Usually "/usr" is the best choice for normal users. However, default value is "/usr/local", and on some systems, it might not work properly. So "--prefix=/usr" is highly recommended for all users.

Besides "--prefix" parameter, there are some other parameters you might want to use:
  * --enable-debug
> It enables debug build, which will generate slower and larger binary but suitable for debugging. This option is not recommended for normal users.

  * --disable-werror
> Disable -Werror compile option, which is enabled by default and may cause problem on some platforms, eg. Fedora 11.

  * --with-browser-plugins-dir
> Since 0.10.3, Google Gadgets for Linux support [new APIs](http://code.google.com/apis/desktop/docs/releasenotes-v58.html) introduced in Google Desktop for Windows 5.8. The most important one is the "Full support for Flash", that is, a desktop gadget can play a .swf file from the internet directly. On Linux, it requires adobe flash browser plugin.
> This option specifies the directory where the flash plugin can be found. The directory might be different on different Linux distributions, for example:
    * On Ubuntu/Debian: /usr/lib/xulrunner-addons/plugins
    * On openSUSE: /usr/lib/browser-plugins  (32bit system) or /usr/lib64/browser-plugins (64bit system)
    * On Fedora: /usr/lib/mozilla/plugins  (32bit system) or /usr/lib64/mozilla/plugins (64bit system)

  * --libdir
> Specifies the directory to install shared libraries. The default value is "${prefix}/lib". On 32bit Linux systems, this option can usually be omitted. But on 64bit Linux systems, the value are different on different systems. On Ubuntu/Debian, it has no difference between 32bit and 64bit systems. On openSUSE and Fedora, it shall be specified explicitly to ${prefix}/lib64, where ${prefix} is the directory specified by --prefix parameter. For example, if you are using openSUSE 11.0 64bit, then you need use following configure command:
```
$ ../configure --prefix=/usr --libdir=/usr/lib64 --with-browser-plugins-dir=/usr/lib64/browser-plugins
```

  * --help
> To display a complete lists of parameters that you can use.

### Build ###

After executing configure script, you can just invoke command `make` in the same directory to build the project. There are multiple CPUs or a CPU with multi cores in your machine, you can use command `make -jN` to make the build process faster, where N is the number of CPUs you have. For example, on a dual-core machine, you can use following command:
```
$ make -j2
```

### Install ###

If the build finished without any problem, then you can use command `make install` to install the project. You need root privilege to run this command. For eample, on Ubuntu, you can:
```
$ sudo make install
```

On some systems like Fedora, `sudo` might not work, and you may need use command `su` to switch to root first.

Then can just run command `ggl-gtk` or `ggl-qt` in a terminal to launch Google Gadgets for Linux. You can also find and launch Google Gadgets in application menu's Accessories or Internet submenu.

If you want to make binary package, you can use following command to install the target files into a specific directory (eg. /tmp/ggl-root):
```
$ make DESTDIR=/tmp/ggl-root install
```

Then you can pack all things in /tmp/ggl-root into binary package.

If you encounter any problem when building, try to use --disable-werror option when execute configure script. There is a known issue which will cause build problem on Fedora 11, if this option is not specified.

## Build with cmake ##

cmake build is only available publicly since 0.10.3. You need cmake 2.4 or above to build it. Similar than autoconf/automake build system, you need three steps to build the project:

### Configure ###

For example, on Ubuntu/Debian systems, to do an out-of-tree build, you need:
```
$ mkdir build
$ cd build
$ cmake -DCMAKE_INSTALL_PREFIX=/usr -DGGL_DEFAULT_BROWSER_PLUGINS_DIR=/usr/lib/xulrunner-addons/plugins ../
```

A summary will be displayed if anything goes well:
```
Build options:
  Version                          "0.10.3"
  Build type                       Debug
  OEM brand                        

 Libraries:
  GTK SVG Support                  1
  Build libggadget-gtk             1
  Build libggadget-qt              1
  Build libggadget-dbus            1

 Extensions:
  Build dbus-script-class          1
  Build gtkmoz-browser-element     1
  Build gst-audio-framework        1
  Build gst-video-element          1
  Build linux-system-framework     1
  Build smjs-script-runtime        1
  Build curl-xml-http-request      1
  Build libxml2-xml-parser         1
  Build gtk-edit-element           1
  Build gtk-system-framework       1
  Build qt-edit-element            1
  Build qtwebkit-browser-element   0
  Build qt-xml-http-request        1
  Build qt-script-runtime          0
  Build qt-system-framework        1

 Hosts:
  Build gtk host                   1
  Build qt host                    1

-- Configuring done
-- Generating done
```

After executing cmake, you can use command `ccmake` to check and change configure parameters:
```
$ ccmake ../
```

### Build ###

Same as autoconf/automake build system, you can use `make` command to build the project. Use `make -j2` on a dual-core system can make the progress faster.

### Install ###

Same as autoconf/automake build system, you can use `make install` command to install the project into your system. You also need root privilege to do it.

You can also use `make DESTDIR=/tmp/ggl-root install` to install all things into a specific directory (eg. /tmp/ggl-root).