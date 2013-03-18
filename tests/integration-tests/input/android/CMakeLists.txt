list(
  APPEND INTEGRATION_TESTS_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/test_fake_event_hub_to_event_filter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_input_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_cursor_listener.cpp
)

set(
  INTEGRATION_TESTS_SRCS
  ${INTEGRATION_TESTS_SRCS}
  PARENT_SCOPE)
