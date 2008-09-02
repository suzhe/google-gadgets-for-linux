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
#

IF(GGL_BUILD_LIBGGADGET_GTK)

APPLY_CONFIG(CAIRO)
APPLY_CONFIG(GTK2)

IF(RSVG_FOUND)
  APPLY_CONFIG(RSVG)
  ADD_DEFINITIONS(-DHAVE_RSVG_LIBRARY)
ENDIF(RSVG_FOUND)

SET(SRCS
  cairo_canvas.cc
  cairo_font.cc
  cairo_graphics.cc
  cairo_image_base.cc
  hotkey.cc
  key_convert.cc
  main_loop.cc
  menu_builder.cc
  pixbuf_image.cc
  single_view_host.cc
  tooltip.cc
  utilities.cc
  view_widget_binder.cc
)

if(RSVG_FOUND)
  SET(SRCS ${SRCS} rsvg_image.cc)
endif(RSVG_FOUND)

ADD_SUBDIRECTORY(tests)

ADD_LIBRARY(ggadget-gtk SHARED ${SRCS})
TARGET_LINK_LIBRARIES(ggadget-gtk
  ggadget-xdg
  ${CAIRO_LIBRARIES}
  ${GTK2_LIBRARIES}
  ${GTHREAD_LIBRARIES}
  ${PANGO_LIBRARIES}
  ${RSVG_LIBRARIES}
)
OUTPUT_LIBRARY(ggadget-gtk)

SET_TARGET_PROPERTIES(ggadget-gtk PROPERTIES
                              VERSION ${GGL_LIB_VERSION}
                              SOVERSION ${GGL_MAJOR_VERSION})

INSTALL( TARGETS ggadget-gtk
    RUNTIME DESTINATION ${BIN_INSTALL_DIR}
    LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
)

INSTALL(FILES
  cairo_graphics.h
  hotkey.h
  key_convert.h
  main_loop.h
  menu_builder.h
  single_view_host.h
  tooltip.h
  utilities.h
  view_widget_binder.h
  DESTINATION "${INCLUDE_INSTALL_DIR}/gtk" COMPONENT Devel)

INSTALL_PKG_CONFIG(libggadget-gtk)

ENDIF(GGL_BUILD_LIBGGADGET_GTK)
