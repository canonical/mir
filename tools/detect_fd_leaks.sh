#!/bin/bash

set -o pipefail

detect_fd_leaks()
{
    local exit_rc=0
    local openfd=""

    while read LINE;
    do
        if [ -n "$openfd" ];
        then
            local inherited=$(echo ${LINE} | grep "<inherited from parent>")
            if [ -z "$inherited" ];
            then
                echo "[ FAILED ] FD leak was detected"
                exit_rc=1
            fi
        fi
        openfd=$(echo ${LINE} | grep "Open file descriptor")
        echo ${LINE}
    done

    exit $exit_rc
}

$@ 2>&1 | detect_fd_leaks
