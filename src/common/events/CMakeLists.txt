# Copyright © 2016 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License version 3,
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>

set(EVENT_SOURCES
  close_surface_event.cpp
  event.cpp
  input_configuration_event.cpp
  keyboard_event.cpp
  motion_event.cpp
  prompt_session_event.cpp
  surface_event.cpp
  input_event.cpp
  keymap_event.cpp
  orientation_event.cpp
  resize_event.cpp
  surface_output_event.cpp
)

add_library(
  mirevents OBJECT
  ${EVENT_SOURCES}
)

include_directories(
    ${PROJECT_SOURCE_DIR}/include/client
    ${PROJECT_SOURCE_DIR}/src/include/cookie
)
