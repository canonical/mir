add_subdirectory("server")
add_subdirectory("client")

mir_add_wrapped_executable(mir_unit_tests_eglstream-kms NOINSTALL
  ${EGLSTREAM_KMS_UNIT_TEST_SOURCES}
  $<TARGET_OBJECTS:mir-test-doubles-udev>
  ${MIR_PLATFORM_OBJECTS}
  ${MIR_SERVER_OBJECTS}
)

target_link_libraries(
  mir_unit_tests_eglstream-kms

  mirclient-static
  mirclientlttng-static
  mirclient-static
  mirclientlttng-static

  mir-test-static
  mir-test-framework-static
  mir-test-doubles-static
  mir-test-doubles-platform-static

  client_platform_common
  server_platform_common
  ${EPOXY_LDFLAGS} ${EPOXY_LIBRARIES}
)

if (MIR_RUN_UNIT_TESTS)
  mir_discover_tests_with_fd_leak_detection(mir_unit_tests_eglstream-kms)
endif (MIR_RUN_UNIT_TESTS)
