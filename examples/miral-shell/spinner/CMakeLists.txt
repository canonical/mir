# -*- Mode: CMake; indent-tabs-mode: nil; tab-width: 2 -*-
#
# Copyright © Canonical Ltd.
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

find_package(PIL REQUIRED)
pkg_check_modules(GLESv2 REQUIRED IMPORTED_TARGET glesv2)
pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)

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

set_property(
  SOURCE eglspinner.cpp
  PROPERTY COMPILE_OPTIONS -Wno-variadic-macros)

pkg_check_modules(WAYLAND_EGL REQUIRED IMPORTED_TARGET wayland-egl)

target_link_libraries(miral-spinner
  example-shell-lib
  PkgConfig::EGL
  PkgConfig::WAYLAND_EGL
  PkgConfig::GLIB
  PkgConfig::GLESv2
)

target_include_directories(miral-spinner
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
