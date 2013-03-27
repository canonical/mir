#!/bin/bash
# build script for Mir on android arm devices
# test run requires package 'android-tools-adb'
#
# you should use this file if you have to do an integration test, or as 
# an example of how to build. this script will do more downloading than you 
# might want in a development workflow
#
set -e

BUILD_DIR=build-android-arm

#note:ndk-rewrite is just temporary until the real is pushed
if [ "$MIR_NDK_PATH" = "" ]; then
    export MIR_NDK_PATH=`pwd`/ndk-rewrite
fi

echo "Using MIR_NDK_PATH: $MIR_NDK_PATH"

if [[ -d ${MIR_NDK_PATH} ]]; then
    pushd ${MIR_NDK_PATH}
    bzr pull
    popd
else
    bzr branch lp:~kdub/mir/ndk-rewrite ${MIR_NDK_PATH}
fi

pushd ${MIR_NDK_PATH} > /dev/null
    ./generate-armhf-deps.sh 
popd > /dev/null

#start with a clean build every time
rm -rf ${BUILD_DIR}
mkdir ${BUILD_DIR}
pushd ${BUILD_DIR} > /dev/null 

    cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/LinuxCrossCompile.cmake \
      -DBoost_COMPILER=-gcc \
      -DMIR_ENABLE_DEATH_TESTS=NO \
      -DMIR_PLATFORM=android \
      -DMIR_DISABLE_INPUT=true \
      .. 

    cmake --build .

popd ${BUILD_DIR} > /dev/null 
