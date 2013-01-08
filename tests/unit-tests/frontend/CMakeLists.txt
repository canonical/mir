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
  ${CMAKE_CURRENT_SOURCE_DIR}/test_protobuf_communicator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_protobuf_surface_apis.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_protobuf_reports_errors.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_resource_cache.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_application_mediator.cpp
)

# TODO this test is a mess - something better is needed.
# - it doesn't pass FDs between processes
# - it doesn't test that the received FD can be used
# - the code it tests [test_file_descriptors()] isn't part of the system proper
# - it touches the filesystem
# Really, it tests for a side-effect of passing an FD over a socket using libancillary.
# So, we should write an integration level test that passes FDs between
# processes and get rid of this test and test_file_descriptors()
if (MIR_TRANSPORT STREQUAL "socket")
list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_protobuf_sends_fds.cpp
)
endif()

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
