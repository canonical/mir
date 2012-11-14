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
add_subdirectory(utils/)

if(MIR_ENABLE_DEATH_TESTS)
    add_subdirectory(death-tests/)
endif(MIR_ENABLE_DEATH_TESTS)
