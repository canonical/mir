list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_platform.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_display.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_display_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_display_multi_monitor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_display_configuration.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_real_kms_output.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_kms_page_flipper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_linux_virtual_terminal.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_cursor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_guest_platform.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_bypass.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_nested_authentication.cpp
  $<TARGET_OBJECTS:mirplatformgraphicsmesakmsobjects>
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
