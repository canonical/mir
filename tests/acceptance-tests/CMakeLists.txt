include(CMakeDependentOption)

if (MIR_ENABLE_WLCS_TESTS)
  add_subdirectory(wayland)
endif()

add_subdirectory(platforms)

set(
  SOURCES

  # Catch double-free bugs by wrapping close() and abort()ing on EBADF
  strict_close.cpp

  server_configuration_wrapping.cpp
  server_configuration_options.cpp
  server_stop_callback.cpp
  test_command_line_handling.cpp
  test_custom_input_dispatcher.cpp
  test_input_device_hub.cpp
  test_seat_report.cpp
)

mir_add_wrapped_executable(mir_acceptance_tests NOINSTALL

  ${SOURCES}
)

target_link_libraries(
  mir_acceptance_tests

  mirplatformtest
)

if (MIR_USE_PRECOMPILED_HEADERS)
  target_precompile_headers(
    mir_acceptance_tests
    PRIVATE
      <gmock/gmock.h>
      <gtest/gtest.h>
      <vector>
      <string>
      <exception>
      <memory>
  )
endif()

add_dependencies(mir_acceptance_tests GMock)

target_link_libraries(mir_acceptance_tests
  mir-test-assist
  mir-test-doubles-platform-static

  mirserver

  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

CMAKE_DEPENDENT_OPTION(
  MIR_RUN_ACCEPTANCE_TESTS
  "Run acceptance tests as part of default testing"
  ON
  "MIR_BUILD_ACCEPTANCE_TESTS"
  OFF)

if (MIR_RUN_ACCEPTANCE_TESTS)
  mir_discover_tests_with_fd_leak_detection(mir_acceptance_tests)
endif (MIR_RUN_ACCEPTANCE_TESTS)
