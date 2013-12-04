#!/bin/bash

set -e

if [ -z $1 ]; then
    echo "please supply directory to create partial chroot in. (eg, ./setup-partial-armhf-chroot.sh mychroot-dir)"
    exit
fi

echo "creating phablet-compatible armhf partial chroot for mir compiles in directory ${1}"

if [ ! -d ${1} ]; then
    mkdir -p ${1} 
fi

pushd ${1} > /dev/null

# TODO: Parse the build-dependencies out of debian/control
    BUILD_DEPENDS=google-mock,\
libboost1.54-dev,\
libboost-chrono1.54-dev,\
libboost-chrono1.54-dev,\
libboost-date-time1.54-dev,\
libboost-filesystem1.54-dev,\
libboost-program-options1.54-dev,\
libboost-system1.54-dev,\
libboost-thread1.54-dev,\
libboost-regex1.54-dev,\
libhardware-dev,\
libgflags-dev,\
libgoogle-glog-dev,\
libprotobuf-dev,\
libegl1-mesa-dev,\
libgles2-mesa-dev,\
libxkbcommon-dev,\
libumockdev-dev,\
liblttng-ust-dev,\
systemtap-sdt-dev,\
android-platform-headers,\
libglm-dev

    #cleanup
    for i in * ; do
        if [[ -d ${i} ]]; then 
            echo "removing directory: ./${i}"
            rm -rf ./${i}
        fi
    done

    fakeroot debootstrap --include=$BUILD_DEPENDS --arch=armhf --download-only --variant=buildd trusty .

    for I in var/cache/apt/archives/* ; do
	if [ ! -d $I ] ; then
	    echo "unpacking: $I"
	    dpkg -x $I .
	fi
    done

popd > /dev/null 
