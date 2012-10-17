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

#add_subdirectory(client/)
add_subdirectory(compositor/)
add_subdirectory(input/)
add_subdirectory(frontend/)
add_subdirectory(process/)

if (MIR_PLATFORM STREQUAL "android")
  add_subdirectory(graphics/android)
endif()

add_executable(
  integration-tests 
  ${INTEGRATION_TESTS_SRCS})

target_link_libraries(
  integration-tests

  mirclient
  mircompositor
  mirfrontend
  mirinput
  mirsurfaces
  mirplatformgraphics
  mirprocess

  3rd_party

  test-utils

  ${Boost_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

mir_discover_tests(integration-tests)
