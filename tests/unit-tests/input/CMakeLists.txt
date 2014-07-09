add_subdirectory(android)

list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_event_filter_chain.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_display_input_region.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_cursor_controller.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_xcursor_loader.cpp
)

set(
  UNIT_TEST_SOURCES
  ${UNIT_TEST_SOURCES}
  PARENT_SCOPE)
