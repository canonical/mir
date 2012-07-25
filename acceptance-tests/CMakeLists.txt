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

add_executable(
  acceptance-tests
  
  test_connect_disconnect.cpp
  display_server_test_fixture.cpp
  test_renderloop.cpp
  test_test_framework.cpp
  testing_process_manager.cpp
  testing_server_options.cpp
)

target_link_libraries(
  acceptance-tests
  
  mir
  mircompositor
  mirfrontend
  mirinput
  mirprocess
  mirsurfaces

  ${Boost_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

mir_discover_tests(acceptance-tests)
