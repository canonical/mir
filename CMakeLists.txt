# Copyright © 2012-2017 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 or 3 as
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

cmake_minimum_required(VERSION 3.5)

cmake_policy(SET CMP0015 NEW)
cmake_policy(SET CMP0022 NEW)

set(CMAKE_GCOV gcov)

project(Mir)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(MIR_VERSION_MAJOR 0)
set(MIR_VERSION_MINOR 28)
set(MIR_VERSION_PATCH 1)

add_definitions(-DMIR_VERSION_MAJOR=${MIR_VERSION_MAJOR})
add_definitions(-DMIR_VERSION_MINOR=${MIR_VERSION_MINOR})
add_definitions(-DMIR_VERSION_MICRO=${MIR_VERSION_PATCH})
add_definitions(-D_GNU_SOURCE)
add_definitions(-D_FILE_OFFSET_BITS=64)

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
include (GNUInstallDirs)
include (cmake/Doxygen.cmake)

set(build_types "None;Debug;Release;RelWithDebInfo;MinSizeRel;Coverage;AddressSanitizer;ThreadSanitizer;UBSanitizer")
# Change informational string for CMAKE_BUILD_TYPE
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "${build_types}" FORCE)
# Enable cmake-gui to display a drop down list for CMAKE_BUILD_TYPE
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "${build_types}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -g -Werror -Wall -pedantic -Wextra -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -g -std=c++14 -Werror -Wall -fno-strict-aliasing -pedantic -Wnon-virtual-dtor -Wextra -fPIC")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--as-needed")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--as-needed")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-Wmismatched-tags HAS_W_MISMATCHED_TAGS)

if(HAS_W_MISMATCHED_TAGS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-mismatched-tags")
endif()

# GCC 7.1 fixed a bug in the ARM ABI, which results in some std::vector methods
# (among others) generating this warning.
#
# There's nothing we can do about it; everything just needs to be rebuilt with
# GCC 7.1.
check_cxx_compiler_flag(-Wpsabi HAS_W_PSABI)
if(HAS_W_PSABI)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")
endif()

option(MIR_USE_LD_GOLD "Enables the \"gold\" linker." OFF)
if(MIR_USE_LD_GOLD)
  set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=gold")
  set (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fuse-ld=gold")
  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=gold")
endif()

# Link time optimization allows leaner, cleaner libraries
option(MIR_LINK_TIME_OPTIMIZATION "Enables the linker to optimize binaries." OFF)
if(MIR_LINK_TIME_OPTIMIZATION)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto -ffat-lto-objects")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto -ffat-lto-objects")
  if(${CMAKE_COMPILER_IS_GNUCXX})
    set(CMAKE_NM "gcc-nm")
    set(CMAKE_AR "gcc-ar")
    set(CMAKE_RANLIB "gcc-ranlib")
  endif()
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
elseif(cmake_build_type_lower MATCHES "threadsanitizer")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -fno-omit-frame-pointer")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread -fno-omit-frame-pointer")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fsanitize=thread")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=thread")
  if (CMAKE_COMPILER_IS_GNUCXX)
    # Work around GCC bug. It should automatically link to tsan when
    # -fsanitize=thread is used, but doesn't.
    #
    # Linking everything with tsan is harmless and simple, so do that.
    link_libraries(tsan) # Workaround for LP:1413474
  endif()
elseif(cmake_build_type_lower MATCHES "ubsanitizer")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -fno-omit-frame-pointer")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined -fno-omit-frame-pointer")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fsanitize=undefined")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=undefined")
  # "Symbol already defined" errors occur with pre-compiled headers
  SET(MIR_USE_PRECOMPILED_HEADERS OFF CACHE BOOL "Use precompiled headers" FORCE)
else()
  # AddressSanitizer builds fail if we disallow undefined symbols
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--no-undefined")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-undefined")
endif()

# Define LOG_NDEBUG=1 to ensure Android ALOGV calls are not compiled in to
# consume CPU time...
add_definitions(-DLOG_NDEBUG=1)

enable_testing()

include_directories(include/core)
include_directories(include/common)
include_directories(include/cookie)

