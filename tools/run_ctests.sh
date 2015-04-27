#!/bin/sh

print_help_and_exit()
{
    local prog=$(basename $0)

    echo "Usage: $prog [OPTION]... DISCOVERY_COMMAND [--] [CTEST_OPTION]..."
    echo "Run CTests produced by a discovery command"
    echo ""
    echo "      --cost-file=FILE  cost file to use to allow CTest to optimize runtime"
    echo "  -h, --help            display this help and exit"

    exit 0
}

TEST_DIR=$(mktemp -d --tmpdir mir-ctests-XXXXX);
trap "if [ -d \"$TEST_DIR\" ]; then rm -r \"$TEST_DIR\"; fi" INT TERM EXIT

while [ $# -gt 0 ];
do
    case "$1" in
        --cost-file) shift; cost_file=$1;;
        --help|-h) print_help_and_exit;;
        --) shift; break;;
        --*) print_help_and_exit;;
        *) discovery_cmd=$1;;
    esac
    shift
done

if [ -z "$discovery_cmd" ];
then
    print_help_and_exit
fi

# Create test list
$discovery_cmd > "$TEST_DIR/CTestTestfile.cmake"

# Populate test directory to enable CTest
touch "$TEST_DIR/CTestConfig.cmake"
echo "ctest_start(ptest \"$TEST_DIR\" \"$TEST_DIR\")" >> "$TEST_DIR/steer.cmake"
echo "ctest_test(BUILD \"$TEST_DIR\")" >> "$TEST_DIR/steer.cmake"

# Use provided cost file
run_cost_file="$TEST_DIR/Testing/Temporary/CTestCostData.txt"
mkdir -p "$TEST_DIR/Testing/Temporary/"
if [ -f "$cost_file" ];
then
    cp "$cost_file" "$run_cost_file"
fi

# Run tests
ctest -S "$TEST_DIR/steer.cmake" --output-on-failure -j8 -V $@

# Update cost file
if [ -n "$cost_file" ];
then
    cp "$run_cost_file" "$cost_file"
fi
