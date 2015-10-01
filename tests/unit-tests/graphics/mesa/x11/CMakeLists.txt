mir_add_wrapped_executable(
  mir_unit_tests_mesa-x11
  ${CMAKE_CURRENT_SOURCE_DIR}/test_platform.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_graphics_platform.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_guest_platform.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_display.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_display_generic.cpp
  ${PROJECT_SOURCE_DIR}/src/platforms/mesa/server/x11/X11_resources.cpp
  ${MIR_SERVER_OBJECTS}
  ${MIR_PLATFORM_OBJECTS}
  ${MIR_COMMON_OBJECTS}
  $<TARGET_OBJECTS:mirplatformgraphicsmesax11objects>
)

add_dependencies(mir_unit_tests_mesa-x11 GMock)

target_link_libraries(
  mir_unit_tests_mesa-x11

  mir-test-static
  mir-test-framework-static
  mir-test-doubles-static
  mir-test-doubles-platform-static
  mirsharedmesaservercommon-static
)

if (MIR_RUN_UNIT_TESTS)
  mir_discover_tests_with_fd_leak_detection(mir_unit_tests_mesa-x11 LD_PRELOAD=libumockdev-preload.so.0 G_SLICE=always-malloc G_DEBUG=gc-friendly)
endif (MIR_RUN_UNIT_TESTS)