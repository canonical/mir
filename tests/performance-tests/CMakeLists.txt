set(
  PERFORMANCE_TESTS_SOURCES

  test_glmark2-es2-mir.cpp
)

add_executable(
  mir_performance_tests

  ${PERFORMANCE_TESTS_SOURCES}
)

add_dependencies(mir_performance_tests GMock)

target_link_libraries(
  mir_performance_tests

  mir-test
  mir-test-framework
  mirserver

  ${GTEST_BOTH_LIBRARIES}
)

install(
  TARGETS mir_performance_tests
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