# Check for boost
find_package(Boost 1.48.0 COMPONENTS date_time system program_options filesystem REQUIRED)
include_directories (SYSTEM
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

# Default to KMS backend, but build all of them
set(
  MIR_PLATFORM
  mesa-kms;mesa-x11;eglstream-kms
  CACHE
  STRING
  "a list of graphics backends to build (options are 'mesa-kms', 'mesa-x11', or 'eglstream-kms')"
)

list(GET MIR_PLATFORM 0 MIR_TEST_PLATFORM)

option(MIR_ENABLE_TESTS "Build tests" ON)

foreach(platform IN LISTS MIR_PLATFORM)
  if (platform STREQUAL "mesa-kms")
    set(MIR_BUILD_PLATFORM_MESA_KMS TRUE)
  endif()
  if (platform STREQUAL "mesa-x11")
     set(MIR_BUILD_PLATFORM_MESA_X11 TRUE)
  endif()
  if (platform STREQUAL "eglstream-kms")
     set(MIR_BUILD_PLATFORM_EGLSTREAM_KMS TRUE)
  endif()
endforeach(platform)

find_package(EGL REQUIRED)
find_package(GLESv2 REQUIRED)
find_package(GLM REQUIRED)
find_package(Protobuf REQUIRED )
find_package(CapnProto REQUIRED)
find_package(GLog REQUIRED)
find_package(GFlags REQUIRED)
find_package(LTTngUST REQUIRED)

pkg_check_modules(UDEV REQUIRED libudev)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(WAYLAND_SERVER REQUIRED wayland-server)
pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)

include_directories (SYSTEM ${GLESv2_INCLUDE_DIRS})
include_directories (SYSTEM ${EGL_INCLUDE_DIRS})
include_directories (SYSTEM ${GLM_INCLUDE_DIRS})

#
# Full OpenGL support is possibly complete but not yet perfect. So is
# presently disabled by default due to:
#   1. Black windows bug: https://bugs.freedesktop.org/show_bug.cgi?id=92265
#   2. Use of glEGLImageTargetTexture2DOES in:
#        src/platform/graphics/egl_extensions.cpp
#      possibly shouldn't work even though it does. Or should it?
#
#if (TARGET_ARCH STREQUAL "x86_64-linux-gnu" OR
#    TARGET_ARCH STREQUAL "i386-linux-gnu")
#  set(DEFAULT_LIBGL "libGL")
#else()
  set(DEFAULT_LIBGL "libGLESv2")
#endif()
set(MIR_SERVER_LIBGL ${DEFAULT_LIBGL} CACHE STRING "OpenGL library to use in Mir servers {libGL,libGLESv2}")

if (MIR_SERVER_LIBGL STREQUAL "libGL")
  pkg_check_modules(GL REQUIRED gl)
  add_definitions(
    -DGL_GLEXT_PROTOTYPES
    -DMIR_SERVER_GL_H=<GL/gl.h>
    -DMIR_SERVER_GLEXT_H=<GL/glext.h>
    -DMIR_SERVER_EGL_OPENGL_BIT=EGL_OPENGL_BIT
    -DMIR_SERVER_EGL_OPENGL_API=EGL_OPENGL_API
  )
elseif (MIR_SERVER_LIBGL STREQUAL "libGLESv2")
  pkg_check_modules(GL REQUIRED glesv2)
  add_definitions(
    -DMIR_SERVER_GL_H=<GLES2/gl2.h>
    -DMIR_SERVER_GLEXT_H=<GLES2/gl2ext.h>
    -DMIR_SERVER_EGL_OPENGL_BIT=EGL_OPENGL_ES2_BIT
    -DMIR_SERVER_EGL_OPENGL_API=EGL_OPENGL_ES_API
  )
else()
  message(FATAL_ERROR "Invalid MIR_SERVER_LIBGL value ${MIR_SERVER_LIBGL}")
endif()

if (MIR_BUILD_PLATFORM_MESA_KMS OR MIR_BUILD_PLATFORM_MESA_X11)
  find_package( PkgConfig )
  pkg_check_modules( GBM REQUIRED gbm>=9.0.0)
  pkg_check_modules( DRM REQUIRED libdrm )
  if (GBM_VERSION VERSION_LESS 11)
    message(WARNING "Hybrid support requires libgbm from Mesa 11.0 or greater. Hybrid setups will not work")
    add_definitions(-DMIR_NO_HYBRID_SUPPORT)
  endif()
