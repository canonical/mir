#!/bin/bash
# build script for Mir on android arm devices
# test run requires package 'android-tools-adb'
# test also assumes that the device is rooted and accessible over the adb bridge
#
# todo: this script should become part of the 'make install'/'make test' system
#

set -e

pushd /tmp > /dev/null
    echo "apt-get -y install libprotobuf7 \
                             libgoogle-glog0 \
                             libboost-program-options1.53.0 \
                             libboost-system1.53.0
          exit" | adb shell
popd > /dev/null
