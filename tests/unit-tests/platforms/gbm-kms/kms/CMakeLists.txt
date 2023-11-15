mir_add_wrapped_executable(mir_unit_tests_gbm-kms NOINSTALL
  ${CMAKE_CURRENT_SOURCE_DIR}/test_platform.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_display_generic.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_display_sink.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_display_multi_monitor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_display_configuration.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_real_kms_output.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_kms_page_flipper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_cursor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_bypass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_drm_helper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_quirks.cpp
  ${MIR_SERVER_OBJECTS}
  $<TARGET_OBJECTS:mirplatformgraphicsgbmkmsobjects>
  $<TARGET_OBJECTS:mir-umock-test-framework>
)

set_property(
  SOURCE test_platform.cpp test_graphics_platform.cpp test_buffer_allocator.cpp
         test_display.cpp test_display_generic.cpp test_display_multi_monitor.cpp test_display_configuration.cpp
         test_display_sink.cpp test_drm_helper.cpp
  PROPERTY COMPILE_OPTIONS -Wno-variadic-macros)

add_dependencies(mir_unit_tests_gbm-kms GMock)

set_target_properties(
  mir_unit_tests_gbm-kms
  PROPERTIES
    ENABLE_EXPORTS TRUE
)

target_link_libraries(
  mir_unit_tests_gbm-kms

  mir-test-static
  mir-test-framework-static
  mir-test-doubles-static
  mir-test-doubles-platform-static
  mirsharedgbmservercommon-static

  PkgConfig::DRM
  PkgConfig::GBM
)

if (MIR_RUN_UNIT_TESTS)
  mir_discover_tests_with_fd_leak_detection(mir_unit_tests_gbm-kms LD_PRELOAD=libumockdev-preload.so.0 G_SLICE=always-malloc G_DEBUG=gc-friendly)
endif (MIR_RUN_UNIT_TESTS)
