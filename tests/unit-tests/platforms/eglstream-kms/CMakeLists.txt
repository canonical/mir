add_subdirectory("server")

mir_add_wrapped_executable(mir_unit_tests_eglstream-kms NOINSTALL
  ${EGLSTREAM_KMS_UNIT_TEST_SOURCES}
)

target_link_libraries(
  mir_unit_tests_eglstream-kms

  ${EGLSTREAM_KMS_UNIT_TEST_REFERENCES}
  mir-test-doubles-udev
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
