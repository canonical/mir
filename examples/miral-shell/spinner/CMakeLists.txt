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

add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-overlength-strings>)

add_library(miral-spinner STATIC
  eglapp.cpp
  eglapp.h
  eglspinner.cpp
  miregl.h
  miregl.cpp
  splash.h
  spinner_logo.h
  spinner_glow.h
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
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
)
