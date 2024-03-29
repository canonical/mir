add_subdirectory("server")

mir_add_wrapped_executable(mir_unit_tests_eglstream-kms NOINSTALL
  ${EGLSTREAM_KMS_UNIT_TEST_SOURCES}
  $<TARGET_OBJECTS:mir-test-doubles-udev>
  ${MIR_PLATFORM_OBJECTS}
  ${MIR_SERVER_OBJECTS}
)

target_link_libraries(
  mir_unit_tests_eglstream-kms

  mir-test-static
  mir-test-framework-static
  mir-test-doubles-static
  mir-test-doubles-platform-static

  server_platform_common
  PkgConfig::EPOXY
  PkgConfig::WAYLAND_EGLSTREAM
)

if (MIR_RUN_UNIT_TESTS)
  mir_discover_tests_with_fd_leak_detection(mir_unit_tests_eglstream-kms)
endif (MIR_RUN_UNIT_TESTS)
