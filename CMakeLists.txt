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

set(
  MIR_GCC_VERSION
  4.4
  CACHE
  STRING
  "Version of gcc/g++"
)

set(CMAKE_C_COMPILER gcc-${MIR_GCC_VERSION})
set(CMAKE_CXX_COMPILER g++-${MIR_GCC_VERSION})

project(
  Mir
  CXX
  C)

cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Werror -Wall -pedantic -Wextra")

include (cmake/EnableCoverageReport.cmake)
include (cmake/MirCommon.cmake)

#####################################################################
# Enable code coverage calculation with gcov/gcovr/lcov
# Usage:
#  * Switch build type to coverage (use ccmake or cmake-gui)
#  * Invoke make, make test, make coverage
#  * Find html report in subdir coveragereport
#  * Find xml report feasible for jenkins in coverage.xml
#####################################################################
IF(CMAKE_BUILD_TYPE MATCHES [cC][oO][vV][eE][rR][aA][gG][eE])
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftest-coverage -fprofile-arcs" )
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ftest-coverage -fprofile-arcs" )
  SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -ftest-coverage -fprofile-arcs" )
  SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -ftest-coverage -fprofile-arcs" )
ENDIF(CMAKE_BUILD_TYPE MATCHES [cC][oO][vV][eE][rR][aA][gG][eE])

enable_testing()

include_directories(include/)

# Check for boost
find_package(Boost 1.48 COMPONENTS chrono filesystem system thread REQUIRED)
include_directories (
  ${Boost_INCLUDE_DIRS}
)

# Check for google protocol buffers and generate code
find_package( Protobuf REQUIRED )
set(MIR_GENERATED_INCLUDE_DIRECTORIES)
add_subdirectory(src/protobuf)
INCLUDE_DIRECTORIES(${MIR_GENERATED_INCLUDE_DIRECTORIES})

add_subdirectory(src/)

find_package (Threads REQUIRED)

# Pulling in local gmock version
# While this seems evil, we are doing
# it to ensure/allow for:
#   (1.) a matching gmock/gtest version
#   (2.) a stream-lined cross-compilation workflow
set(GTEST_INCLUDE_DIR gmock-1.6.0/src)

add_subdirectory(gmock-1.6.0/)
set(GMOCK_LIBRARY gmock)
set(GMOCK_MAIN_LIBRARY gmock_main)
set(
  GTEST_BOTH_LIBRARIES

  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY})
set(GTEST_FOUND TRUE)
# GMock/GTest build finished and available to subsequent components
include_directories (
  ${GTEST_INCLUDE_DIR}
)

add_subdirectory(tests/)
add_subdirectory(guides/)
add_subdirectory(cmake/)

enable_coverage_report(mir)
