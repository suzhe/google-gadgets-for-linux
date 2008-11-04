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

IF(GGL_BUILD_GTK_HOST)

# Generates ggl-gtk.desktop file
SET(bindir ${CMAKE_INSTALL_PREFIX}/bin)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/ggl-gtk.desktop.in"
  "${CMAKE_CURRENT_BINARY_DIR}/ggl-gtk.desktop.in"
  @ONLY)
ADD_CUSTOM_TARGET(GenAppDesktop ALL
  ./intl_desktop_file ${CMAKE_CURRENT_BINARY_DIR}/ggl-gtk.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/ggl-gtk.desktop
  DEPENDS intl_desktop_file
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/output/bin
  )
INSTALL(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/ggl-gtk.desktop
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/share/applications
  )

APPLY_CONFIG(CAIRO)
APPLY_CONFIG(GTK2)

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/output/lib)

ADD_DEFINITIONS(-DGGL_APP_NAME=\\\"ggl-gtk\\\")

SET(SRCS
  main.cc
  sidebar_gtk_host.cc
  simple_gtk_host.cc
)

ADD_EXECUTABLE(ggl-gtk ${SRCS})
OUTPUT_EXECUTABLE(ggl-gtk)

TARGET_LINK_LIBRARIES(ggl-gtk
  ggadget${GGL_EPOCH}
  ggadget-gtk${GGL_EPOCH}
  ${CAIRO_LIBRARIES}
  ${GTK2_LIBRARIES}
)

INSTALL( TARGETS ggl-gtk
    RUNTIME DESTINATION ${BIN_INSTALL_DIR}
    LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
)
ENDIF(GGL_BUILD_GTK_HOST)
