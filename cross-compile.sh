#!/bin/sh
# build script for Mir on android arm devices
# set $MIR_ANDROID_NDK_DIR to the android toolchain noted in DEPENDENCIES
# test run requires package 'android-tools-adb' 
#
set -e

if [ ! -e $MIR_ANDROID_NDK_DIR ]; then
    echo "aborting: MIR_ANDROID_NDK_DIR not set. set to path containing mir NDK"
    exit
fi

export MIR_NDK_PATH=$MIR_ANDROID_NDK_DIR
BUILD_DIR=build-android-arm
if [ ! -e ${BUILD_DIR} ]; then
    mkdir ${BUILD_DIR}
    ( cd ${BUILD_DIR} &&
      cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/AndroidCrossCompile.cmake \
          -DBoost_COMPILER=-gcc \
          -DMIR_ENABLE_DEATH_TESTS=NO \
          -DMIR_INPUT_ENABLE_EVEMU=NO \
          -DMIR_PLATFORM=android \
          .. )
fi

cmake --build ${BUILD_DIR}

adb push ${BUILD_DIR}/lib/libmirclient.so.0.0.1 /data

adb push ${BUILD_DIR}/bin/acceptance-tests /data
adb shell 'cd /data && LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH GTEST_OUTPUT=xml:./ ./acceptance-tests'
adb pull '/data/acceptance-tests.xml'

adb push ${BUILD_DIR}/bin/integration-tests /data
adb shell 'cd /data && LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH GTEST_OUTPUT=xml:./ ./integration-tests'
adb pull '/data/integration-tests.xml'

adb push ${BUILD_DIR}/bin/unit-tests /data
adb shell 'cd /data && LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH GTEST_OUTPUT=xml:./ ./unit-tests'
adb pull '/data/unit-tests.xml'
