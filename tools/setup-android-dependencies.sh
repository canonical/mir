#!/bin/bash
# build script for Mir on android arm devices
# test run requires package 'android-tools-adb'
# test also assumes that the device is rooted and accessible over the adb bridge
#
# todo: this script should become part of the 'make install'/'make test' system
#

set -e

pushd /tmp > /dev/null
    rm -f libgoogle-glog0_0.3.2-4ubuntu1_armhf.deb
    rm -f libgflags2_2.0-1_armhf.deb 

    wget "https://launchpad.net/ubuntu/+archive/primary/+files/libgoogle-glog0_0.3.2-4ubuntu1_armhf.deb"
    wget "https://launchpad.net/ubuntu/+archive/primary/+files/libgflags2_2.0-1_armhf.deb"

    adb push libgoogle-glog0_0.3.2-4ubuntu1_armhf.deb /data/ubuntu/root
    adb push libgflags2_2.0-1_armhf.deb /data/ubuntu/root

    echo "ubuntu_chroot shell;
        cd /root;
        dpkg -i libgflags2_2.0-1_armhf.deb libgoogle-glog0_0.3.2-4ubuntu1_armhf.deb
        apt-get -y install libprotobuf7 \
                           libboost-system1.49.0 \
                           libboost-program-options1.49.0 \
                           libboost-thread1.49.0 \
                           libxkbcommon0

            exit;
            exit" | adb shell
popd > /dev/null
