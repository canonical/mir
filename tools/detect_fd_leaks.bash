#!/bin/bash

set -o pipefail

process_fd_leak_backtrace_line()
{
    local inherited=$(echo "${1}" | grep "<inherited from parent>")
    if [ -n "$inherited" ];
    then
        echo "noleak"
        return
    fi

    # Ignore fd leaks from g_dbus code, since they are only caused by the
    # umockdev library and we cannot avoid or work around them.
    local has_dbus=$(echo "${1}" | grep "g_dbus")
    if [ -n "$has_dbus" ];
    then
        echo "ignore"
        return
    fi

    local still_in_backtrace=$(echo "${1}" | grep "by 0x\|at 0x")
    if [ -n "$still_in_backtrace" ];
    then
        echo "unsure"
    else
        echo "leak"
    fi
}

detect_fd_leaks()
{
    local exit_rc=0
    local openfd=""
    local test_process_exiting=false

    while read LINE;
    do
        if [[ ${LINE} =~ ^\[==========\]\ [0-9]+\ test.* ]];
        then
            test_process_exiting=true
        fi

        if [ -n "$openfd" ];
        then
            if [ "$test_process_exiting" == true ];
            then
                local leak_status=$(process_fd_leak_backtrace_line "${LINE}")
                case "$leak_status" in
                    "noleak")
                        ;;
                    "ignore")
                        echo "[ WARNING ] FD leak was detected and ignored"
                        ;;
                    "leak")
                        echo "[ FAILED ] FD leak was detected"
                        exit_rc=1
                        ;;
                    "unsure")
                        continue
                        ;;
                esac
            fi
        fi
        openfd=$(echo ${LINE} | grep "Open file descriptor\|Open \w\+ socket")
        echo ${LINE}
    done

    exit $exit_rc
}

$@ 2>&1 | detect_fd_leaks
