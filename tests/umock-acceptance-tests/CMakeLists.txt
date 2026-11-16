include(CMakeDependentOption)

mir_add_wrapped_executable(mir_umock_acceptance_tests NOINSTALL

  test_libinput.cpp
  )

target_include_directories(mir_umock_acceptance_tests
  PRIVATE
    ${CMAKE_SOURCE_DIR}/tests/include/
)

add_dependencies(mir_umock_acceptance_tests GMock)

target_link_libraries(mir_umock_acceptance_tests
  mir-test-assist
  mir-test-framework-static
  mir-test-doubles-static
  mir-umock-test-framework

  PkgConfig::UMOCKDEV
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

# Earlier umockdev releases lose device emulation on dup()ed fds, which the
# input platforms rely on when opening devices. Only in version 0.20 was this
# support added: https://github.com/martinpitt/umockdev/releases/tag/0.20.0
if (UMOCKDEV_VERSION VERSION_GREATER_EQUAL 0.20)
  target_sources(mir_umock_acceptance_tests PRIVATE test_evdev_recordings.cpp)
  target_link_libraries(mir_umock_acceptance_tests mir-recorded-input-test-framework)
else()
  message(STATUS "umockdev ${UMOCKDEV_VERSION} < 0.20: skipping evdev recording tests")
endif()

CMAKE_DEPENDENT_OPTION(
  MIR_RUN_ACCEPTANCE_TESTS
  "Run acceptance tests as part of default testing"
  ON
  "MIR_BUILD_ACCEPTANCE_TESTS"
  OFF)

if (MIR_RUN_ACCEPTANCE_TESTS)
  mir_discover_tests_with_fd_leak_detection(mir_umock_acceptance_tests LD_PRELOAD=libumockdev-preload.so.0 G_SLICE=always-malloc G_DEBUG=gc-friendly)
endif (MIR_RUN_ACCEPTANCE_TESTS)
