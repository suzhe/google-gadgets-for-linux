#!/bin/sh
aclocal -I autotools
autoheader
libtoolize --force --copy --automake
automake --add-missing --copy
autoconf
