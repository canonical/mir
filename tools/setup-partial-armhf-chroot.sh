#!/bin/bash

set -e

if [ -z $1 ]; then
    echo "please supply directory to create partial chroot in. (eg, ./setup-partial-armhf-chroot.sh mychroot-dir)"
    exit
fi

echo "creating phablet-compatible raring armhf partial chroot for mir compiles in directory ${1}"

if [ ! -d ${1} ]; then
    mkdir -p ${1} 
fi

download_and_extract_packages()
{
    declare -a PACKAGES=$1[@]
    local ARCHITECTURE=$2

    for i in ${!PACKAGES}; do

        PACKAGE_VERSION=`apt-cache show ${i}:${ARCHITECTURE} | grep Version | awk -F: '{print $NF}' | sed "s/ //g"`
        PACKAGE_FILENAME="${i}_${PACKAGE_VERSION}_${ARCHITECTURE}.deb"

        if [ ! -f ${PACKAGE_FILENAME} ]; then
            echo "Downloading mir dependency: ${i}"
            apt-get download "${i}:${ARCHITECTURE}"
        else
            echo "already downloaded: ${PACKAGE_FILENAME}"
        fi

        #quick sanity check
        if [ ! -f ${PACKAGE_FILENAME} ]; then
            echo "error: did not download expected file (${PACKAGE_FILENAME}. script is malformed!"; exit 1        
        fi

        echo "Extracting: ${PACKAGE_FILENAME}"
        dpkg -x ${PACKAGE_FILENAME} . 
    done
}

pushd ${1} > /dev/null

    declare -a PACKAGES_ARMHF=(
        libboost1.49-dev
        libboost-chrono1.49-dev
        libboost-chrono1.49-dev
        libboost-date-time1.49-dev
        libboost-filesystem1.49-dev
        libboost-program-options1.49-dev
        libprotobuf-dev
        libboost-chrono1.49.0
        libboost-date-time1.49.0
        libboost-filesystem1.49.0
        libboost-system1.49.0
        libboost-system1.49-dev
        libboost-thread1.49-dev
        libboost-thread1.49.0
        libboost-regex1.49-dev
        libboost-regex1.49.0
        libboost-program-options1.49.0
        libhybris
        libhybris-dev
        libgflags2
        libgflags-dev
        libgoogle-glog-dev
        libgoogle-glog0
        libicu48
        libprotobuf7
        libprotobuf-dev
        libegl1-mesa-dev
        libgles2-mesa-dev
        libxkbcommon0
        libxkbcommon-dev
        zlib1g)

    declare -a PACKAGES_ALL=(libglm-dev)

    #cleanup
    for i in * ; do
        if [[ -d ${i} ]]; then 
            echo "removing directory: ./${i}"
            rm -rf ./${i}
        fi
    done

    download_and_extract_packages PACKAGES_ARMHF armhf
    download_and_extract_packages PACKAGES_ALL all

    #todo: we get egl/gles headers from the mesa packages, but should be pointing at the hybris libraries
    #just rewrite the symlinks for now
    rm ./usr/lib/arm-linux-gnueabihf/libEGL.so 
    rm ./usr/lib/arm-linux-gnueabihf/libGLESv2.so 
    ln -s libhybris-egl/libEGL.so.1 ./usr/lib/arm-linux-gnueabihf/libEGL.so 
    ln -s libhybris-egl/libGLESv2.so.2 ./usr/lib/arm-linux-gnueabihf/libGLESv2.so
popd > /dev/null 
