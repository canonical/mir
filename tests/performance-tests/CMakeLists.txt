set(
  PERFORMANCE_TESTS_SOURCES

  test_glmark2-es2-mir.cpp
)

add_executable(
  mir_performance_tests

  ${PERFORMANCE_TESTS_SOURCES}
)

target_link_libraries(
  mir_performance_tests
  mirserver

  ${Boost_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
)

install(
  TARGETS mir_performance_tests
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
