add_subdirectory(android)

list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_event_filter_chain.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_input_surface_info.cpp
)

set(
  UNIT_TEST_SOURCES
  ${UNIT_TEST_SOURCES}
  PARENT_SCOPE)
