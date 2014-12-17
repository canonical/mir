include(CMakeDependentOption)

include_directories(
  ${CMAKE_SOURCE_DIR}
)

add_definitions(
  -DMIR_PLATFORM_DRIVER_BINARY="${MIR_PLATFORM_DRIVER_BINARY}"
)

set(
  SOURCES

  server_configuration_wrapping.cpp
  server_configuration_options.cpp
  server_signal_handling.cpp
  test_client_header_version.cpp
  test_client_library.cpp
  test_client_library_errors.cpp
  test_client_library_old.cpp
  test_client_surface_events.cpp
  test_client_surface_swap_buffers.cpp
  test_command_line_handling.cpp
  test_client_surfaces.cpp
  test_server_shutdown.cpp
  test_client_focus_notification.cpp
  test_client_authorization.cpp
  test_nested_mir.cpp
  test_display_configuration.cpp
  test_surfaces_with_output_id.cpp
  test_server_disconnect.cpp
  test_client_library_drm.cpp
  test_prompt_session_client_api.cpp
  test_client_screencast.cpp
  test_client_surface_visibility.cpp
  test_client_with_custom_display_config_deadlock.cpp
  test_server_without_active_outputs.cpp
  test_server_startup.cpp
  test_debug_api.cpp
  test_unresponsive_client.cpp
  test_client_platform_operation.cpp
)

if (MIR_TEST_PLATFORM STREQUAL "mesa")
  list(APPEND SOURCES test_symbols_required_by_mesa.cpp)
endif()

add_subdirectory(throwback)

mir_add_wrapped_executable(
  mir_acceptance_tests

  ${SOURCES}
  $<TARGET_OBJECTS:acceptance-test-throwback>
)

mir_precompiled_header(mir_acceptance_tests ${CMAKE_CURRENT_SOURCE_DIR}/precompiled.hpp)

add_dependencies(mir_acceptance_tests GMock)

target_link_libraries(
  mir_acceptance_tests

  mirserver
  mirclient
  mirclient-debug-extension
  mirplatform

  mir-test
  mir-test-doubles
  mir-test-doubles-platform
  mir-test-framework
  3rd_party

  ${PROTOBUF_LIBRARIES}
  ${Boost_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

CMAKE_DEPENDENT_OPTION(
  MIR_RUN_ACCEPTANCE_TESTS
  "Run acceptance tests as part of default testing"
  ON
  "MIR_BUILD_ACCEPTANCE_TESTS"
  OFF)

if (MIR_RUN_ACCEPTANCE_TESTS)
  mir_discover_tests(mir_acceptance_tests)
endif (MIR_RUN_ACCEPTANCE_TESTS)
