list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_logind_console_services.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_linux_virtual_terminal.cpp
)

set(UNIT_TEST_SOURCES

  ${UNIT_TEST_SOURCES}
  PARENT_SCOPE
)
