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
# Authored by: Robert Carr <robert.carr@canonical.com>

list(
  APPEND ANDROID_SHARED_INPUT_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/android_input_lexicon.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/android_input_receiver.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/android_input_platform.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/event_conversion_helpers.cpp
)

set(ANDROID_SHARED_INPUT_SOURCES ${ANDROID_SHARED_INPUT_SOURCES} PARENT_SCOPE)
