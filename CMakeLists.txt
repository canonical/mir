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
set(MIR_VERSION_MINOR 8) # This should change at least with every MIRSERVER_ABI
set(MIR_VERSION_PATCH 0)

set(MIR_VERSION ${MIR_VERSION_MAJOR}.${MIR_VERSION_MINOR}.${MIR_VERSION_PATCH})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
execute_process(
  COMMAND ${CMAKE_CXX_COMPILER} -dumpmachine
  OUTPUT_VARIABLE TARGET_ARCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

option(use_debflags "Use build flags from dpkg-buildflags." OFF)
if(use_debflags)
  include (cmake/Debian.cmake)
endif()
include (cmake/EnableCoverageReport.cmake)
include (cmake/MirCommon.cmake)
include (cmake/PrePush.cmake)
include (GNUInstallDirs)

set(build_types "None;Debug;Release;RelWithDebInfo;MinSizeRel;Coverage;AddressSanitizer")
# Change informational string for CMAKE_BUILD_TYPE
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "${build_types}" FORCE)
# Enable cmake-gui to display a drop down list for CMAKE_BUILD_TYPE
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "${build_types}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -g -Werror -Wall -pedantic -Wextra -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -g -std=c++0x -Werror -Wall -fno-strict-aliasing -pedantic -Wnon-virtual-dtor -Wextra -fPIC")

if ("${CMAKE_CXX_COMPILER}" MATCHES "clang")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-return-type-c-linkage -Wno-mismatched-tags")
endif()

# Link time optimization allows leaner, cleaner libraries
message(STATUS "CMAKE_C_COMPILER: " ${CMAKE_C_COMPILER})

option(MIR_LINK_TIME_OPTIMIZATION "Enables the linker to optimize binaries." OFF)
if(MIR_LINK_TIME_OPTIMIZATION)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" cmake_build_type_lower)

#####################################################################
# Enable code coverage calculation with gcov/gcovr/lcov
# Usage:
#  * Switch build type to coverage (use ccmake or cmake-gui)
#  * Invoke make, make test, make coverage
#  * Find html report in subdir coveragereport
#  * Find xml report feasible for jenkins in coverage.xml
#####################################################################
if(cmake_build_type_lower MATCHES "coverage")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftest-coverage -fprofile-arcs")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ftest-coverage -fprofile-arcs")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -ftest-coverage -fprofile-arcs")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -ftest-coverage -fprofile-arcs")
endif()

if(cmake_build_type_lower MATCHES "addresssanitizer")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fsanitize=address")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
else()
  # AddressSanitizer builds fail if we disallow undefined symbols
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-undefined")
endif()

# Define LOG_NDEBUG=1 to ensure Android ALOGV calls are not compiled in to
# consume CPU time...
add_definitions(-DLOG_NDEBUG=1)

enable_testing()

include_directories(include/common include.private/common)

# Check for boost
find_package(Boost 1.48.0 COMPONENTS date_time system program_options iostreams filesystem REQUIRED)
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

add_definitions(-DMESA_EGL_NO_X11_HEADERS)

# Default to mesa backend
set(
  MIR_PLATFORM
  mesa
  CACHE
  STRING
  "a list of graphics backends to build (options are 'mesa' or 'android')"
)

list(GET MIR_PLATFORM 0 MIR_TEST_PLATFORM)

foreach(platform IN LISTS MIR_PLATFORM)
  if (platform STREQUAL "mesa")
    set(MIR_BUILD_PLATFORM_MESA TRUE)
  endif()
  if (platform STREQUAL "android")
     set(MIR_BUILD_PLATFORM_ANDROID TRUE)
  endif()
endforeach(platform)

find_package(EGL REQUIRED)
find_package(GLESv2 REQUIRED)
find_package(GLM REQUIRED)
find_package(Protobuf REQUIRED )
find_package(GLog REQUIRED)
find_package(GFlags REQUIRED)
find_package(XKBCOMMON REQUIRED)
find_package(LTTngUST REQUIRED)
pkg_check_modules(UDEV REQUIRED libudev)

include_directories (${GLESv2_INCLUDE_DIRS})
include_directories (${EGL_INCLUDE_DIRS})
include_directories (${GLM_INCLUDE_DIRS})

if (MIR_BUILD_PLATFORM_ANDROID)
  find_package(AndroidProperties REQUIRED)
  find_package(LibHardware REQUIRED)
endif()

if (MIR_BUILD_PLATFORM_MESA)
  find_package( PkgConfig )
  pkg_check_modules( GBM REQUIRED gbm>=9.0.0)
  pkg_check_modules( DRM REQUIRED libdrm )
endif()

set(MIR_ANDROID_INCLUDE_DIRECTORIES) # to be filled by android-input
set(MIR_ANDROID_INPUT_COMPILE_FLAGS) # to be filled by android-input
set(MIR_XCURSOR_INCLUDE_DIRECTORIES)
set(MIR_3RD_PARTY_INCLUDE_DIRECTORIES)
add_subdirectory(3rd_party/)

set(MIR_TRACEPOINT_LIB_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/mir/tools)

set(MIR_GENERATED_INCLUDE_DIRECTORIES)
macro(uses_android_input _target_name)
  set_property(TARGET ${_target_name} APPEND_STRING PROPERTY COMPILE_FLAGS "${MIR_ANDROID_INPUT_COMPILE_FLAGS}")
endmacro()
add_subdirectory(src/)
include_directories(${MIR_GENERATED_INCLUDE_DIRECTORIES})

option(MIR_ENABLE_TESTS "Build tests" ON)

add_subdirectory(benchmarks/)
add_subdirectory(tools/)
add_subdirectory(examples/)
add_subdirectory(guides/)
add_subdirectory(cmake/)

if (MIR_ENABLE_TESTS)
  find_package(Gtest REQUIRED)
  include_directories(${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR})
  add_subdirectory(tests/)

  # There's no nice way to format this. Thanks CMake.
  add_test(LGPL-required
    /bin/sh -c "! grep -rl 'GNU General' ${PROJECT_SOURCE_DIR}/src/client ${PROJECT_SOURCE_DIR}/include/client ${PROJECT_SOURCE_DIR}/src/common ${PROJECT_SOURCE_DIR}/include/common ${PROJECT_SOURCE_DIR}/src/platform ${PROJECT_SOURCE_DIR}/include/platform"
  )
  add_test(GPL-required
    /bin/sh -c "! grep -rl 'GNU Lesser' ${PROJECT_SOURCE_DIR}/src/server ${PROJECT_SOURCE_DIR}/include/server ${PROJECT_SOURCE_DIR}/include/test ${PROJECT_SOURCE_DIR}/tests ${PROJECT_SOURCE_DIR}/examples"
  )
  
  add_test(NAME server-ABI-unchanged
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMAND sha1sum -c server-ABI-sha1sums
  )
  
  add_test(NAME client-ABI-unchanged
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMAND sha1sum -c client-ABI-sha1sums
  )

  add_test(NAME common-ABI-unchanged
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMAND sha1sum -c common-ABI-sha1sums
  )

  add_test(NAME platform-ABI-unchanged
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMAND sha1sum -c platform-ABI-sha1sums
  )
endif ()

enable_coverage_report(mirserver)
include (cmake/Doxygen.cmake)
