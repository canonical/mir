mir_add_wrapped_executable(mir_unit_tests_mesa-x11 NOINSTALL
  ${CMAKE_CURRENT_SOURCE_DIR}/test_platform.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_graphics_platform.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_display.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_display_generic.cpp
  $<TARGET_OBJECTS:mirplatformservermesax11sharedresources>
  $<TARGET_OBJECTS:mirplatformgraphicsmesax11objects>
  $<TARGET_OBJECTS:mir-umock-test-framework>
  $<TARGET_OBJECTS:mirnullreport>  # Sub-optimal. We really want to link a lib
)

set_property(
  SOURCE test_platform.cpp test_graphics_platform.cpp test_display_generic.cpp
  PROPERTY COMPILE_OPTIONS -Wno-variadic-macros)

add_dependencies(mir_unit_tests_mesa-x11 GMock)

target_link_libraries(
  mir_unit_tests_mesa-x11

  mir-test-static
  mir-test-doubles-static
  mir-test-doubles-platform-static
  mir-test-framework-static
  mirsharedmesaservercommon-static
  server_platform_common
)

if (MIR_RUN_UNIT_TESTS)
  mir_discover_tests_with_fd_leak_detection(mir_unit_tests_mesa-x11 LD_PRELOAD=libumockdev-preload.so.0 G_SLICE=always-malloc G_DEBUG=gc-friendly)
endif (MIR_RUN_UNIT_TESTS)
