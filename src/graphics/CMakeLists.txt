# Copyright © 2012 Canonical Ltd.
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
#
# Authored by: Thomas Guest <thomas.guest@canonical.com>

# By default, build for LLVM

include_directories(${GLESv2_INCLUDE_DIRS})

if (MIR_PLATFORM STREQUAL "android")
  add_subdirectory(android/)
endif()

if (MIR_PLATFORM STREQUAL "gbm")
  add_subdirectory(gbm/)
endif()

add_library(
  mirgraphics STATIC

  gl_renderer.cpp
)

target_link_libraries(
  mirgraphics

  mirlogging
  
  ${GLESv2_LIBRARIES}
)
