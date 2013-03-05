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

list(
  APPEND INPUT_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/android_input_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/android_input_reader_policy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/android_pointer_controller.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/android_input_lexicon.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/rudimentary_input_reader_policy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/event_filter_dispatcher_policy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/default_android_input_configuration.cpp
)

set(
  INPUT_SOURCES
  ${INPUT_SOURCES}
  PARENT_SCOPE
)

