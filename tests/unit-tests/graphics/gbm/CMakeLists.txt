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
# Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>

list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/mock_drm.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mock_gbm.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gbm_platform.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gbm_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gbm_buffer_allocator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gbm_display.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gbm_display_multi_monitor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gbm_display_configuration.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_kms_output.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_kms_page_flipper.cpp
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
