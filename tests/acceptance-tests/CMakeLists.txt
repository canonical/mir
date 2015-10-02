include(CMakeDependentOption)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include/cookie
)

set(
  SOURCES

  server_configuration_wrapping.cpp
  server_configuration_options.cpp
  server_signal_handling.cpp
  test_application_not_responding_detection.cpp
  test_client_header_version.cpp
  test_client_input.cpp
  test_client_library.cpp
  test_client_library_callbacks.cpp
  test_client_library_old.cpp
  test_client_surface_events.cpp
  test_client_surface_swap_buffers.cpp
  test_command_line_handling.cpp
  test_client_surfaces.cpp
  test_custom_window_management.cpp
  test_custom_input_dispatcher.cpp
  test_server_shutdown.cpp
  test_client_focus_notification.cpp
  test_client_authorization.cpp
  test_mirblob.cpp
  test_nested_mir.cpp
  test_nested_input.cpp
  test_display_configuration.cpp
  test_surfaces_with_output_id.cpp
  test_server_disconnect.cpp
  test_prompt_session_client_api.cpp
  test_client_scaling.cpp
  test_client_screencast.cpp
  test_client_surface_visibility.cpp
  test_buffer_stream_arrangement.cpp
  test_client_with_custom_display_config_deadlock.cpp
  test_server_without_active_outputs.cpp
  test_server_startup.cpp
  test_debug_api.cpp
  test_unresponsive_client.cpp
  test_client_platform_operation.cpp
  test_input_device_hub.cpp
  test_latency.cpp
  test_render_override.cpp
  test_surface_modifications.cpp
  test_surface_placement.cpp
  test_surface_morphing.cpp
  test_surface_specification.cpp
  test_system_compositor_window_manager.cpp
  test_session_mediator_report.cpp
)

if (MIR_TEST_PLATFORM STREQUAL "mesa-kms" OR MIR_TEST_PLATFORM STREQUAL "mesa-x11")
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

target_link_libraries(mir_acceptance_tests
  mir-test-assist

  mirclient
  mirclient-debug-extension
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