endif()

if (MIR_BUILD_PLATFORM_EGLSTREAM_KMS)
  pkg_check_modules(EPOXY REQUIRED epoxy)
  pkg_check_modules(DRM REQUIRED libdrm)
endif()

set(MIR_TRACEPOINT_LIB_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/mir/tools)

set(MIR_GENERATED_INCLUDE_DIRECTORIES)
add_subdirectory(src/)
include_directories(${MIR_GENERATED_INCLUDE_DIRECTORIES})

# This copy is used by users of mirplatforminputevdev
if ("${LIBINPUT_VERSION}" VERSION_LESS "1.1")
  add_definitions(-DMIR_LIBINPUT_HAS_ACCEL_PROFILE=0)
else ()
  add_definitions(-DMIR_LIBINPUT_HAS_ACCEL_PROFILE=1)
endif ()

add_subdirectory(benchmarks/)
add_subdirectory(examples/)
add_subdirectory(playground/)
add_subdirectory(guides/)
add_subdirectory(cmake/)

if (MIR_ENABLE_TESTS)
  find_package(GtestGmock REQUIRED)
  pkg_check_modules(LIBEVDEV REQUIRED libevdev)
  include_directories(${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR})
  add_subdirectory(tests/)

  # There's no nice way to format this. Thanks CMake.
  mir_add_test(NAME LGPL-required
    COMMAND /bin/sh -c "! grep -rl 'GNU General' ${PROJECT_SOURCE_DIR}/src/client ${PROJECT_SOURCE_DIR}/include/client ${PROJECT_SOURCE_DIR}/src/core ${PROJECT_SOURCE_DIR}/include/core ${PROJECT_SOURCE_DIR}/src/common ${PROJECT_SOURCE_DIR}/include/common ${PROJECT_SOURCE_DIR}/src/include/common ${PROJECT_SOURCE_DIR}/src/platform ${PROJECT_SOURCE_DIR}/include/platform ${PROJECT_SOURCE_DIR}/src/include/platform ${PROJECT_SOURCE_DIR}/src/platforms ${PROJECT_SOURCE_DIR}/include/platforms ${PROJECT_SOURCE_DIR}/src/include/platforms ${PROJECT_SOURCE_DIR}/src/renderers ${PROJECT_SOURCE_DIR}/include/renderers ${PROJECT_SOURCE_DIR}/src/include/renderers ${PROJECT_SOURCE_DIR}/src/capnproto"
  )
  mir_add_test(NAME GPL-required
    COMMAND /bin/sh -c "! grep -rl 'GNU Lesser' ${PROJECT_SOURCE_DIR}/src/server ${PROJECT_SOURCE_DIR}/include/server ${PROJECT_SOURCE_DIR}/src/include/server ${PROJECT_SOURCE_DIR}/tests ${PROJECT_SOURCE_DIR}/examples"
  )

  mir_add_test(NAME package-abis
    COMMAND /bin/sh -c "cd ${PROJECT_SOURCE_DIR} && tools/update_package_abis.sh --check --verbose")
endif ()

enable_coverage_report(mirserver)

add_custom_target(ptest
    COMMAND "${CMAKE_SOURCE_DIR}/tools/run_ctests.sh" "--cost-file" "${CMAKE_BINARY_DIR}/ptest_ctest_cost_data.txt" "sh ${CMAKE_BINARY_DIR}/discover_all_tests.sh" "--" "$$ARGS"
    )

add_custom_target(release-checks)

mir_check_no_unreleased_symbols(mirclient release-checks)
mir_check_no_unreleased_symbols(mircommon release-checks)
mir_check_no_unreleased_symbols(mircookie release-checks)
mir_check_no_unreleased_symbols(mirplatform release-checks)
mir_check_no_unreleased_symbols(mirprotobuf release-checks)
mir_check_no_unreleased_symbols(mirserver release-checks)

if (TARGET doc)
  add_custom_target(doc-show
          xdg-open ${CMAKE_BINARY_DIR}/doc/html/index.html
          DEPENDS doc)
endif()
