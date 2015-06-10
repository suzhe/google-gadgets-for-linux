# Introduction #

This page describes the method of localizing Google Gadgets for Linux into other languages.


# Localize main program #

All UI strings of Google Gadgets for Linux are stored in a XML file, named strings.xml, which has the exactly same format of gadget's strings.xml. Please refer to [this document](http://code.google.com/apis/desktop/docs/script.html#internationalization) about the file format.

The original English strings are stored in [resources/en/strings.xml](http://code.google.com/p/google-gadgets-for-linux/source/browse/trunk/resources/en/strings.xml) file. The file content looks like:

```
<strings>

...

<!-- Decorated View toolbar back button tooltip. -->
<VD_BACK_BUTTON_TOOLTIP>Previous</VD_BACK_BUTTON_TOOLTIP>
<!-- Decorated View toolbar forward button tooltip. -->
<VD_FORWARD_BUTTON_TOOLTIP>Next</VD_FORWARD_BUTTON_TOOLTIP>
<!-- Decorated View toolbar toggle expanded view button tooltip. -->
<VD_POP_IN_OUT_BUTTON_TOOLTIP>Toggle Expanded View</VD_POP_IN_OUT_BUTTON_TOOLTIP>
<!-- Decorated View toolbar menu button tooltip. -->
<VD_MENU_BUTTON_TOOLTIP>Menu</VD_MENU_BUTTON_TOOLTIP>
<!-- Decorated View toolbar remove gadget button tooltip. -->
<VD_CLOSE_BUTTON_TOOLTIP>Remove</VD_CLOSE_BUTTON_TOOLTIP>

<!-- Tooltip text shown for the add gadgets button -->
<SIDEBAR_ADD_GADGETS_TOOLTIP>Add gadgets</SIDEBAR_ADD_GADGETS_TOOLTIP>
<SIDEBAR_MENU_BUTTON_TOOLTIP>Menu</SIDEBAR_MENU_BUTTON_TOOLTIP>
<SIDEBAR_MINIMIZE_BUTTON_TOOLTIP>Minimize</SIDEBAR_MINIMIZE_BUTTON_TOOLTIP>

...

</strings>
```

In order to  localize Google Gadgets for Linux, you need:
  1. Create a sub-directory in resources for the new language, for example ja for Japanese, ko for Korean or zh-TW for Traditional Chinese. The name should be a language code defined in RFC1766.
  1. Copy resources/en/strings.xml to the new directory.
  1. Translate all strings defined in strings.xml into the new language. Be sure to keep elements name unchanged.
  1. Add the new strings.xml into Makefile.am, just like other files.
  1. Rebuild and install.

You may take resources/zh-CN/strings.xml as an example.

# Localize built-in gadgets #

Besides the main program, there are some builtin gadgets need localizing as well. The localization method is same.

  * Gadget Browser
> > The source code is in [gadgets/gadget\_browser](http://code.google.com/p/google-gadgets-for-linux/source/browse/#svn/trunk/gadgets/gadget_browser) sub-directory.
  * Photos
> > The source code is in [gadgets/photos](http://code.google.com/p/google-gadgets-for-linux/source/browse/#svn/trunk/gadgets/photos) sub-directory.
  * iGoogle
> > The source code is in [gadgets/igoogle](http://code.google.com/p/google-gadgets-for-linux/source/browse/#svn/trunk/gadgets/igoogle) sub-directory.
  * Web Clip
> > The source code is in [gadgets/rss](http://code.google.com/p/google-gadgets-for-linux/source/browse/#svn/trunk/gadgets/rss) sub-directory.
  * Gadget Designer
> > The source code is in [gadgets/designer/gadget](http://code.google.com/p/google-gadgets-for-linux/source/browse/#svn/trunk/gadgets/designer/gadget) sub-directory. [gadgets/designer/blank\_gadget](http://code.google.com/p/google-gadgets-for-linux/source/browse/#svn/trunk/gadgets/designer/blank_gadget) is the blank gadget used by Gadget Designer, it also needs localizing.

# Contribute your localization work to us #

We'd very appreciate if you could contribute your localization work to us, so that other users will be benefited from it.

First of all, you may follow [this instruction](BecomingAContributor.md) to become a contributor of this project. Then you may just send your localized strings.xml file to our [dev maillist](http://groups.google.com/group/google-gadgets-for-linux-dev), or submit an issue and attach the file. Then we will merge it into our source code repository.