mir_add_wrapped_executable(mir_unit_tests_nested NOINSTALL
  ${CMAKE_CURRENT_SOURCE_DIR}/test_nested_display_configuration.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_nested_display.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mir_display_configuration_builder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_nested_cursor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_nested_display_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_ipc_operations.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_host_buffer.cpp
  $<TARGET_OBJECTS:mir-test-doubles-udev>
  ${MIR_PLATFORM_OBJECTS}
  ${MIR_SERVER_OBJECTS}
)

target_link_libraries(
  mir_unit_tests_nested
  mir-test-static
  mir-test-framework-static
  mir-test-doubles-static
  mir-test-doubles-platform-static

  client_platform_common
  server_platform_common
  mirclient-static
  mirclientlttng-static
)

if (MIR_RUN_UNIT_TESTS)
  mir_discover_tests_with_fd_leak_detection(mir_unit_tests_nested)
endif (MIR_RUN_UNIT_TESTS)
