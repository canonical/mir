# Copyright © Canonical Ltd.
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

set(EVENT_SOURCES
  close_window_event.cpp
  event.cpp
  event_builders.cpp
  keyboard_event.cpp
  keyboard_resync_event.cpp
  touch_event.cpp
  pointer_event.cpp
  prompt_session_event.cpp
  window_event.cpp
  input_event.cpp
  input_device_state_event.cpp
  orientation_event.cpp
  resize_event.cpp
  window_output_event.cpp
  event_helpers.cpp
  window_placement_event.cpp ${PROJECT_SOURCE_DIR}/src/include/common/mir/events/window_placement_event.h
)

add_library(
  mirevents OBJECT
  ${EVENT_SOURCES}
)

target_link_libraries(mirevents
  PUBLIC
    mircore
)

target_include_directories(mirevents
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src/include/cookie
)
