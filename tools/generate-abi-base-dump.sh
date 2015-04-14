#!/bin/sh

NUM_JOBS=$(( $(grep -c ^processor /proc/cpuinfo) + 1 ))

apt-get source mir
MIR_RELEASE_DIR=$(find -maxdepth 1 -name mir-\* -type d)
ln -sf ${MIR_RELEASE_DIR}/ mir-prev-release
cd ${MIR_RELEASE_DIR}
debian/rules override_dh_auto_configure
cd obj-$(dpkg-architecture -qDEB_HOST_MULTIARCH)
make -j${NUM_JOBS} abi-dump-base


