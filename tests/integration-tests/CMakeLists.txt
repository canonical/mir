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

set(
  INTEGRATION_TESTS_SRCS
  test_custom_input_dispatcher.cpp
  test_touchspot_visualization.cpp
  test_surface_stack_with_compositor.cpp
  test_display_server_main_loop_events.cpp
  test_server_client_types.cpp
)

add_subdirectory(compositor/)
add_subdirectory(process/)
add_subdirectory(input/)

link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
mir_add_wrapped_executable(mir_integration_tests NOINSTALL
  ${INTEGRATION_TESTS_SRCS}
)

if (MIR_USE_PRECOMPILED_HEADERS)
  target_precompile_headers(
    mir_integration_tests
    PRIVATE
      <gmock/gmock.h>
      <gtest/gtest.h>
      <vector>
      <string>
      <exception>
      <memory>
  )
endif()

add_dependencies(mir_integration_tests GMock)

target_link_libraries(
  mir_integration_tests

  mir-test-static
  mir-test-framework-static
  mir-test-doubles-static

  mircommon

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

if (MIR_TEST_PLATFORM STREQUAL "gbm-kms")
target_link_libraries(mir_integration_tests
  mirsharedgbmservercommon-static
  PkgConfig::GBM
  PkgConfig::DRM
)
endif()

CMAKE_DEPENDENT_OPTION(
  MIR_RUN_INTEGRATION_TESTS
  "Run integration tests as part of default testing"
  ON
  "MIR_BUILD_INTEGRATION_TESTS"
  OFF)

if (MIR_RUN_INTEGRATION_TESTS)
  mir_discover_tests_with_fd_leak_detection(mir_integration_tests)
endif (MIR_RUN_INTEGRATION_TESTS)
