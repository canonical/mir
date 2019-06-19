list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_linux_virtual_terminal.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_minimal_console_services.cpp
)

if (MIR_BUILD_INTERPROCESS_TESTS)
  list(APPEND UNIT_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test_logind_console_services.cpp)
endif()

set(UNIT_TEST_SOURCES

  ${UNIT_TEST_SOURCES}
  PARENT_SCOPE
)
