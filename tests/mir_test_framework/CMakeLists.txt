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
# Authored by: Alan Griffiths <alan@octopull.co.uk>

include_directories(
    ${Boost_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
)

set(
  TEST_FRAMEWORK_SRCS

  testing_server_options.cpp
  testing_process_manager.cpp
  display_server_test_fixture.cpp
  process.cpp
  signal_dispatcher.cpp
)

if (MIR_TRANSPORT STREQUAL "socket")
list(APPEND TEST_FRAMEWORK_SRCS
  socket_detect_server.cpp
)
elseif (MIR_TRANSPORT STREQUAL "binder")
list(APPEND TEST_FRAMEWORK_SRCS
  binder_detect_server.cpp
)
endif()

add_library(
  mir-test-framework STATIC
  ${TEST_FRAMEWORK_SRCS})

target_link_libraries(
  mir-test-framework

  mirserver
  mirclient

  3rd_party

  ${PROTOBUF_LIBRARIES}
  ${Boost_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)
