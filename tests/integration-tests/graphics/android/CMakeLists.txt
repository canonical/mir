list(
  APPEND INTEGRATION_TESTS_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_integration.cpp
#  ${CMAKE_CURRENT_SOURCE_DIR}/test_display_integration.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_internal_client.cpp
)

set(
  INTEGRATION_TESTS_SRCS
  ${INTEGRATION_TESTS_SRCS}
  PARENT_SCOPE)
