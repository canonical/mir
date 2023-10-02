mir_add_wrapped_executable(mir_unit_tests_virtual NOINSTALL
  ${CMAKE_CURRENT_SOURCE_DIR}/test_platform.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_display.cpp
  $<TARGET_OBJECTS:mirplatformvirtual>
  $<TARGET_OBJECTS:mirnullreport>  # Sub-optimal. We really want to link a lib
)

set_property(
  SOURCE test_platform.cpp
  PROPERTY COMPILE_OPTIONS -Wno-variadic-macros)

add_dependencies(mir_unit_tests_virtual GMock)

set_target_properties(
  mir_unit_tests_virtual
  PROPERTIES
    ENABLE_EXPORTS TRUE
)

target_link_libraries(
  mir_unit_tests_virtual

  mir-test-static
  mir-test-doubles-static
  mir-test-doubles-platform-static
  mir-test-framework-static
  server_platform_common
)

if (MIR_RUN_UNIT_TESTS)
  mir_discover_tests_with_fd_leak_detection(mir_unit_tests_virtual G_SLICE=always-malloc G_DEBUG=gc-friendly)
endif (MIR_RUN_UNIT_TESTS)
