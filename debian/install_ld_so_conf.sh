#!/bin/sh

set -e

mir_platform_types="platformgraphics clientplatform"
mir_platforms="android mesa"

DEB_HOST_MULTIARCH=$1

for platform_type in $mir_platform_types;
do
    for platform in $mir_platforms;
    do
        platform_dir="/usr/lib/$DEB_HOST_MULTIARCH/mir/$platform_type/$platform"
        package_dir="debian/libmir$platform_type-$platform/$platform_dir"
        echo "$platform_dir" > $package_dir/ld.so.conf
    done
done
