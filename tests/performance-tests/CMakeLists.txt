set(
  PERFORMANCE_TESTS_SOURCES

  test_glmark2-es2-mir.cpp
)

mir_add_wrapped_executable(
  mir_performance_tests

  ${PERFORMANCE_TESTS_SOURCES}
)

add_dependencies(mir_performance_tests GMock)

target_link_libraries(
  mir_performance_tests

  mir-test-static
  mir-test-framework-static
  mir-test-doubles-static
  mirserver

  ${GTEST_BOTH_LIBRARIES}
)

