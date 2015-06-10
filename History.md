This page is dedicated for recording notable changes of each release.

# History #
  * 0.11.2 - Jan 27th 2009
    * DBus proxy: Introspect remote object asynchronously.
    * Rss gadget: Improved compatibility with some websites.
    * Edit element: Added vAlign property (Only implemented in Gtk version).
    * Base element: Added focus overlay and tab stop feature.
    * Gtk host: Added a menu item to add an iGoogle gadget directly.
    * Fixed compatibility issue with gtk+-2.18.0 or above.
    * Fixed compatibility issue with xulrunner 1.9.2.
    * Fixed some compile warnings when compiling with gcc 4.4.x
    * Fixed some crash issues.
    * Fixed some issues in soup-xml-http-request for handling zero length POST.
    * Lots of other bugfixes.

  * 0.11.1 - Sep 9th 2009
    * Many reported issues have been fixed, including: 311, 313, 314, 315, 316, 320, 321, 323, 324, 326, 327, 328, 329, 331, 332, 333, etc.

  * 0.11.0 - Jun 1st 2009
    * Many reported issues have been fixed, including: 132, 151, 190, 244, 265, 276, 280, 284, 285,  286, 294, 297, 300, 306, etc.
    * Performance has been improved dramatically.
    * Memory consumption has been reduced notably.
    * A new script runtime based on WebKit/JavaScriptCore has been added.
    * A new browser element based on WebKit/Gtk has been added.
    * A new xml http request extension based on libsoup has been added.
    * A new flash element based on browser element has been added (mainly used with webkit browser element).
    * French translation provided by Pierre Slamich has been added.
    * Lots of other bugfixes.

  * 0.10.5 - Jan 7th 2009
    * Following reported issues have been fixed: 257, 258, 260, 261, 262, 263
    * Some nasty bugs in libggadget, libggadget-gtk and qt host have been fixed.
    * An about dialog has been added for both gtk and qt hosts.
    * Gadgets' about dialog has been reimplemented with platform independent code.
    * A Doxyfile has been included in source package, which can be used for generating API reference manual with doxygen command. graphviz is needed.
    * API reference manual has been improved dramatically.
    * Fully static build has been supported (only with configure/make).

  * 0.10.4 - Dec 16th 2008
    * Following reported issues have been fixed: 242, 246, 245, 253, 239, 249, 250, 243, 234, 254, 255
    * Many optimizations in performance and memory consumption.
    * Support for NPAPI plugins (like Flash) has been improved a lot.
    * gtkmoz\_browser\_element has been improved a lot.
    * gtk host now supports running a gadget in standalone mode.
    * Gadget Designer has been improved a lot. And it now runs in standalone mode, just like an ordinary application.
    * gtk sidebar host has been improved a lot.
    * Many bugs related to qt host have been fixed.
    * Many other bugfixes.

  * 0.10.3 - Nov 7th 2008
    * Following reported issues have been fixed: 215, 214, 201, 220, 216, 194, 229, 227, 224, 206, 204, 192, 193, 125, 164, 228, 225, 199, 235, 237, 226, 231, 238, 230, 240, 241
    * Most new features introduced in Google Desktop Windows 5.8 have been supported, except edit.detectUrls and personalization API. Please refer to: http://code.google.com/apis/desktop/docs/releasenotes-v58.html
    * View decorator related classes have been refactored to support KDE Plasma.
    * The binary dependency problem of xulrunner and libmozjs has been fixed by introducing a glue layer (extensions/smjs\_script\_runtime/libmozjs\_glue.{h,cc}).
    * analytics\_usage\_collector module has been added to collect anonymous usage data.
    * cmake build scripts have been included in source package, though it's still not widely tested and may have bugs.
    * Many improvements have been made for qt host.
    * Many other small fixes.

  * 0.10.2 - Sept 13th 2008
    * Lots of bugs have been fixed, including: 138, 146, 191, 203, 209, 211, 172, 200, 207, 208, 187, 202, and more.
    * Desktop Gadgets compatibility has been improved a lot. Many gadgets for GDWin can run on GGL now.
    * framework.system.getFileIcon() has been implemented.
    * framework.openUrl() now supports opening local file and launch desktop file.
    * Photos gadget can handle multiple level directory now.
    * Ability to adjust default font size has been added to gtk host.
    * Many code cleanup, refactory and optimization.

  * 0.10.1 - Aug 8th 2008
    * To celebrate the opening of the Beijing 2008 Olympic Games.
    * A prototype of Gadget Designer gadget has been added. Just add it from add gadget dialog and drag it out of sidebar (it's too large to fit in sidebar). Currently it only supports editing existing gadget. It's also a demo to show the power of Google Desktop Gadgets API, because itself is a gadget.
    * A new Photos gadget has been added, which can display your favorite photos from web or local directory as a slideshow.
    * Lots of bugs have been fixed, and some popular gadgets can run without problem, such as facebook and Google Calendar gadgets.

  * 0.10.0 - July 12th 2008
    * Memory consumption and performance have been improved by introducing class based property registration.
    * Several built-in gadgets have been added, such as Web Clip, Analog Clock, etc.
    * Most issues of SideBar have been resolved.
    * SideBar now supports auto hide mode.
    * ggl-gtk now supports hotkey.
    * Compatibility issues with xulrunner 1.9 have been resolved.
    * Desktop entries for ggl-gtk and ggl-qt have been added.
    * Debug Console has been added for both ggl-gtk and ggl-qt. Use -dc command option to enable it.
    * qt\_script\_runtime has been added, though there are still some bugs.
    * Many other bugfixes.

  * 0.9.3 -- Jun 13th 2008
    * Fixed many issues, such as: 96, 122, 128, 133, 135, 137, 139, 142, 143, 145, 149, 152, 156, 157
    * And now gadgets will show in all virtual desktops.

  * 0.9.2 -- Jun 6th 2008
    * Fixed most compilation issues on modern Linux distributions.
    * Fixed compatibility issues with xulrunner 1.9, though still has some bugs.
    * Fixed compatibility issues with curl, which doesn't have openssl support.
    * Fixed many issues in sidebar mode of gtk host.
    * Fixed some issues in qt host.
    * Added Simplified Chinese translation of most messages.
    * Various other bug fixes.
    * Made sidebar mode the default in gtk host. Use `-ns` switch to disable sidebar mode.

  * 0.9.1 -- May 30th 2008
    * Fixed a serious bug in build script, which may cause the project unable to build.

  * 0.9.0 -- May 30th 2008
    * The first public release.