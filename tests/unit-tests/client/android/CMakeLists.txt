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

list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gralloc_registrar.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_native_window.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_client_platform.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_client_surface_interpreter.cpp
  $<TARGET_OBJECTS:mirclientplatformandroidobjects>
  $<TARGET_OBJECTS:mirclient_platformimpl>
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
