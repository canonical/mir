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

    for x in bin/mir_acceptance_tests \
             bin/mir_integration_tests \
             bin/mir_unit_tests \
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
        ./mir_unit_tests;
        ./mir_integration_tests;
        ./mir_acceptance_tests;
        exit;
        exit" | adb shell

    adb pull "${RUN_DIR}/mir_acceptance_tests.xml"
    adb pull "${RUN_DIR}/mir_integration_tests.xml"
    adb pull "${RUN_DIR}/mir_unit_tests.xml"

popd ${BUILD_DIR} > /dev/null 
