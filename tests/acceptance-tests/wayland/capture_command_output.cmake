string(REPLACE ";" ":" TESTS_TO_QUERY_STR "${TESTS_TO_QUERY}")
  
execute_process(
  COMMAND
    ${WLCS_BINARY} ${WLCS_INTEGRATION} --gtest_list_tests --gtest_filter=${TESTS_TO_QUERY_STR}
  RESULT_VARIABLE
    wlcs_success
  OUTPUT_VARIABLE
    wlcs_output
)

if (NOT wlcs_success)
  error("Failed to run WLCS to query tests")
endif()

foreach (test_name IN LISTS TESTS_TO_QUERY)
  if (NOT (wlcs MATCHES ".*${test_name}.*"))
    message("Test ${test_name} does not exist in WLCS")
  endif()
endforeach()
