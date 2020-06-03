#!/bin/sh

print_help_and_exit()
{
    local prog=$(basename $0)

    echo "Usage: $prog [OPTION]... [--] cmds"
    echo "Discover tests from a binary that uses GTest and emit CTest entries for them"
    echo ""
    echo "      --test-name TESTNAME          the base test name, if not specified"
    echo "                                    the basename of the gtest executable is used"
    echo "      --gtest-executable EXECUTABLE the command to run to execute the GTest tests to discover"
    echo "                                    if not specified, the last element of \`cmds\` is used."
    echo "      --env=ENV                     add ENV to the environment of the tests"
    echo "  -h, --help                        display this help and exit"
    echo ""
    echo "Example: $prog path/to/mir_unit_tests"
    echo "Example: $prog --env LD_PRELOAD=p.so -- valgrind --trace-children=yes path/to/mir_unit_tests"
    echo "Example: $prog --gtest-executable path/to/mir_unit_tests -- path/to/mir_unit_tests make_explode_with_delight"

    exit 0
}

add_env()
{
    env="$env ENVIRONMENT \"$1\""
}

add_cmd()
{
    cmd="$cmd \"$1\""
}

add_filter()
{
    filter="$filter $1"
}

while [ $# -gt 0 ];
do
    case "$1" in
        --test-name) shift; testname="$1";;
        --env) shift; add_env "$1";;
        --help|-h) print_help_and_exit;;
	--gtest-executable) shift; test_binary="$1";;
        --) shift; break;;
        --*) print_help_and_exit;;
        *) break;;
    esac
    shift
done

while [ $# -gt 0 ];
do
    case "$1" in
        --gtest_filter*) add_filter "$1";;
        --*) ;;
        *) last_cmd="$1";;
    esac
    add_cmd $1
    shift
done

if [ -z "$test_binary" ];
then
    test_binary=$last_cmd
fi

if [ -z "$testname" ];
then
    testname=$(basename $test_binary)
fi

tests=$($test_binary --gtest_list_tests $filter | grep -v '^ ' | cut -d' ' -f1 | grep '\.' | sed 's/$/*/')

for t in $tests;
do
    echo "add_test($testname.$t $cmd \"--gtest_filter=$t\")"
    echo "set_tests_properties($testname.$t PROPERTIES $env)"
done
