#!/bin/sh

NUM_JOBS=$(( $(grep -c ^processor /proc/cpuinfo) + 1 ))

apt-get source mir
MIR_RELEASE_DIR=$(find -maxdepth 1 -name mir-\* -type d)
ln -sf ${MIR_RELEASE_DIR}/ mir-prev-release
cd ${MIR_RELEASE_DIR}

#NOTE: patch needed since mir 0.13.x fails to generate base dump for libmircommon
abi_check_file_fixed=$(grep -c common\/mir\/events cmake/ABICheck.cmake)
if [ "${abi_check_file_fixed}" -eq 0 ]; then
    sed -i 's/common\/mir\/input/&\n\$\{CMAKE_SOURCE_DIR\}\/src\/include\/common\/mir\/events/' cmake/ABICheck.cmake
fi

debian/rules override_dh_auto_configure
cd obj-$(dpkg-architecture -qDEB_HOST_MULTIARCH)
make -j${NUM_JOBS} abi-dump-base


