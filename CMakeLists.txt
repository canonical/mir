# Copyright © Canonical Ltd.
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

cmake_minimum_required(VERSION 3.16)

cmake_policy(SET CMP0015 NEW)
cmake_policy(SET CMP0022 NEW)

set(CMAKE_GCOV gcov)

project(Mir)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(MIR_VERSION_MAJOR 2)
set(MIR_VERSION_MINOR 20)
set(MIR_VERSION_PATCH 0)

add_compile_definitions(MIR_VERSION_MAJOR=${MIR_VERSION_MAJOR})
add_compile_definitions(MIR_VERSION_MINOR=${MIR_VERSION_MINOR})
add_compile_definitions(MIR_VERSION_MICRO=${MIR_VERSION_PATCH})
add_compile_definitions(_GNU_SOURCE)
add_compile_definitions(_FILE_OFFSET_BITS=64)

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
add_subdirectory(doc)

set(build_types "None;Debug;Release;RelWithDebInfo;MinSizeRel;Coverage;AddressSanitizer;ThreadSanitizer;UBSanitizer")
# Change informational string for CMAKE_BUILD_TYPE
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "${build_types}" FORCE)
# Enable cmake-gui to display a drop down list for CMAKE_BUILD_TYPE
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "${build_types}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -g -Wall -pedantic -Wextra -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -g -Wall -fno-strict-aliasing -pedantic -Wnon-virtual-dtor -Wextra -fPIC")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--as-needed")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--as-needed")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")


set(MIR_FATAL_COMPILE_WARNINGS ON CACHE BOOL "Should compiler warnings fail the build")
if(MIR_FATAL_COMPILE_WARNINGS)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
  # Disable maybe-uninitialized errors on GCC when optimisation is enabled
  # They are notorious for false-positives
  # Also, disable array-bounds errors, they are likewise significantly
  # false-positivey
  # (We particularly hit https://gcc.gnu.org/bugzilla/show_bug.cgi?id=111415)
  if ((CMAKE_C_COMPILER_ID MATCHES "GNU") AND NOT (CMAKE_BUILD_TYPE MATCHES "Debug"))
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error=maybe-uninitialized -Wno-error=array-bounds")
  endif()
  if ((CMAKE_CXX_COMPILER_ID MATCHES "GNU") AND NOT (CMAKE_BUILD_TYPE MATCHES "Debug"))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=maybe-uninitialized -Wno-error=array-bounds")
  endif()
endif()

set(MIR_COMPILER_QUIRKS "" CACHE STRING "Semicolon-separated list of compiler quirks to enable")

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

