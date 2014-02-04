list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/message_processor_report.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_legacy_input_report.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_display_report.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_compositor_report.cpp
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
