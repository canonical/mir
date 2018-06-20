# -*- Mode: CMake; indent-tabs-mode: nil; tab-width: 2 -*-
#
# Copyright © 2014 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

pkg_check_modules(GLIB REQUIRED glib-2.0)
find_package(GLESv2 REQUIRED)
find_package(PIL REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-overlength-strings")


function(png2header png header varname)
  add_custom_command(
    OUTPUT ${header}
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/png2header.py ${png} ${varname} > ${header}
    DEPENDS ${png} ${CMAKE_CURRENT_SOURCE_DIR}/png2header.py
  )
endfunction()

png2header(
  ${CMAKE_CURRENT_SOURCE_DIR}/spinner-glow.png
  ${CMAKE_CURRENT_BINARY_DIR}/spinner_glow.h
  spinner_glow
)

png2header(
  ${CMAKE_CURRENT_SOURCE_DIR}/spinner-logo.png
  ${CMAKE_CURRENT_BINARY_DIR}/spinner_logo.h
  spinner_logo
)

include_directories(
  ${PROJECT_SOURCE_DIR}/include/client
  ${GLIB_INCLUDE_DIRS}
  ${GLESv2_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
)

add_library(miral-spinner STATIC
  eglapp.cpp
  eglapp.h
  eglspinner.cpp
  miregl.h
  miregl.cpp
  splash.h
  ${CMAKE_CURRENT_BINARY_DIR}/spinner_logo.h
  ${CMAKE_CURRENT_BINARY_DIR}/spinner_glow.h
)

target_link_libraries(miral-spinner
  example-shell-lib
  mirclientcpp
  EGL
  wayland-egl
  ${WAYLAND_CLIENT_LDFLAGS} ${WAYLAND_CLIENT_LIBRARIES}
  ${GLIB_LDFLAGS}
  ${GLESv2_LIBRARIES}
  ${MIRCLIENT_LDFLAGS}
)