set(MIR_USE_LD ld CACHE STRING "Linker to use")
set_property(CACHE MIR_USE_LD PROPERTY STRINGS "ld;gold;lld;mold")
if(MIR_USE_LD MATCHES "gold")
  set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=gold")
  set (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fuse-ld=gold")
  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=gold")
endif()
if(MIR_USE_LD MATCHES "lld")
  set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=lld")
  set (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fuse-ld=lld")
  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
endif()
if(MIR_USE_LD MATCHES "mold")
  set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=mold")
  set (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fuse-ld=mold")
  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=mold")
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
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address)
elseif(cmake_build_type_lower MATCHES "threadsanitizer")
  add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
  add_link_options(-fsanitize=thread)
elseif(cmake_build_type_lower MATCHES "ubsanitizer")
  add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=undefined)
  # "Symbol already defined" errors occur with pre-compiled headers
  SET(MIR_USE_PRECOMPILED_HEADERS OFF CACHE BOOL "Use precompiled headers" FORCE)
  # We have inline classes as interfaces used by .so's and implemented elsewhere.
  # This results in multiple trivial implementations. So let's not warn about that.
  add_compile_options(-fno-sanitize=vptr)
else()
  # AddressSanitizer builds fail if we disallow undefined symbols
  add_link_options(-Wl,--no-undefined)
endif()

enable_testing()
find_package(PkgConfig)

# Check for boost
find_package(Boost 1.48.0 COMPONENTS system program_options filesystem iostreams REQUIRED)

add_compile_definitions(EGL_NO_X11)

pkg_check_modules(WAYLAND_EGLSTREAM wayland-eglstream IMPORTED_TARGET)

# Set the default platforms based on availability of NVIDIA libraries
if (WAYLAND_EGLSTREAM_FOUND)
  set(
    MIR_PLATFORM
    gbm-kms;atomic-kms;x11;eglstream-kms;wayland
    CACHE
    STRING
    "a list of graphics backends to build (options are 'gbm-kms', 'atomic-kms', 'x11', 'eglstream-kms', or 'wayland')"
  )
else()
  set(
    MIR_PLATFORM
    gbm-kms;atomic-kms;x11;wayland
    CACHE
    STRING
    "a list of graphics backends to build (options are 'gbm-kms', 'atomic-kms', 'x11', 'eglstream-kms', or 'wayland')"
  )
endif()

list(GET MIR_PLATFORM 0 MIR_TEST_PLATFORM)

option(MIR_ENABLE_TESTS "Build tests" ON)
CMAKE_DEPENDENT_OPTION(
  MIR_ENABLE_WLCS_TESTS "Also Build WLCS tests" ON
  "MIR_ENABLE_TESTS" OFF
)
CMAKE_DEPENDENT_OPTION(
  MIR_RUN_WLCS_TESTS "Run WLCS tests as part of testsuite" ON
  "MIR_ENABLE_WLCS_TESTS" OFF
)
CMAKE_DEPENDENT_OPTION(
  MIR_SIGBUS_HANDLER_ENVIRONMENT_BROKEN "Expect WLCS BadBuffer tests to fail" OFF
  "MIR_RUN_WLCS_TESTS" OFF
)

pkg_check_modules(APPARMOR IMPORTED_TARGET libapparmor)

CMAKE_DEPENDENT_OPTION(
  MIR_USE_APPARMOR "Whether or not Mir should build with support for AppArmor" ON
  "APPARMOR_FOUND" OFF
)
if (MIR_USE_APPARMOR)
  add_compile_definitions(MIR_USE_APPARMOR)
endif()

SET(MIR_EXCLUDE_TESTS "" CACHE STRING "Semicolon-separated list of tests to exclude (wildcards accepted)")


foreach(platform IN LISTS MIR_PLATFORM)
  if (platform STREQUAL "gbm-kms")
    set(MIR_BUILD_PLATFORM_GBM_KMS TRUE)
  endif()
  if (platform STREQUAL "atomic-kms")
    set(MIR_BUILD_PLATFORM_ATOMIC_KMS TRUE)
  endif()
  if (platform STREQUAL "x11")
     set(MIR_BUILD_PLATFORM_X11 TRUE)
  endif()
  if (platform STREQUAL "eglstream-kms")
     if (NOT WAYLAND_EGLSTREAM_FOUND)
        message(FATAL_ERROR "EGLStream platform requested, but required NVIDIA external EGL platform nvidia-egl-wayland not found")
     endif()
     set(MIR_BUILD_PLATFORM_EGLSTREAM_KMS TRUE)
  endif()
  if (platform STREQUAL "wayland")
     set(MIR_BUILD_PLATFORM_WAYLAND TRUE)
  endif()
endforeach(platform)

pkg_check_modules(GLM glm)
if(NOT GLM_FOUND)
  find_package(glm REQUIRED)  # There's no glm.pc on Fedora, but find_package(glm) fails on Ubuntu...
endif()
pkg_check_modules(DRM REQUIRED IMPORTED_TARGET libdrm)
pkg_check_modules(EGL REQUIRED IMPORTED_TARGET egl)
pkg_check_modules(EPOXY REQUIRED IMPORTED_TARGET epoxy)
pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0 gio-unix-2.0)
pkg_check_modules(GLESv2 REQUIRED IMPORTED_TARGET glesv2)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
pkg_check_modules(LIBINPUT REQUIRED IMPORTED_TARGET libinput>=1.1)
pkg_check_modules(LTTNG_UST REQUIRED IMPORTED_TARGET lttng-ust>=2.9)
pkg_check_modules(UDEV REQUIRED IMPORTED_TARGET libudev)
pkg_check_modules(UUID REQUIRED IMPORTED_TARGET uuid)
pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client)
pkg_check_modules(WAYLAND_SERVER REQUIRED IMPORTED_TARGET wayland-server)
pkg_check_modules(X11_XCURSOR REQUIRED IMPORTED_TARGET xcursor)
pkg_check_modules(XCB REQUIRED IMPORTED_TARGET xcb)
pkg_check_modules(XCB_COMPOSITE REQUIRED IMPORTED_TARGET xcb-composite)
pkg_check_modules(XCB_RANDR REQUIRED IMPORTED_TARGET xcb-randr)
pkg_check_modules(XCB_RENDER REQUIRED IMPORTED_TARGET xcb-render)
pkg_check_modules(XCB_XFIXES REQUIRED IMPORTED_TARGET xcb-xfixes)

