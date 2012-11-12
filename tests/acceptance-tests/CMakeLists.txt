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
# Authored by: Thomas Voss <thomas.voss@canonical.com>,

include_directories(
  ${CMAKE_SOURCE_DIR}
)

add_executable(
  acceptance-tests
  
  test_client_library.cpp
  test_surfaceloop.cpp
  test_test_framework.cpp
  test_server_startup.cpp
)

target_link_libraries(
  acceptance-tests
  
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

  test-utils
  3rd_party
  
  ${PROTOBUF_LIBRARIES}
  ${Boost_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

mir_discover_tests(acceptance-tests)
