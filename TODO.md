# Introduction #

This page is dedicated to collect future plans and action items of this project. All items listed below are proposals. If we decide to implement any of them, a corresponding issue entry shall be created to track the development progress.

If you want to help us implement any of following items, please first follow [this instruction](BecomingAContributor.md) to become a contributor of this project, then send your plan to our [dev maillist](http://groups.google.com/group/google-gadgets-for-linux-dev). You may create an issue to track your progress. If you have any questions, feel free to ask us on dev maillist. We'd appreciate your contribution very much.

# Details #

The TODO List contains several different categories. Each item belongs to one specific category and has a priority value assigned. The priority value ranges from 1 to 5, the smaller the higher.

## Core code and Design ##

  * Reduce the usage of std::map and other complex containers.
  * Use std::string and static string literals as much as possible.
  * **(Almost DONE)** Reduce the size of each class, by:
    1. **(Almost DONE)** Re-order the data members;
    1. **(Almost DONE)** Use more effective data types, such as replacing bools with bit fields.
  * **(DONE)** Refactory Signal, Slot and ScriptableHelper to support class based property registration.
  * **(DONE)** Evaluate the effect of SmallObjectAllocator and use it to more classes if possible.
  * Investigate the binary size and memory usage of every classes and functions, and optimize them as much as possible.
  * Reduce unnecessary inline functions.
  * Performance optimization, especially the performance of graphics operation.

## New Features in Gadget API level ##

  * DOM2 and DOM3 support
  * URL detection in edit element
  * Support for [displayAPI](http://code.google.com/intl/zh-CN/apis/desktop/docs/script.html)

## New extensions ##

  * Option extension based on GConf
  * Option extension based on KConfig
  * **(DONE)** JavaScript extension based on QtScript
  * JavaScript extension based on KJS or [WebKit JavaScriptCore](http://webkit.org/projects/javascript/)
  * JavaScript extension based on [V8](http://code.google.com/p/v8/)
  * Browser extension based on [WebKit gtk/cairo port](http://live.gnome.org/WebKitGtk)
> > See the [initial implementation](http://code.google.com/p/google-gadgets-for-linux/issues/detail?id=132) written by jmalonzo.
  * Audio and Mediaplayer extensions based on KDE Phonon
  * xml\_http\_request extension based on [libsoup](http://live.gnome.org/LibSoup)

## New host related features ##

  * Sidebar host based on Qt
  * Customizable module loading
  * Support of global hotkeys
  * Better support of window manager integration
  * Support of system taskbar

## Porting / Platform integration ##

  * **(Almost DONE)** KDE Plasma integration
  * Support of OpenGL acceleration (both cairo and qt)
  * Port to other X-Window based and UNIX like operation systems, such as OpenSolaris and FreeBSD
  * Port to [Clutter](http://www.clutter-project.org/)
  * Port to [DirectFB](http://www.directfb.org/)

## Security related ##

  * **(DONE)** Support of fine-grained access control per gadget

## Packaging ##

  * **(DONE)** Provide metadata for deb and rpm packages.
  * **(DONE)** Provide desktop files

## Documentation ##

  * Public design doc
  * Complete API reference
  * Developer's guide

## Others ##
  * Localize to other languages