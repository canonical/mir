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
  ${CMAKE_CURRENT_SOURCE_DIR}/test_session_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_session.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_the_session_container_implementation.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_registration_order_focus_selection_strategy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_single_visibility_focus_mechanism.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_protobuf_socket_communicator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_protobuf_socket_communicator_surface_counter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_protobuf_socket_communicator_fd.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_protobuf_socket_communicator_errors.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_resource_cache.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_application_mediator.cpp
)

if (MIR_PLATFORM STREQUAL "android")
  list(APPEND UNIT_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test_application_mediator_android.cpp
  )
elseif (MIR_PLATFORM STREQUAL "gbm")
  list(APPEND UNIT_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test_application_mediator_gbm.cpp
  )
endif()

set(
  UNIT_TEST_SOURCES 
  ${UNIT_TEST_SOURCES} 
  PARENT_SCOPE)
