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
#              Alan Griffiths <alan@octopull.co.uk>
add_definitions(-DTEST_RECORDINGS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/input_recordings/")

set(UNIT_TEST_SOURCES test_gmock_fixes.cpp test_synchronizer.cpp)

add_subdirectory(compositor/)
add_subdirectory(frontend/)
add_subdirectory(geometry/)
add_subdirectory(input/)
add_subdirectory(surfaces/)

add_executable(unit-tests ${UNIT_TEST_SOURCES})

target_link_libraries(
  unit-tests

  mircompositor
  mirfrontend
  mirinput
  mirsurfaces

  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${Boost_LIBRARIES}
  ${EVEMU_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread. 
)

mir_discover_tests(unit-tests)
