list(
  APPEND INTEGRATION_TESTS_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/test_configuring_input_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_cursor_observer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_single_seat_setup.cpp
)

set(
  INTEGRATION_TESTS_SRCS
  ${INTEGRATION_TESTS_SRCS}
  PARENT_SCOPE)
