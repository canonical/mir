mir_add_wrapped_executable(mir_unit_tests_mesa-kms NOINSTALL
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gbm_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_allocator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_platform.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_graphics_platform.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_display.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_display_generic.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_display_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_display_multi_monitor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_display_configuration.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_real_kms_output.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_kms_page_flipper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_cursor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_bypass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_nested_authentication.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_drm_helper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_ipc_operations.cpp
  ${MIR_SERVER_OBJECTS}
  $<TARGET_OBJECTS:mirplatformgraphicsmesakmsobjects>
  $<TARGET_OBJECTS:mir-umock-test-framework>
)

set_property(
  SOURCE test_gbm_buffer.cpp test_platform.cpp test_graphics_platform.cpp test_buffer_allocator.cpp
         test_display.cpp test_display_generic.cpp test_display_multi_monitor.cpp test_display_configuration.cpp
         test_display_buffer.cpp test_drm_helper.cpp
  PROPERTY COMPILE_OPTIONS -Wno-variadic-macros)

add_dependencies(mir_unit_tests_mesa-kms GMock)

target_link_libraries(
  mir_unit_tests_mesa-kms

  mir-test-static
  mir-test-framework-static
  mir-test-doubles-static
  mir-test-doubles-platform-static
  mirsharedmesaservercommon-static

  server_platform_common

  ${DRM_LDFLAGS} ${DRM_LIBRARIES}
)

if (MIR_RUN_UNIT_TESTS)
  mir_discover_tests_with_fd_leak_detection(mir_unit_tests_mesa-kms LD_PRELOAD=libumockdev-preload.so.0 G_SLICE=always-malloc G_DEBUG=gc-friendly)
endif (MIR_RUN_UNIT_TESTS)
