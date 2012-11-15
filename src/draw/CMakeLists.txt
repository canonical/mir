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
# Authored by: Alan Griffiths <alan@octopull.co.uk>

include_directories(
    ${Boost_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
)

set(
  DRAW_SRCS

  graphics_utils.cpp 
)

if (MIR_PLATFORM STREQUAL "android")
  set(
    DRAW_SRCS

    android_graphics.cpp
    ${DRAW_SRCS} 
    )
endif()

add_library(
  mirdraw SHARED 
  ${DRAW_SRCS})

target_link_libraries(
  mirdraw 

  3rd_party

  ${PROTOBUF_LIBRARIES}
  ${Boost_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)
