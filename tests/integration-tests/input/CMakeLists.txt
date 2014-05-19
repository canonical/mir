list(
  APPEND INTEGRATION_TESTS_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/test_configuring_input_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_nested_input.cpp
)

if(NOT MIR_TEST_PLATFORM STREQUAL "android")
  add_subdirectory(android)
endif()

set(
  INTEGRATION_TESTS_SRCS
  ${INTEGRATION_TESTS_SRCS}
  PARENT_SCOPE)
