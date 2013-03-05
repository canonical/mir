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

list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_event_filter_chain.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_event_filter_input_dispatcher_policy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_pointer_controller.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_input_lexicon.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_input_reader_policy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_input_manager.cpp
)

set(
  UNIT_TEST_SOURCES
  ${UNIT_TEST_SOURCES}
  PARENT_SCOPE)
