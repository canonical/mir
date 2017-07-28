# Copyright © 2016 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License version 2 or 3,
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
  keyboard_event.cpp
  touch_event.cpp
  pointer_event.cpp
  prompt_session_event.cpp
  surface_event.cpp
  input_event.cpp
  input_device_state_event.cpp
  input_configuration_event.cpp
  keymap_event.cpp
  orientation_event.cpp
  resize_event.cpp
  surface_output_event.cpp
  surface_placement_event.cpp       ${PROJECT_SOURCE_DIR}/src/include/common/mir/events/surface_placement_event.h
)

add_library(
  mirevents OBJECT
  ${EVENT_SOURCES}
)

add_dependencies(mirevents mircapnproto)

include_directories(
    ${PROJECT_SOURCE_DIR}/include/client
    ${PROJECT_SOURCE_DIR}/src/include/cookie
    ${MIR_GENERATED_INCLUDE_DIRECTORIES}
)
