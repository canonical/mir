add_executable(
  mir_discover_gtest_tests
  EXCLUDE_FROM_ALL
  ${CMAKE_CURRENT_SOURCE_DIR}/mir_discover_gtest_tests.cpp)

set_target_properties(
  mir_discover_gtest_tests PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/mir_gtest
)

add_executable(
  mir_test_memory_error
  EXCLUDE_FROM_ALL
  ${CMAKE_CURRENT_SOURCE_DIR}/mir_test_memory_error.cpp)

set_target_properties(
  mir_test_memory_error PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/mir_gtest
)

file(INSTALL ${CMAKE_CURRENT_SOURCE_DIR}/fail_on_success.sh
  DESTINATION ${CMAKE_BINARY_DIR}/mir_gtest
  USE_SOURCE_PERMISSIONS
)
