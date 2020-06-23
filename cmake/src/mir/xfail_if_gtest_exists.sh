#!/bin/sh

set -e

# Usage: BINARY TEST_SPECIFICATION [OTHER_ARGS]

gtest_binary=$1
shift
test_spec=$1
shift
other_args=$@

matching_test=$(${gtest_binary} --gtest_filter=${test_spec} --gtest_list_tests ${other_args})

print_success_and_exit()
{
    echo "[XFAIL]: Expected failure"
    exit 0
}

if [ -n "${matching_test}" ]
then
    ${gtest_binary} --gtest_filter=${test_spec} ${other_args} || print_success_and_exit
    # Unexpected success
    echo "[XPASS]: Test unexpectedly passes"
    exit 1
else
    echo "[SKIP] ${test_spec}: Not implemented in ${gtest_binary}"
fi

exit 0
