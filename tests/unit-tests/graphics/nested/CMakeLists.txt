list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_nested_display_configuration.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_nested_display.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_nested_display_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mir_display_configuration_builder.cpp
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
