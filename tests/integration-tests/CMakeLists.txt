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

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/include/mir_test_cucumber)

set(
  INTEGRATION_TESTS_SRCS
  test_renderloop.cpp
  test_surfaceloop.cpp
  test_error_reporting.cpp
  test_display_info.cpp
)

add_subdirectory(client/)
add_subdirectory(compositor/)
add_subdirectory(frontend/)
add_subdirectory(process/)
if (NOT MIR_DISABLE_INPUT)
add_subdirectory(input/)
endif()

if (MIR_PLATFORM STREQUAL "android")
  add_subdirectory(graphics/android)
endif()

if(MIR_PLATFORM STREQUAL "gbm")
  include_directories(
    ${DRM_INCLUDE_DIRS}
    ${GBM_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
  )
  add_subdirectory(graphics/gbm)
  list(APPEND INTEGRATION_TESTS_SRCS test_drm_auth_magic.cpp)
endif()

add_subdirectory(cucumber)

add_executable(
  integration-tests
  ${INTEGRATION_TESTS_SRCS})

target_link_libraries(
  integration-tests

  mir-test-framework
  mir-test-doubles
  mir-test-cucumber

  mirserver
  mirclient
  mirdraw
  mirtestdraw

  3rd_party

  ${PROTOBUF_LIBRARIES}
  ${Boost_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
  ${ANDROID_STDLIB}
)

mir_discover_tests(integration-tests)
