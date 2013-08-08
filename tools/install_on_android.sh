#!/bin/bash
# build script for Mir on android arm devices
# script assumes that the Mir dependencies are already set up on the device.
# if they are not set up, see the setup-android-dependencies.sh script
# test run requires package 'android-tools-adb'
# test also assumes that the device is rooted and accessible over the adb bridge
#
# todo: this script should become part of the 'make install'/'make test' system
#

set -e

if [ -z ${1} ]; then
    BUILD_DIR=build-android-arm
else
    BUILD_DIR=${1}
fi

pushd ${BUILD_DIR} > /dev/null 
    #
    # Upload and run the tests!
    # Requires: https://wiki.canonical.com/ProductStrategyTeam/Android/Deploy
    #
    RUN_DIR=/tmp/mirtest

    adb wait-for-device
    adb root
    adb wait-for-device
    adb shell mkdir -p ${RUN_DIR}

    for x in bin/acceptance-tests \
             bin/integration-tests \
             bin/unit-tests \
             lib/libmirclient.so.* \
             lib/libmirprotobuf.so.* \
             lib/libmirplatform.so \
             lib/libmirplatformgraphics.so \
             lib/libmirserver.so.*
    do
        adb push $x ${RUN_DIR}
    done

    echo "cd ${RUN_DIR};
        export GTEST_OUTPUT=xml:./;
        export LD_LIBRARY_PATH=.;
        ./unit-tests;
        ./integration-tests;
        ./acceptance-tests;
        exit;
        exit" | adb shell

    adb pull "${RUN_DIR}/acceptance-tests.xml"
    adb pull "${RUN_DIR}/integration-tests.xml"
    adb pull "${RUN_DIR}/unit-tests.xml"

popd ${BUILD_DIR} > /dev/null 
