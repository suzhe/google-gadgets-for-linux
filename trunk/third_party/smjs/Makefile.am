#
# Copyright 2008 Google Inc.
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

SOURCE_PKG=js.tar.bz2
SOURCE_TOPDIR=mozilla
SOURCE_DIR=$(SOURCE_TOPDIR)/js/src

EXTRA_DIST=$(SOURCE_PKG) README

if GGL_BUILD_SMJS_SCRIPT_RUNTIME
all-local: libjs.a

libjs.a: $(SOURCE_PKG)
	bunzip2 -c $(srcdir)/$(SOURCE_PKG) | tar -xf -
	$(MAKE) BUILD_OPT=1 -C $(SOURCE_DIR) -f Makefile.ref
	mv -f $(SOURCE_DIR)/editline/*_OPT.OBJ/libedit.a .
	mv -f $(SOURCE_DIR)/*_OPT.OBJ/libjs.a .
	mv -f $(SOURCE_DIR)/*_OPT.OBJ/*.h .
	cp -f $(SOURCE_DIR)/*.h .
	cp -f $(SOURCE_DIR)/*.tbl .
	cp -f $(SOURCE_DIR)/*.msg .

clean-local:
	rm -fr *.h *.a $(SOURCE_TOPDIR)
endif
