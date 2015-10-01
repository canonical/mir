mir_add_wrapped_executable(
  mir_unit_tests_mesa-kms
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
  ${CMAKE_CURRENT_SOURCE_DIR}/test_linux_virtual_terminal.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_cursor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_guest_platform.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_bypass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_nested_authentication.cpp
  ${MIR_SERVER_OBJECTS}
  ${MIR_PLATFORM_OBJECTS}
  ${MIR_COMMON_OBJECTS}
  $<TARGET_OBJECTS:mirplatformgraphicsmesakmsobjects>
)

add_dependencies(mir_unit_tests_mesa-kms GMock)

target_link_libraries(
  mir_unit_tests_mesa-kms

  mirdraw
  demo-shell
  mircommon

  mirclient-static
  mirclientrpc-static
  mirclientlttng-static

  mir-test-static
  mir-test-framework-static
  mir-test-doubles-static
  mir-test-doubles-platform-static
  mirsharedmesaservercommon-static

  xcursorloader-static

  ${DRM_LDFLAGS} ${DRM_LIBRARIES}
  ${GBM_LDFLAGS} ${GBM_LIBRARIES}
  ${PROTOBUF_LITE_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${Boost_LIBRARIES}
  ${UMOCKDEV_LIBRARIES}
  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
  ${GLESv2_LDFLAGS} ${GLESv2_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
  ${MIR_PLATFORM_REFERENCES}
  ${MIR_COMMON_REFERENCES}
  ${MIR_SERVER_REFERENCES}
  ${XKBCOMMON_LIBRARIES}
)

if (MIR_RUN_UNIT_TESTS)
  mir_discover_tests_with_fd_leak_detection(mir_unit_tests_mesa-kms LD_PRELOAD=libumockdev-preload.so.0 G_SLICE=always-malloc G_DEBUG=gc-friendly)
endif (MIR_RUN_UNIT_TESTS)