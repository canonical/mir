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

set(CMAKE_GCOV gcov)

project(Mir)

cmake_minimum_required(VERSION 2.8)

cmake_policy(SET CMP0015 NEW)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

set(MIR_VERSION_MAJOR 0)
set(MIR_VERSION_MINOR 0)
set(MIR_VERSION_PATCH 10)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(use_debflags "Use build flags from dpkg-buildflags." OFF)
if(use_debflags)
  include (cmake/Debian.cmake)
endif()
include (cmake/EnableCoverageReport.cmake)
include (cmake/MirCommon.cmake)
include (cmake/Doxygen.cmake)
include (cmake/PrePush.cmake)
include (GNUInstallDirs)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Werror -Wall -pedantic -Wextra -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++0x -Werror -Wall -fno-strict-aliasing -pedantic -Wnon-virtual-dtor -Wextra -fPIC")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")

if ("${CMAKE_CXX_COMPILER}" MATCHES "clang")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-return-type-c-linkage -Wno-mismatched-tags")
endif()

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

include_directories(include/shared)

# Check for boost
find_package(Boost 1.48.0 COMPONENTS chrono date_time filesystem system thread program_options regex REQUIRED)
include_directories (
  ${Boost_INCLUDE_DIRS}
)

option(
  MIR_DISABLE_EPOLL_REACTOR
  "Disable boost::asio's epoll implementation and switch to a select-based reactor to account for ancient kernels on ppa builders."
  OFF
)
if(MIR_DISABLE_EPOLL_REACTOR)
add_definitions(
  -DBOOST_ASIO_DISABLE_EPOLL -DBOOST_ASIO_DISABLE_KQUEUE -DBOOST_ASIO_DISABLE_DEV_POLL
)
endif(MIR_DISABLE_EPOLL_REACTOR)

# Default to gbm backend
set(
  MIR_PLATFORM
  gbm
  CACHE
  STRING
  "graphics backend to build (options are 'gbm' or 'android')"
)

find_package(EGL REQUIRED)
find_package(GLESv2 REQUIRED)
find_package(GLM REQUIRED)
find_package(Protobuf REQUIRED )
find_package(GLog REQUIRED)
find_package(GFlags REQUIRED)
find_package(XKBCOMMON REQUIRED)
find_package(LTTngUST REQUIRED)

include_directories (${GLESv2_INCLUDE_DIRS})
include_directories (${EGL_INCLUDE_DIRS})
include_directories (${GLM_INCLUDE_DIRS})

add_definitions(-DANDROID_USE_STD)

if (MIR_PLATFORM STREQUAL "android")
  find_package( PkgConfig )
  include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/3rd_party/android-deps)
  list(APPEND CMAKE_SYSTEM_INCLUDE_PATH "${PROJECT_SOURCE_DIR}/3rd_party/android-deps")

  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")

  add_definitions( -DANDROID )
 
  find_package( LibHardware REQUIRED )

  #ctest does not work for android, so turn test discovery off
  set(DISABLE_GTEST_TEST_DISCOVERY ON)
elseif (MIR_PLATFORM STREQUAL "gbm")
  find_package( PkgConfig )
  pkg_check_modules( GBM REQUIRED gbm>=9.0.0)
  pkg_check_modules( DRM REQUIRED libdrm )
  pkg_check_modules( UDEV REQUIRED libudev )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__GBM__")
else ()
  message (FATAL_ERROR "MIR_BACKEND must be either 'android' or 'gbm'")
endif()

set(MIR_ANDROID_INCLUDE_DIRECTORIES) # to be filled by android-input
set(MIR_ANDROID_INPUT_COMPILE_FLAGS) # to be filled by android-input
set(MIR_3RD_PARTY_INCLUDE_DIRECTORIES)
add_subdirectory(3rd_party/)
include_directories(${MIR_3RD_PARTY_INCLUDE_DIRECTORIES})

include_directories(${MIR_ANDROID_INCLUDE_DIRECTORIES})

set(MIR_TRACEPOINT_LIB_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/mir/tools)

set(MIR_GENERATED_INCLUDE_DIRECTORIES)
macro(uses_android_input _target_name)
  set_property(TARGET ${_target_name} APPEND_STRING PROPERTY COMPILE_FLAGS "${MIR_ANDROID_INPUT_COMPILE_FLAGS}")
endmacro()
add_subdirectory(src/)
include_directories(${MIR_GENERATED_INCLUDE_DIRECTORIES})

set (OLD_CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
# Don't treat warnings as errors in 3rd_party/{gmock}
string (REPLACE " -Werror " " " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})


# Restore -Werror for non-3rd-party code
set (CMAKE_CXX_FLAGS ${OLD_CMAKE_CXX_FLAGS})

option(MIR_ENABLE_TESTS "Build tests" ON)

if (MIR_ENABLE_TESTS)
   find_package(Gtest REQUIRED)
   include_directories(${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR})
   add_subdirectory(tests/)
endif ()
add_subdirectory(benchmarks/)
add_subdirectory(tools/)
add_subdirectory(examples/)
add_subdirectory(guides/)
add_subdirectory(cmake/)

# There's no nice way to format this. Thanks CMake.
add_test(LGPL-required
  /bin/sh -c "! grep -rl 'GNU General' ${PROJECT_SOURCE_DIR}/src/client ${PROJECT_SOURCE_DIR}/include/client ${PROJECT_SOURCE_DIR}/src/shared ${PROJECT_SOURCE_DIR}/include/shared ${PROJECT_SOURCE_DIR}/src/platform ${PROJECT_SOURCE_DIR}/include/platform"
)
add_test(GPL-required
  /bin/sh -c "! grep -rl 'GNU Lesser' ${PROJECT_SOURCE_DIR}/src/server ${PROJECT_SOURCE_DIR}/include/server ${PROJECT_SOURCE_DIR}/include/test ${PROJECT_SOURCE_DIR}/tests ${PROJECT_SOURCE_DIR}/examples"
)

enable_coverage_report(mirserver)
