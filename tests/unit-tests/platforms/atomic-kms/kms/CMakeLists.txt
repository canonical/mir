mir_add_wrapped_executable(mir_unit_tests_atomic-kms NOINSTALL
  ${CMAKE_CURRENT_SOURCE_DIR}/test_cursor.cpp
)

add_dependencies(mir_unit_tests_atomic-kms GMock)

set_target_properties(
  mir_unit_tests_atomic-kms
  PROPERTIES
    ENABLE_EXPORTS TRUE
)

target_link_libraries(
  mir_unit_tests_atomic-kms

  mirplatformgraphicsatomickmsobjects
  mirsharedatomickmscommon-static
  mir-umock-test-framework
  mir-test-static
  mir-test-framework-static
  mir-test-doubles-static
  mir-test-doubles-platform-static
  mirsharedgbmservercommon-static

  PkgConfig::DRM
  PkgConfig::GBM
)

if (MIR_RUN_UNIT_TESTS)
  mir_discover_tests_with_fd_leak_detection(mir_unit_tests_atomic-kms LD_PRELOAD=libumockdev-preload.so.0 G_SLICE=always-malloc G_DEBUG=gc-friendly)
endif (MIR_RUN_UNIT_TESTS)
