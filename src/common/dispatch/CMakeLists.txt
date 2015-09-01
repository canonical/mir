# Copyright © 2015 Canonical Ltd.
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
# Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>

list(
  APPEND MIR_COMMON_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/action_queue.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/multiplexing_dispatchable.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/readable_fd.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/threaded_dispatcher.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/utils.cpp
)

set(MIR_COMMON_SOURCES ${MIR_COMMON_SOURCES} PARENT_SCOPE)
