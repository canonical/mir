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

set(SESSION_MANAGER_BEHAVIOR_SOURCES <SOURCES>)
add_executable(session-management-cucumber-steps session_manager_steps.cpp session_management_context.cpp)

target_link_libraries(
  session-management-cucumber-steps
  
  mir
  miroptions
  mircompositor
  mirfrontend
  mirgraphics
  mirinput
  mirsurfaces
  mirplatformgraphics
  mirprotobuf
  
  mir-test-doubles
  3rd_party
  ${Boost_LIBRARIES}
  ${CUKE_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
)