include(CheckCXXSymbolExists)
list(APPEND CMAKE_REQUIRED_INCLUDES ${DRM_INCLUDE_DIRS})
list(APPEND CMAKE_REQUIRED_LIBRARIES ${DRM_LIBRARIES})
include_directories(SYSTEM ${DRM_INCLUDE_DIRS})

# Add USDT hooks to all LTTNG tracepoints
#
# This might be gnarly; this tweaks an LTTNG header to enable its
# USDT integration. The integration only involves #include-ing a
# SystemTap header and calling a macro from it in the LTTNG macro,
# but there aren't guarantees this won't change in future.
include(CheckIncludeFile)
CHECK_INCLUDE_FILE(sys/sdt.h HAVE_SYS_SDT_H)
if (HAVE_SYS_SDT_H)
  add_compile_definitions(LTTNG_UST_HAVE_SDT_INTEGRATION)
endif()

if (MIR_BUILD_PLATFORM_GBM_KMS OR MIR_BUILD_PLATFORM_ATOMIC_KMS)
  pkg_check_modules(GBM REQUIRED IMPORTED_TARGET gbm>=11.0.0)
endif()

set(MIR_TRACEPOINT_LIB_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/mir/tools)

mir_make_pkgconfig_variable(PKGCONFIG_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
mir_make_pkgconfig_variable(PKGCONFIG_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}")
include(JoinPaths)

set(MIR_GENERATED_INCLUDE_DIRECTORIES)
add_subdirectory(src/)
include_directories(${MIR_GENERATED_INCLUDE_DIRECTORIES})

add_subdirectory(examples/)
add_subdirectory(guides/)
add_subdirectory(cmake/)

if (MIR_ENABLE_TESTS)
  find_package(GtestGmock REQUIRED)
  pkg_check_modules(LIBEVDEV REQUIRED IMPORTED_TARGET libevdev)
  include_directories(${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR})
  add_subdirectory(tests/)

  # There's no nice way to format this. Thanks CMake.
  mir_add_test(NAME LGPL-required
    COMMAND /bin/sh -c "! grep -rl --exclude-dir=protocol --exclude=org_kde* 'GNU General' ${PROJECT_SOURCE_DIR}/src/core ${PROJECT_SOURCE_DIR}/include/core ${PROJECT_SOURCE_DIR}/src/common ${PROJECT_SOURCE_DIR}/include/common ${PROJECT_SOURCE_DIR}/src/include/common ${PROJECT_SOURCE_DIR}/src/platform ${PROJECT_SOURCE_DIR}/include/platform ${PROJECT_SOURCE_DIR}/src/include/platform ${PROJECT_SOURCE_DIR}/src/platforms ${PROJECT_SOURCE_DIR}/include/platforms ${PROJECT_SOURCE_DIR}/src/include/platforms ${PROJECT_SOURCE_DIR}/src/renderers ${PROJECT_SOURCE_DIR}/include/renderers ${PROJECT_SOURCE_DIR}/src/include/renderers"
  )
  mir_add_test(NAME GPL-required
    COMMAND /bin/sh -c "! grep -rl --exclude-dir=protocol --exclude=org_kde* 'GNU Lesser' ${PROJECT_SOURCE_DIR}/src/server ${PROJECT_SOURCE_DIR}/include/server ${PROJECT_SOURCE_DIR}/src/include/server ${PROJECT_SOURCE_DIR}/tests ${PROJECT_SOURCE_DIR}/examples"
  )

  if (IS_DIRECTORY "${PROJECT_SOURCE_DIR}/debian")
    mir_add_test(NAME package-abis
      COMMAND /bin/sh -c "cd ${PROJECT_SOURCE_DIR} && tools/update_package_abis.sh --check --verbose")
  endif ()
endif ()

enable_coverage_report(mirserver)

add_custom_target(ptest
    COMMAND "${CMAKE_SOURCE_DIR}/tools/run_ctests.sh" "--cost-file" "${CMAKE_BINARY_DIR}/ptest_ctest_cost_data.txt" "sh ${CMAKE_BINARY_DIR}/discover_all_tests.sh" "--" "$$ARGS"
    )

add_custom_target(release-checks)

mir_check_no_unreleased_symbols(mircommon release-checks)
mir_check_no_unreleased_symbols(mirplatform release-checks)
mir_check_no_unreleased_symbols(mirserver release-checks)

if (TARGET doc)
  add_custom_target(doc-show
          xdg-open ${CMAKE_BINARY_DIR}/doc/sphinx/_build/index.html
          DEPENDS doc)
endif()
