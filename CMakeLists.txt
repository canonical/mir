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
# Authored by: Thomas Voss <thomas.voss@canonical.com>,
#              Alan Griffiths <alan@octopull.co.uk>

project(Mir)

cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Werror -Wall -pedantic -Wextra")
include (cmake/MirCommon.cmake)

enable_testing()

include_directories(include/)

add_subdirectory(src/)

# Check for boost
find_package(Boost 1.48 COMPONENTS chrono filesystem system REQUIRED)

# Build Google Test and make its headers known
# Check for google test and build it locally
set(GTEST_ROOT_DIR
  "/usr/src/gtest" # Default value, adjustable by user with e.g., ccmake
  CACHE
  PATH
  "Path to Google test srcs"
  )
  
find_path(GTEST_INCLUDE_DIR gtest/gtest.h)
  
if(GTEST_INCLUDE_DIR)
  add_subdirectory(${GTEST_ROOT_DIR} gtest)
  
  set(GTEST_BOTH_LIBRARIES gtest gtest_main)
  set(GTEST_FOUND TRUE)
endif(GTEST_INCLUDE_DIR)

# TODO(tvoss): Move to its own FindGoogleMock module
find_library(GMOCK_LIBRARY gmock)
find_library(GMOCK_MAIN_LIBRARY gmock_main)

if (NOT GMOCK_LIBRARY OR NOT GMOCK_MAIN_LIBRARY OR NOT GTEST_FOUND)
  message ("Google Mock and Google Test not found - cannot build tests!")
endif (NOT GMOCK_LIBRARY OR NOT GMOCK_MAIN_LIBRARY OR NOT GTEST_FOUND)

include_directories (
  ${Boost_INCLUDE_DIRS}
  ${GTEST_INCLUDE_DIRS}
)

find_package (Threads REQUIRED)

add_subdirectory(acceptance-tests/)
add_subdirectory(end-to-end-tests/)
add_subdirectory(unit-tests/)
add_subdirectory(integration-tests/)
add_subdirectory(guides/)
add_subdirectory(cmake/)
