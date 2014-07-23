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

list(APPEND MIR_COMMON_EXTRA_INCLUDE
  ${MIR_3RD_PARTY_INCLUDE_DIRECTORIES}
  ${MIR_ANDROID_INCLUDE_DIRECTORIES}
)

add_subdirectory(android)

list(APPEND MIR_COMMON_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/xkb_mapper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/udev_wrapper.cpp
)

list(APPEND MIR_COMMON_LIBRARIES
)

list(APPEND MIR_COMMON_REFERENCES
  android-input
  ${XKBCOMMON_LIBRARIES}
  ${UDEV_LDFLAGS} ${UDEV_LIBRARIES}
)

set(MIR_COMMON_EXTRA_INCLUDE ${MIR_COMMON_EXTRA_INCLUDE} PARENT_SCOPE)
set(MIR_COMMON_SOURCES ${MIR_COMMON_SOURCES} PARENT_SCOPE)
set(MIR_COMMON_LIBRARIES ${MIR_COMMON_LIBRARIES} PARENT_SCOPE)
set(MIR_COMMON_REFERENCES ${MIR_COMMON_REFERENCES} PARENT_SCOPE)
