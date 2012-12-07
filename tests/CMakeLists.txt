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
# Authored by: Thomas Voss <thomas.voss@canonical.com>

if(NOT DEFINED ENV{MIR_TESTS_USE_REAL_GRAPHICS})
  message(STATUS "Tests are NOT run with real graphics support.")
else()
  message(STATUS "Tests are run with real graphics.")
endif()

if(NOT DEFINED ENV{MIR_TESTS_USE_REAL_INPUT})
  message(STATUS "Tests are NOT run with real input.")
else()
  message(STATUS "Tests are run with real input.")
endif()

option(
  MIR_ENABLE_DEATH_TESTS
  "Option to control whether to run death tests"
  ON
)

if(MIR_ENABLE_DEATH_TESTS)
  add_definitions(-DMIR_DEATH_TESTS_ENABLED)
endif(MIR_ENABLE_DEATH_TESTS)

add_subdirectory(acceptance-tests/)
add_subdirectory(unit-tests/)
add_subdirectory(integration-tests/)
add_subdirectory(mir-test-framework/)
add_subdirectory(mir-test-doubles/)
add_subdirectory(draw/)

if(MIR_ENABLE_DEATH_TESTS)
  add_subdirectory(death-tests/)
else()
  message(STATUS "Death tests are disabled and are not run.")
endif(MIR_ENABLE_DEATH_TESTS)
