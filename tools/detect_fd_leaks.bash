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

    # Ignore fd leaks from UdevEnvironment code, since they are only caused by the
    # umockdev library and we cannot avoid or work around them.
    local has_udev=$(echo "${1}" | grep "mir_test_framework::UdevEnvironment::add_standard_device")
    if [ -n "$has_udev" ];
    then
        echo "ignore"
        return
    fi

    # Ignore fd leaks from ~GSourceHandle; when this is called after the
    # GMainLoop has been torn down the the glib-internal eventfd isn't
    # cleaned up.
    local from_source_handle_destructor=$(echo "${1}" | grep "mir::detail::GSourceHandle::~GSourceHandle")
    if [ -n "$from_source_handle_destructor" ];
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

    while read LINE;
    do
        if [ -n "$openfd" ];
        then
            local leak_status=$(process_fd_leak_backtrace_line "${LINE}")
            case "$leak_status" in
                "noleak")
                    openfd=""
                    ;;
                "ignore")
                    echo "[ WARNING ] FD leak was detected and ignored"
                    openfd=""
                    ;;
                "leak")
                    echo "[ FAILED ] FD leak was detected"
                    exit_rc=1
                    openfd=""
                    ;;
            esac
        else
            openfd=$(echo ${LINE} | grep "Open file descriptor\|Open \w\+ socket")
        fi
        echo ${LINE}
    done

    exit $exit_rc
}

$@ 2>&1 | detect_fd_leaks
