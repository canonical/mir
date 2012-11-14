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
# Authored by: Kevin DuBois <kevin.dubois@canonical.com>

include_directories(
    ${Boost_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
)

set(
  TEST_UTILS_SRCS

  graphics_utils.cpp 
  test_protobuf_client.cpp
  event_factory.cpp
  fake_event_hub.cpp
)

if (MIR_PLATFORM STREQUAL "android")
  set(
    TEST_UTILS_SRCS

    android_graphics.cpp
    ${TEST_UTILS_SRCS} 
    )
endif()

add_library(
  mir-test-doubles STATIC 
  ${TEST_UTILS_SRCS})

target_link_libraries(
  mir-test-doubles 

  mir
  miroptions
  mircompositor
  mirfrontend
  mirgraphics
  mirinput
  mirprocess
  mirsurfaces
  mirclient
  mirplatformgraphics
  mirprotobuf

  mir-test-doubles
  3rd_party
  
  ${PROTOBUF_LIBRARIES}
  ${Boost_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)
