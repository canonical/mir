#!/bin/sh

set -e

DEB_HOST_ARCH=$1
DEB_HOST_MULTIARCH=$2

mir_platform_types="${PLATFORM_DRIVER} ${CLIENT_DRIVER}"
case $DEB_HOST_ARCH in
    arm64)
        mir_platforms="mesa"
        ;;
    *)
        mir_platforms="android mesa"
        ;;
esac

for platform_type in $mir_platform_types;
do
    for platform in $mir_platforms;
    do
        platform_dir="/usr/lib/$DEB_HOST_MULTIARCH/mir/$platform_type/$platform"
        package_dir="debian/libmir$platform_type-$platform/$platform_dir"
        echo "$platform_dir" > $package_dir/ld.so.conf
    done
done
