string(REPLACE "-fno-lto" "" NO_NO_LTO_FLAGS ${CMAKE_C_FLAGS})
set(CMAKE_C_FLAGS ${NO_NO_LTO_FLAGS})
string(REPLACE "-fno-lto" "" NO_NO_LTO_FLAGS ${CMAKE_CXX_FLAGS})
set(CMAKE_CXX_FLAGS ${NO_NO_LTO_FLAGS})

mir_add_wrapped_executable(mir_examples_tests NOINSTALL
  test_floating_window_manager.cpp)

target_include_directories(mir_examples_tests
  PRIVATE
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/tests/include
  ${PROJECT_SOURCE_DIR}/include/test
  ${PROJECT_SOURCE_DIR}/src/include/common
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${GMOCK_INCLUDE_DIR}
  ${GTEST_INCLUDE_DIR})

target_link_libraries(mir_examples_tests
  mir-test-static
  mir-public-test-framework
  mir-test-framework-static
  mir-test-doubles-static
  mircommon
  miral
  example-shell-lib
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARIES})

CMAKE_DEPENDENT_OPTION(
  MIR_RUN_EXAMPLES_TESTS
  "Run tests on examples as part of default testing"
  ON
  "MIR_RUN_EXAMPLES_TESTS"
  OFF)

if (MIR_RUN_EXAMPLES_TESTS)
  mir_discover_tests_with_fd_leak_detection(mir_examples_tests)
endif (MIR_RUN_EXAMPLES_TESTS)
