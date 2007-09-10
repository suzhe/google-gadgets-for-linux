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

#ADD_SUBDIRECTORY(tests)

# FIXME need to not hard code this
SET(CAIRO_INCLUDE_DIR /usr/include/cairo)
SET(GTK_INCLUDE_DIR
  /usr/include/gtk-2.0
  /usr/lib/gtk-2.0/include
  /usr/include/atk-1.0
  /usr/include/pango-1.0
  /usr/include/glib-2.0
  /usr/lib/glib-2.0/include
)
SET(GTK_LIBRARIES gtk-x11-2.0 pangocairo-1.0)
SET(CAIRO_LIBRARIES cairo)

INCLUDE_DIRECTORIES(${GTK_INCLUDE_DIR}
  ${CAIRO_INCLUDE_DIR}
)

SET(SRCS
  main.cc
  gtk_cairo_host.cc
  gadget_view_widget.cc
)

ADD_EXECUTABLE(ggl ${SRCS})
OUTPUT_EXECUTABLE(ggl)

TARGET_LINK_LIBRARIES(ggl 
  ggadget ggadget_cairo 
  ${CAIRO_LIBRARIES} ${GTK_LIBRARIES}
)
