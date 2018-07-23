include_directories (${LIBEVDEV_INCLUDE_DIRS})

set(
  PRIVILEGED_TESTS_SOURCES

  test_input_events.cpp
  ui_get_sysname_ioctl_override.cpp
)

mir_add_wrapped_executable(mir_privileged_tests NOINSTALL

  ${PRIVILEGED_TESTS_SOURCES}
)

add_dependencies(mir_privileged_tests GMock)

target_link_libraries(
  mir_privileged_tests

  ${GTEST_BOTH_LIBRARIES}
  ${LIBEVDEV_LDFLAGS} ${LIBEVDEV_LIBRARIES}

  mir-test-static
  mir-test-framework-static
  mirclient
)

