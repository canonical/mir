include(CMakeDependentOption)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/src/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/cookie
  ${PROJECT_SOURCE_DIR}/src/include/common
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/src/include/client
  ${PROJECT_SOURCE_DIR}/src/include/gl
  ${PROJECT_SOURCE_DIR}/include/renderers/sw
)

set(WINDOW_MANAGEMENT_TESTS_SRCS
  test_minimal_window_manager.cpp
)

link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
mir_add_wrapped_executable(mir_window_management_tests NOINSTALL
  ${WINDOW_MANAGEMENT_TESTS_SRCS}
  ${MIR_SERVER_OBJECTS}
  ${MIR_PLATFORM_OBJECTS}
)

if (MIR_USE_PRECOMPILED_HEADERS)
  target_precompile_headers(
    mir_window_management_tests
    PRIVATE
    <gmock/gmock.h>
    <gtest/gtest.h>
    <vector>
    <string>
    <exception>
    <memory>
  )
endif()

add_dependencies(mir_window_management_tests GMock)

target_link_libraries(
  mir_window_management_tests

  mir-test-static
  mir-test-framework-static
  mir-test-doubles-static

  mircommon
  miral
  miral-internal

  ${MIR_PLATFORM_REFERENCES}
  ${MIR_SERVER_REFERENCES}
  Boost::system

  # GBM platform dependencies
  PkgConfig::DRM
  # Shared platform dependencies
  PkgConfig::EGL
  PkgConfig::GLESv2
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)


CMAKE_DEPENDENT_OPTION(
  MIR_RUN_WINDOW_MANAGEMENT_TESTS
  "Run window management tests as part of default testing"
  ON
  "MIR_BUILD_WINDOW_MANAGEMENT_TESTS"
  OFF)

if (MIR_RUN_WINDOW_MANAGEMENT_TESTS)
  mir_discover_tests_with_fd_leak_detection(mir_window_management_tests)
endif (MIR_RUN_WINDOW_MANAGEMENT_TESTS)
