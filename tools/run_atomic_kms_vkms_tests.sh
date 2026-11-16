#!/bin/bash
set -eu

if (( EUID != 0 )); then
    if ! sudo --non-interactive -- true; then
        echo "atomic-kms VKMS tests require passwordless sudo inside the workshop" >&2
        exit 1
    fi
    exec sudo --non-interactive -- "$0" "$@"
fi

if (( $# == 0 )); then
    echo "usage: $0 <wrapped-gtest-command> [arguments...]" >&2
    exit 2
fi

exec {lock_fd}>/run/lock/mir-atomic-kms-vkms-tests.lock
flock --exclusive "${lock_fd}"

if ! mountpoint --quiet /sys/kernel/config; then
    mount --types configfs configfs /sys/kernel/config
fi

if [[ ! -d /sys/kernel/config/vkms ]]; then
    modprobe vkms || true
fi

if [[ ! -d /sys/kernel/config/vkms ]]; then
    echo "atomic-kms VKMS tests require Linux 6.19 VKMS configfs topology and live connector-status support" >&2
    exit 1
fi

exec "$@"
