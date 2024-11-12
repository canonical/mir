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

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
)

add_library(miral-user-decoration-example
  user_decoration_manager_example.cpp
)

target_include_directories(miral-user-decoration-example
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include/miral # Just so I don't have to install miral on every change of the new files
)
