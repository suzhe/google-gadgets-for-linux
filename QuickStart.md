# Project Description #

Google Gadgets for Linux provides a platform for running desktop gadgets under Linux, catering to the unique needs of Linux users. We are compatible with the gadgets written for Google Desktop for Windows as well as the Universal Gadgets on iGoogle. Following Linux norms, this project is open-sourced under the Apache License.

The Windows and Mac versions of Google Desktop has provided gadget hosting functionality on Windows and Mac for a while now and the Linux version of Google Gadgets will extend this platform to Linux users. By enabling cross-platform gadgets, a large library of existing gadgets are immediately available to Linux users. In addition, gadget developers will benefit from a much larger potential user base without having to learn a new API.

There's two main components to the application: one is a common gadget library responsible for running and presenting a gadget, and the other is a host program that allows the user to choose gadgets and run them on the desktop. Currently we have hosts written for GTK+ and QT, with the GTK+ host offering a sidebar similar to that of Google Desktop for Windows.

# Quick Start Guide #

Thank you for your interest in Google Gadgets for Linux. Here's some pointers to get started:

## Building and Running the Code ##

All of the code are stored in our Subversion repository. You can get instructions on downloading the code here:

> http://code.google.com/p/google-gadgets-for-linux/source/checkout

Once checked out, you'll want to build the code in order to run it. Read the [building instructions](HowToBuild.md) Wiki page for details.

`ggl-gtk` is the output binary name for the GTK+ host. To run the code with default settings, type:

```
/usr/local/bin/ggl-gtk
```

(Assuming it's installed in the default prefix: /usr/local)

This brings up a simple window with a button allowing you to add gadgets to be run. All added gadgets are displayed right on the desktop. Try it and see how things go!

We also have a sidebar host that should be familiar to anyone who has used the Google Gadgets for Windows. The sidebar is a vertical bar where gadgets can attach on the side of the screen. To run the sidebar:

```
/usr/local/bin/ggl-gtk -s
```

In order to run it in background mode, just use -bg option, such as:

```
/usr/local/bin/ggl-gtk -s -bg
```

For best results, we recommend a recent system with software and hardware support for alpha-blended windows. Specifically, X Windows with the XComposite extension installed and a version of GTK+ greater or equal to 2.10 are preferred. Otherwise, the gadgets will show up on an opaque background that doesn't convey the same glitz that a newer system would.

## Install from binary package ##

If you don't want to install from the source package, you can choose to download and install a pre-built binary package, though it might be a little older than the latest source package.

The binary packages are provided by opensource community, please check BinaryPackages page to see if there is a binary package for your system.

## Contributing to the Project ##

Finally, if you're interested in actively contributing the project, you might be interesting in setting up a SVK infrastructure on your machine. This makes it easier to do some things like code reviews. We have a [Wiki page](SVKQuickStart.md) to help you with that.

(And [this page](MSMPTQuickStart.md) might be useful if you would like to send us code to review.)

**Note:** We'll allow external developers as soon as the developer related documentations are ready.

## Feedback ##

Even if you don't wish become a contributor, we would still like to hear your feedback. The project is still evolving, so all comments are important. Go here to file them:

> http://code.google.com/p/google-gadgets-for-linux/issues/list

Our public [discussion group for users](http://groups.google.com/group/google-gadgets-for-linux-user) is also a good place to ask questions and share experiences with other users.

And that's it, enjoy playing with the gadgets!