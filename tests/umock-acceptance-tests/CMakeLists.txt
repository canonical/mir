include(CMakeDependentOption)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/tests/include/
  ${CMAKE_SOURCE_DIR}/include/cookie
  ${UMOCKDEV_INCLUDE_DIRS}
)

mir_add_wrapped_executable(
  mir_umock_acceptance_tests

  test_libinput.cpp
  $<TARGET_OBJECTS:mir-umock-test-framework>
  )

add_dependencies(mir_umock_acceptance_tests GMock)

target_link_libraries(mir_umock_acceptance_tests
  mir-test-assist
  mir-test-framework-static
  mir-test-doubles-static

  ${UMOCKDEV_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

CMAKE_DEPENDENT_OPTION(
  MIR_RUN_ACCEPTANCE_TESTS
  "Run acceptance tests as part of default testing"
  ON
  "MIR_BUILD_ACCEPTANCE_TESTS"
  OFF)

if (MIR_RUN_ACCEPTANCE_TESTS)
  mir_discover_tests_with_fd_leak_detection(mir_umock_acceptance_tests LD_PRELOAD=libumockdev-preload.so.0 G_SLICE=always-malloc G_DEBUG=gc-friendly)
endif (MIR_RUN_ACCEPTANCE_TESTS)
