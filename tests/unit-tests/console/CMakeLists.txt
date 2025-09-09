list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_linux_virtual_terminal.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_minimal_console_services.cpp
)

if (MIR_BUILD_INTERPROCESS_TESTS)
  list(APPEND UNIT_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test_logind_console_services.cpp)

  find_program(PYTHON_EXECUTABLE python3 REQUIRED)

  execute_process(
      COMMAND ${PYTHON_EXECUTABLE} -c "import dbusmock"
      RESULT_VARIABLE EXIT_CODE
  )

  if (NOT ${EXIT_CODE} EQUAL 0)
      message(FATAL_ERROR "The \"dbusmock\" module is required for logind integration tests.")
  endif()
endif()

set(UNIT_TEST_SOURCES

  ${UNIT_TEST_SOURCES}
  PARENT_SCOPE
)
