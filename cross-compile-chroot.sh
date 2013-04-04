#!/bin/bash
# build script for Mir on android arm devices
#
set -e

BUILD_DIR=build-android-arm

if [ "$MIR_NDK_PATH" = "" ]; then
    export MIR_NDK_PATH=`pwd`/partial-armhf-chroot
    if [ ! -d ${MIR_NDK_PATH} ]; then 
        echo "no partial root specified or detected. attempting to create one"
        pushd tools > /dev/null
            ./setup-partial-armhf-chroot.sh ${MIR_NDK_PATH}
        popd > /dev/null
    fi
fi

echo "Using MIR_NDK_PATH: $MIR_NDK_PATH"

#start with a clean build every time
rm -rf ${BUILD_DIR}
mkdir ${BUILD_DIR}
pushd ${BUILD_DIR} > /dev/null 

    cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/LinuxCrossCompile.cmake \
      -DBoost_COMPILER=-gcc \
      -DMIR_ENABLE_DEATH_TESTS=NO \
      -DMIR_PLATFORM=android \
      .. 

    cmake --build .

popd ${BUILD_DIR} > /dev/null 
