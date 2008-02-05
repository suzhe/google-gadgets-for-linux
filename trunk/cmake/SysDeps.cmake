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

INCLUDE(CheckTypeSize)

SET(HAVE_STDDEF_H 1)

CHECK_TYPE_SIZE(int GGL_SIZEOF_INT)
CHECK_TYPE_SIZE(long GGL_SIZEOF_LONG_INT)
CHECK_TYPE_SIZE(size_t GGL_SIZEOF_SIZE_T)
CHECK_TYPE_SIZE(double GGL_SIZEOF_DOUBLE)

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/ggadget/sysdeps.h.in
               ${CMAKE_BINARY_DIR}/ggadget/sysdeps.h)
