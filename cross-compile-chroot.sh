#!/bin/bash
# build script to compile Mir for armhf devices
#
set -e

usage() {
  echo "usage: $(basename $0) [-c] [-u] [-d <dist>]"
  echo "  -c         Clean before building"
  echo "  -d <dist>  Select the distribution to build for"
  echo "  -u         Update partial chroot directory"
  echo "  -h         This message"
}

clean_build_dir() {
    rm -rf ${1}
    mkdir ${1}
}

# Default to a dist-agnostic directory name so as to not break Jenkins right now
BUILD_DIR=build-android-arm
NUM_JOBS=$(( $(grep -c ^processor /proc/cpuinfo) + 1 ))
_do_update_chroot=0

# Default to vivid as we don't seem to have any working wily devices right now 
dist=vivid
clean=0

while getopts "cuhd:" OPTNAME
do
    case $OPTNAME in
      c )
        clean=1
        ;;
      d )
        dist=${OPTARG}
        BUILD_DIR=${BUILD_DIR}-${dist}
        ;;
      u )
        _do_update_chroot=1
        ;;
      h )
        usage
        exit 0
        ;;
      : )
        echo "Parameter -${OPTARG} needs an argument"
        usage
        exit 1;
        ;;
      * )
        echo "invalid option specified"
        usage
        exit 1
        ;;
    esac
done

shift $((${OPTIND}-1))

if [ ${clean} -ne 0 ]; then
    clean_build_dir ${BUILD_DIR}
fi

if [ "${MIR_NDK_PATH}" = "" ]; then
    export MIR_NDK_PATH=~/.cache/mir-armhf-chroot-${dist}
fi

if [ ! -d ${MIR_NDK_PATH} ]; then 
    echo "no partial chroot dir detected. attempting to create one"
    _do_update_chroot=1
fi

if [ ! -d ${BUILD_DIR} ]; then 
    mkdir ${BUILD_DIR}
fi

echo "Building for distro: $dist"
echo "Using MIR_NDK_PATH: ${MIR_NDK_PATH}"

if [ ${_do_update_chroot} -eq 1 ] ; then
    pushd tools > /dev/null
        ./setup-partial-armhf-chroot.sh ${MIR_NDK_PATH} ${dist}
    popd > /dev/null
    # force a clean build after an update, since CMake cache maybe out of date
    clean_build_dir ${BUILD_DIR}
fi

cc_variant=
if [ "${dist}" = "vivid" ]; then
    cc_variant=-4.9
fi

pushd ${BUILD_DIR} > /dev/null 

    export PKG_CONFIG_PATH="${MIR_NDK_PATH}/usr/lib/pkgconfig:${MIR_NDK_PATH}/usr/lib/arm-linux-gnueabihf/pkgconfig"
    export PKG_CONFIG_ALLOW_SYSTEM_CFLAGS=1
    export PKG_CONFIG_ALLOW_SYSTEM_LIBS=1
    export PKG_CONFIG_SYSROOT_DIR=$MIR_NDK_PATH
    export PKG_CONFIG_EXECUTABLE=`which pkg-config`
    echo "Using PKG_CONFIG_PATH: $PKG_CONFIG_PATH"
    echo "Using PKG_CONFIG_EXECUTABLE: $PKG_CONFIG_EXECUTABLE"
    cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/LinuxCrossCompile.cmake \
      -DCC_VARIANT=${cc_variant} \
      -DMIR_PLATFORM=android\;mesa-kms \
      .. 

    make -j${NUM_JOBS} $@

popd > /dev/null 
