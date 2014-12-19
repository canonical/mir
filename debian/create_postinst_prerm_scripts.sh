#!/bin/sh

set -e

dpkg_alternatives_priority=500
deb_host_arch=$1
deb_host_multiarch=$2

mir_platform_types="${PLATFORM_DRIVER}"
case $deb_host_arch in
    arm64)
        mir_platforms="mesa"
        ;;
    *)
        mir_platforms="android mesa"
        ;;
esac

create_script()
{
    local script=$1
    local platform_type=$2
    local platform=$3

    sed -e "s/@DEB_HOST_MULTIARCH@/$deb_host_multiarch/" \
        -e "s/@MIR_PLATFORM_TYPE@/$platform_type/" \
        -e "s/@MIR_PLATFORM@/$platform/" \
        -e "s/@DPKG_ALTERNATIVES_PRIORITY@/$dpkg_alternatives_priority/" \
        debian/update-alternatives.${script}.in > debian/libmir$platform_type-$platform.${script}
}

for platform_type in $mir_platform_types;
do
    for platform in $mir_platforms;
    do
        create_script postinst $platform_type $platform
        create_script prerm $platform_type $platform
    done
done
