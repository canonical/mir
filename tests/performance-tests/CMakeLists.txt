set(
  PERFORMANCE_TESTS_SOURCES

  test_glmark2-es2-mir.cpp
)

find_program(GLMARK2_ES2_MIR_FOUND glmark2-es2-mir)

if(NOT GLMARK2_ES2_MIR_FOUND)
  message("warning: glmark2-es2-mir not found, performance tests _may_ fail.")
endif()

add_executable(
  mir_performance_tests

  ${PERFORMANCE_TESTS_SOURCES}
)

target_link_libraries(
  mir_performance_tests

  mirserver
  mirclient
  mirplatform

  mir-test
  mir-test-framework
  mir-test-doubles
  mir-test-doubles-platform
  3rd_party

  ${PROTOBUF_LIBRARIES}
  ${Boost_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

install(
  TARGETS mir_performance_tests
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
