list(
  APPEND INTEGRATION_TESTS_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_integration.cpp
  $<TARGET_OBJECTS:mirplatformgraphicskmsobjects>
)

set(
  INTEGRATION_TESTS_SRCS
  ${INTEGRATION_TESTS_SRCS}
  PARENT_SCOPE
)
