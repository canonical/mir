# Copyright © 2015 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Authored by: Robert Carr <robert.carr@canonical.com>

list(APPEND MIR_CLIENT_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/event_builders.cpp
)

set(MIR_CLIENT_SOURCES ${MIR_CLIENT_SOURCES} PARENT_SCOPE)
