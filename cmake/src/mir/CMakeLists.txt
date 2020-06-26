add_executable(
  mir_test_memory_error
  EXCLUDE_FROM_ALL
  ${CMAKE_CURRENT_SOURCE_DIR}/mir_test_memory_error.cpp)

set_target_properties(
  mir_test_memory_error PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/mir_gtest
)

add_executable(
  mir_test_fd_leak
  EXCLUDE_FROM_ALL
  ${CMAKE_CURRENT_SOURCE_DIR}/mir_test_fd_leak.cpp)

set_target_properties(
  mir_test_fd_leak PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/mir_gtest
)

file(INSTALL ${CMAKE_CURRENT_SOURCE_DIR}/fail_on_success.sh
  DESTINATION ${CMAKE_BINARY_DIR}/mir_gtest
  USE_SOURCE_PERMISSIONS
)

file(INSTALL ${CMAKE_CURRENT_SOURCE_DIR}/xfail_if_gtest_exists.sh
  DESTINATION ${CMAKE_BINARY_DIR}/mir_gtest
  USE_SOURCE_PERMISSIONS
)
