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
  MIR_INPUT_ENABLE_EVDEV
  "Controls if devices for handling evdev events are compiled"
  ON
)

if(MIR_INPUT_ENABLE_EVDEV)

  # Check for evemu (which may be named evemu or utouch-evemu)
  find_package(PkgConfig REQUIRED)
  pkg_search_module(EVEMU REQUIRED evemu utouch-evemu)

  list(
    APPEND INPUT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/evemu_device.cpp)

  set(
    INPUT_SOURCES 
    ${INPUT_SOURCES} 
    PARENT_SCOPE)

endif(MIR_INPUT_ENABLE_EVDEV)
  