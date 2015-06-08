#!/bin/bash
# build script to compile Mir for armhf devices
#
set -e

usage() {
  echo "usage: $(basename $0) [-c] [-u]"
  echo "-c clean before building"
  echo "-u update partial chroot directory"
  echo "-h this message"
}

clean_build_dir() {
    rm -rf ${1}
    mkdir ${1}
}

BUILD_DIR=build-android-arm
NUM_JOBS=$(( $(grep -c ^processor /proc/cpuinfo) + 1 ))
_do_update_chroot=0

while getopts "cuh" OPTNAME
do
    case $OPTNAME in
      c )
        clean_build_dir ${BUILD_DIR}
        shift
        ;;
      u )
        _do_update_chroot=1
        shift
        ;;
      h )
        usage
        exit 0
        ;;
      * )
        echo "invalid option specified"
        usage
        exit 1
        ;;
    esac
done


if [ "${MIR_NDK_PATH}" = "" ]; then
    export MIR_NDK_PATH=~/.cache/mir-armhf-chroot
fi

if [ ! -d ${MIR_NDK_PATH} ]; then 
    echo "no partial chroot dir detected. attempting to create one"
    _do_update_chroot=1
fi

if [ ! -d ${BUILD_DIR} ]; then 
    mkdir ${BUILD_DIR}
fi

if [ ${_do_update_chroot} -eq 1 ] ; then
    pushd tools > /dev/null
        ./setup-partial-armhf-chroot.sh ${MIR_NDK_PATH}
    popd > /dev/null
    # force a clean build after an update, since CMake cache maybe out of date
    clean_build_dir ${BUILD_DIR}
fi

echo "Using MIR_NDK_PATH: ${MIR_NDK_PATH}"

pushd ${BUILD_DIR} > /dev/null 

    export PKG_CONFIG_PATH="${MIR_NDK_PATH}/usr/lib/pkgconfig:${MIR_NDK_PATH}/usr/lib/arm-linux-gnueabihf/pkgconfig"
    export PKG_CONFIG_ALLOW_SYSTEM_CFLAGS=1
    export PKG_CONFIG_ALLOW_SYSTEM_LIBS=1
    export PKG_CONFIG_SYSROOT_DIR=$MIR_NDK_PATH
    export PKG_CONFIG_EXECUTABLE=`which pkg-config`
    echo "Using PKG_CONFIG_PATH: $PKG_CONFIG_PATH"
    echo "Using PKG_CONFIG_EXECUTABLE: $PKG_CONFIG_EXECUTABLE"
    cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/LinuxCrossCompile.cmake \
      -DBoost_COMPILER=-gcc \
      -DMIR_PLATFORM=android\;mesa-kms \
      .. 

    make -j${NUM_JOBS} $@

popd > /dev/null 
