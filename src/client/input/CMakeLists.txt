# Copyright © 2012 Canonical Ltd.
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
# Authored by: Thomas Voss <thomas.voss@canonical.com>

set(
  ANDROID_SHARED_INPUT_SOURCES
)

include_directories(${MIR_3RD_PARTY_INCLUDE_DIRECTORIES})
include_directories(${MIR_ANDROID_INCLUDE_DIRECTORIES})

add_subdirectory(android)

add_library(mirsharedinput OBJECT
  ${ANDROID_SHARED_INPUT_SOURCES}
  input_event.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/xkb_mapper.cpp
)

list(APPEND MIR_CLIENT_REFERENCES ${XKBCOMMON_LIBRARIES})

set(MIR_CLIENT_SOURCES ${MIR_CLIENT_SOURCES} PARENT_SCOPE)
set(MIR_CLIENT_REFERENCES ${MIR_CLIENT_REFERENCES} PARENT_SCOPE)
