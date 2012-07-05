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
# Authored by: Thomas Guest <thomas.guest@canonical.com>

set(EXCLUSIONS
    -build/include_order,
    -build/header_guard,
    -readability/braces,
    -readability/casting,
    -readability/streams,
    -readability/todo,
    -whitespace/blank_line,
    -whitespace/braces,
    -whitespace/comments,
    -whitespace/labels,
    -whitespace/newline)

string(REPLACE "" "" EXCLUSIONS ${EXCLUSIONS})

file(GLOB_RECURSE SRC ${CMAKE_SOURCE_DIR}/src/*.cpp ${CMAKE_SOURCE_DIR}/include/*.h)

add_custom_target(cpplint python ${CMAKE_CURRENT_SOURCE_DIR}/cpplint.py 
    --filter=${EXCLUSIONS} ${SRC})
