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
  4.6
  CACHE
  STRING
  "Version of gcc/g++"
)

set(CMAKE_C_COMPILER gcc-${MIR_GCC_VERSION})
set(CMAKE_CXX_COMPILER g++-${MIR_GCC_VERSION})
set(CMAKE_GCOV gcov-${MIR_GCC_VERSION})

project(Mir)

cmake_minimum_required(VERSION 2.8)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

set(MIR_VERSION_MAJOR 0)
set(MIR_VERSION_MINOR 0)
set(MIR_VERSION_PATCH 1)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include (cmake/Debian.cmake)
include (cmake/EnableCoverageReport.cmake)
include (cmake/MirCommon.cmake)
include (cmake/Doxygen.cmake)
include (cmake/PrePush.cmake)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Werror -Wall -fno-strict-aliasing -pedantic -Wextra -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall -fno-strict-aliasing -pedantic -Wextra -fPIC")

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
find_package(Boost 1.48.0 COMPONENTS chrono date_time filesystem system thread program_options REQUIRED)
include_directories (
  ${Boost_INCLUDE_DIRS}
)

# Default to gbm backend
set(
  MIR_PLATFORM
  gbm
  CACHE
  STRING
  "graphics backend to build (options are 'gbm' or 'android')"
)

set (MIR_TRANSPORT
  socket
  CACHE
  STRING
  "IPC transport to build (options are 'socket' or 'binder')"
)

set (MIR_CUTDOWN_FOR_EMULATOR
  false
  CACHE
  BOOL
  "Disable parts of system that don't run on Emulator"
)

find_package(EGL REQUIRED)
find_package(GLESv2 REQUIRED)
find_package(Protobuf REQUIRED )

if (MIR_PLATFORM STREQUAL "android")
#  include (cmake/AndroidFlags.cmake)
#  get_android_flags()
#  add_definitions( ${ANDROID_DEFINITIONS} )
#  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ANDROID_CXX_FLAGS}")
#  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ANDROID_LINKER_FLAGS}")
#  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${ANDROID_LINKER_FLAGS} ${ANDROID_SO_LINKER_FLAGS}")

  add_definitions(

    -D_GLIBCXX_HAS_GTHREADS;
    -D_GLIBCXX_USE_C99_STDINT_TR1;
    -DHAVE_ANDROID_OS=1;
    -D_GLIBCXX_USE_SCHED_YIELD;
    -DHAVE_FUTEX; 
    -DHAVE_FUTEX_WRAPPERS=1; 
    -DHAVE_FORKEXEC; 
    -DHAVE_OOM_ADJ; 
    -DHAVE_ANDROID_IPC; 
    -DHAVE_POSIX_FILEMAP; 
    -DHAVE_TERMIO_H; 
    -DHAVE_SYS_UIO_H; 
    -DHAVE_SYMLINKS; 
    -DHAVE_IOCTL; 
    -DHAVE_POSIX_CLOCKS; 
    -DHAVE_TIMEDWAIT_MONOTONIC; 
    -DHAVE_EPOLL; 
    -DHAVE_ENDIAN_H; 
    -DHAVE_LITTLE_ENDIAN; 
    -DHAVE_BACKTRACE=0; 
    -DHAVE_DLADDR=0; 
    -DHAVE_CXXABI=0; 
    -DHAVE_SCHED_SETSCHEDULER; 
    -D__linux__; 
    -DHAVE_MALLOC_H; 
    -DHAVE_LINUX_LOCAL_SOCKET_NAMESPACE=1; 
    -DHAVE_INOTIFY=1; 
    -DHAVE_MADVISE=1; 
    -DHAVE_TM_GMTOFF=1; 
    -DHAVE_DIRENT_D_TYPE=1; 
    -DARCH_ARM; 
    -DOS_SHARED_LIB_FORMAT_STR="lib%s.so"; 
    -DHAVE__MEMCMP16=1; 
    -DMINCORE_POINTER_TYPE="unsigned char *"; 
    -DHAVE_SA_NOCLDWAIT; 
    -DOS_PATH_SEPARATOR='/';
    -DOS_CASE_SENSITIVE; 
    -DHAVE_SYS_SOCKET_H=1; 
    -DHAVE_PRCTL=1; 
    -DHAVE_WRITEV=1 ;
    -DHAVE_STDINT_H=1 ;
    -DHAVE_STDBOOL_H=1 ;
    -DHAVE_SCHED_H=1 ;
    -D__STDC_INT64__;
    -DANDROID_SMP;
  
 -DANDROID )

  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath-link,/opt/bundle/usr/lib/arm-linux-gnueabihf")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/opt/bundle/junk -fpermissive -Wl,-rpath-link,/opt/bundle/lib/arm-linux-gnueabihf")
  find_package( LibHardware REQUIRED )
  find_package( AndroidUI REQUIRED )

  #ctest does not work for android, so turn test discovery off
  set(DISABLE_GTEST_TEST_DISCOVERY ON)
#  set(gtest_disable_pthreads ON)

elseif (MIR_PLATFORM STREQUAL "gbm")
  find_package( PkgConfig )
  pkg_check_modules( GBM REQUIRED gbm>=9.0.0)
  pkg_check_modules( DRM REQUIRED libdrm )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__GBM__")
else ()
  message (FATAL_ERROR "MIR_BACKEND must be either 'android' or 'gbm'")
endif()

set(MIR_ANDROID_INCLUDE_DIRECTORIES) # to be filled by android-input
set(MIR_ANDROID_INPUT_COMPILE_FLAGS) # to be filled by android-input
set(MIR_3RD_PARTY_INCLUDE_DIRECTORIES)
set(MIR_3RD_PARTY_SOURCES)
add_subdirectory(3rd_party/)
include_directories(${MIR_3RD_PARTY_INCLUDE_DIRECTORIES})

include_directories(${MIR_ANDROID_INCLUDE_DIRECTORIES})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MIR_ANDROID_INPUT_COMPILE_FLAGS}")

set(MIR_GENERATED_INCLUDE_DIRECTORIES)
add_subdirectory(src/)
include_directories(${MIR_GENERATED_INCLUDE_DIRECTORIES})

# Pulling in local gmock version
# While this seems evil, we are doing
# it to ensure/allow for:
#   (1.) a matching gmock/gtest version
#   (2.) a stream-lined cross-compilation workflow
set(
  GTEST_INCLUDE_DIR

  3rd_party/gmock-1.6.0/include
  3rd_party/gmock-1.6.0/gtest/include)

add_subdirectory(3rd_party/gmock-1.6.0/)
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

add_subdirectory(benchmarks/)
add_subdirectory(tests/)
add_subdirectory(tools/)
add_subdirectory(examples/)
add_subdirectory(guides/)
add_subdirectory(cmake/)

enable_coverage_report(mir)
