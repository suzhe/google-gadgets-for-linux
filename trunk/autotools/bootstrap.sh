#!/bin/sh
aclocal -I autotools
autoheader
libtoolize --force --copy --automake --ltdl
automake --add-missing --copy
autoconf
