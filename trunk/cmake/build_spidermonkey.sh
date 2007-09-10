#!/bin/sh
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

if [ "$1" = "Release" ]; then
  BUILD_OPT=1
fi

cd "$2/js/src"
make -f Makefile.ref
find -name '*.so' -exec cp {} "$3" \;
find -name 'libedit.a' -exec cp {} . \;
find -name 'jsautocfg.h' -exec cp -f {} . \;
