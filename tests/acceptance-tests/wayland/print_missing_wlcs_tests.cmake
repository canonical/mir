separate_arguments(TESTS_TO_QUERY)
string(REPLACE ";" ":" TESTS_TO_QUERY_STR "${TESTS_TO_QUERY}")

foreach (test_name IN LISTS TESTS_TO_QUERY)
  execute_process(
    COMMAND
      ${WLCS_BINARY} ${WLCS_INTEGRATION} --gtest_list_tests --gtest_filter=${TESTS_TO_QUERY_STR}
    RESULT_VARIABLE
      wlcs_success
    OUTPUT_VARIABLE
      wlcs_output
  )

  if (wlcs_success)
    message(FATAL_ERROR "Failed to run WLCS to query tests: ${wlcs_success}")
  endif()

  string(REPLACE "." ";" split_test_name ${test_name})
  list(GET split_test_name 0 test_suite)
  list(GET split_test_name 1 test_name)

  if (NOT ((wlcs_output MATCHES ".*${test_suite}.*") AND (wlcs_output MATCHES ".*${test_name}.*")))
    message("Test ${test_suite}.${test_name} does not exist in WLCS")
  endif()
endforeach()
