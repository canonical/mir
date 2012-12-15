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
# Authored by: Thomas Voß <thomas.voss@canonical.com>

find_program(VERA_EXECUTABLE vera++)

if(VERA_EXECUTABLE)
  message(STATUS "vera++ available - enabling make target style_check")

  add_custom_target(
    style_check
    COMMAND "find" "${CMAKE_SOURCE_DIR}/include" "-name" "'*.h'" "|" "xargs" "vera++" "-profile" "mir_vera_profile"
    COMMAND "find" "${CMAKE_SOURCE_DIR}/src" "-name" "'*.h'" "|" "xargs" "vera++" "-profile" "mir_vera_profile"
    COMMAND "find" "${CMAKE_SOURCE_DIR}/src" "-name" "'*.cpp'" "|" "xargs" "vera++" "-profile" "mir_vera_profile"
    COMMAND "find" "${CMAKE_SOURCE_DIR}/tests" "-name" "'*.h'" "|" "xargs" "vera++" "-profile" "mir_vera_profile"
    COMMAND "find" "${CMAKE_SOURCE_DIR}/tests" "-name" "'*.cpp'" "|" "xargs" "vera++" "-profile" "mir_vera_profile"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
else()
  message(WARNING "vera++ not available - disabling make target style_check")
endif(VERA_EXECUTABLE)