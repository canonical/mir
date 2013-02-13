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
# Authored by: Robert Carr <robert.carr@canonical.com>

include_directories(${CUKE_INCLUDE_DIRS})

set(BEHAVIOR_TESTING_LINK_LIBRARIES 
  mirserver
  
  mir-test-doubles
  3rd_party
  ${Boost_LIBRARIES}
)

set(MIR_TEST_CUCUMBER_LIB_SRCS
  session_management_context.cpp)

add_library(
  mir-test-cucumber STATIC
  ${MIR_TEST_CUCUMBER_LIB_SRCS})

set(SESSION_MANAGER_BEHAVIOR_TESTING_SOURCES session_manager_steps.cpp)
add_executable(session-management-cucumber-steps ${SESSION_MANAGER_BEHAVIOR_TESTING_SOURCES})

include_directories(${PROJECT_SOURCE_DIR}/include/mir_test_cucumber)

target_link_libraries(
  mir-test-cucumber
  ${BEHAVIOR_TESTING_LINK_LIBRARIES}
)

target_link_libraries(
  session-management-cucumber-steps
  mir-test-cucumber

  ${BEHAVIOR_TESTING_LINK_LIBRARIES}
  ${CUKE_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
)
