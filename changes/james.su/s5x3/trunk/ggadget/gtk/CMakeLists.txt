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

PKGCONFIG_EX(cairo 1.0 CAIRO_LIBRARIES)
PKGCONFIG_EX(gtk+-2.0 2.6 GTK_LIBRARIES)
PKGCONFIG_EX(librsvg-2.0 2.14 RSVG_LIBRARIES)

ADD_DEFINITIONS(-DHAVE_RSVG_LIBRARY)

SET(SRCS
  cairo_canvas.cc
  cairo_graphics.cc
  cairo_font.cc
  global_file_manager.cc
  gtk_edit.cc
  gtk_gadget_host.cc
  gtk_main_loop.cc
  gtk_view_host.cc
  gadget_view_widget.cc
  gtk_key_convert.cc
  gtk_menu_impl.cc
  options.cc
  pixbuf_image.cc
  pixbuf_utils.cc
  rsvg_image.cc
)

ADD_SUBDIRECTORY(tests)

ADD_LIBRARY(ggadget_gtk SHARED ${SRCS})
TARGET_LINK_LIBRARIES(ggadget_gtk
  ${CAIRO_LIBRARIES}
  ${GTK_LIBRARIES}
  ${RSVG_LIBRARIES}
)
OUTPUT_LIBRARY(ggadget_gtk)
