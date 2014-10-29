include(CMakeDependentOption)

include_directories(
  ${CMAKE_SOURCE_DIR}
)

add_definitions(
  -DMIR_PLATFORM_DRIVER_BINARY="${MIR_PLATFORM_DRIVER_BINARY}"
  -DMIR_CLIENT_DRIVER_BINARY="${MIR_CLIENT_DRIVER_BINARY}"
)

set(
  SOURCES

  server_configuration_wrapping.cpp
  clients.cpp
  test_client_library.cpp
  test_client_library_old.cpp
  test_client_surface_events.cpp
  test_client_surface_swap_buffers.cpp
  test_command_line_handling.cpp
  test_custom_input_dispatcher.cpp
  test_client_surfaces.cpp
  test_test_framework.cpp
  test_focus_selection.cpp
  test_server_shutdown.cpp
  test_client_focus_notification.cpp
  test_client_authorization.cpp
  test_shell_control_of_surface_configuration.cpp
  test_nested_mir.cpp
  test_display_configuration.cpp
  test_surfaces_with_output_id.cpp
  test_server_disconnect.cpp
  test_client_library_drm.cpp
  test_prompt_session_client_api.cpp
  test_client_screencast.cpp
  test_client_cursor_api.cpp
  test_large_messages.cpp
  test_client_surface_visibility.cpp
  test_touchspot_visualization.cpp
  test_client_with_custom_display_config_deadlock.cpp
  test_macros.cpp
  test_server_without_active_outputs.cpp
  test_client_input.cpp
  test_server_startup.cpp
  test_debug_api.cpp
  test_unresponsive_client.cpp
)

if (MIR_TEST_PLATFORM STREQUAL "mesa")
  list(APPEND SOURCES test_symbols_required_by_mesa.cpp)
endif()

add_executable(
  mir_acceptance_tests

  ${SOURCES}
)

mir_precompiled_header(mir_acceptance_tests ${CMAKE_CURRENT_SOURCE_DIR}/precompiled.hpp)

add_dependencies(mir_acceptance_tests GMock)

uses_android_input(mir_acceptance_tests)


target_link_libraries(
  mir_acceptance_tests

  mirserver
  mirclient
  mirclient-debug-extension
  mirplatform

  mir-test
  mir-test-framework
  mir-test-doubles
  mir-test-doubles-platform
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

install(
    TARGETS mir_acceptance_tests
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
