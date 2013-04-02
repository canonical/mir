#!/bin/bash

set -e

if [ -z $1 ]; then
    echo "please supply directory to create partial chroot in. (eg, ./setup-partial-armhf-chroot.sh mychroot-dir)"
    exit
fi

echo "creating phablet-compatible quantal armhf partial chroot for mir compiles in directory ${1}"

if [ ! -d ${1} ]; then
    mkdir -p ${1} 
fi

pushd ${1} > /dev/null

    ARCHITECTURE=armhf

    declare -a PACKAGES=(
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
        libicu48
        libprotobuf7
        libprotobuf-dev
        libegl1-mesa-dev
        libgles2-mesa-dev
        zlib1g)

    #cleanup
    for i in * ; do
        if [[ -d ${i} ]]; then 
            echo "removing directory: ./${i}"
            rm -rf ./${i}
        fi
    done

    for i in ${PACKAGES[@]}; do

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

    #glog is only available in raring for armhf. we download the raring sources and 
    #install them manually for this quantal script
    if [ ! -f libgoogle-glog0_0.3.2-4ubuntu1_armhf.deb ]; then
        wget "https://launchpad.net/ubuntu/+archive/primary/+files/libgoogle-glog0_0.3.2-4ubuntu1_armhf.deb"
    fi

    if [ ! -f libgflags2_2.0-1_armhf.deb ]; then
        wget "https://launchpad.net/ubuntu/+archive/primary/+files/libgflags2_2.0-1_armhf.deb"
    fi

    if [ ! -f libgoogle-glog-dev_0.3.2-4ubuntu1_armhf.deb ]; then
        wget "http://launchpadlibrarian.net/134086853/libgoogle-glog-dev_0.3.2-4ubuntu1_armhf.deb"
    fi

    if [ ! -f libgflags-dev_2.0-1_armhf.deb ]; then
        wget "http://launchpadlibrarian.net/106868249/libgflags-dev_2.0-1_armhf.deb"
    fi

    dpkg -x libgflags-dev_2.0-1_armhf.deb .
    dpkg -x libgflags2_2.0-1_armhf.deb .
    dpkg -x libgoogle-glog0_0.3.2-4ubuntu1_armhf.deb .
    dpkg -x libgoogle-glog-dev_0.3.2-4ubuntu1_armhf.deb .

    #todo: we get egl/gles headers from the mesa packages, but should be pointing at the hybris libraries
    #just rewrite the symlinks for now
    rm ./usr/lib/arm-linux-gnueabihf/libEGL.so 
    rm ./usr/lib/arm-linux-gnueabihf/libGLESv2.so 
    ln -s libhybris-egl/libEGL.so.1 ./usr/lib/arm-linux-gnueabihf/libEGL.so 
    ln -s libhybris-egl/libGLESv2.so.2 ./usr/lib/arm-linux-gnueabihf/libGLESv2.so
popd > /dev/null 
