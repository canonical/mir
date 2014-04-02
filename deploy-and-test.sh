#!/bin/sh

if [ ! -d build-android-arm ] ; then
    echo "Built tree not found in $(pwd)/build-android-arm"
    exit 1
fi

# Unpack umockdev requirements
( cd build-android-arm ;
  apt-get download umockdev:armhf libumockdev0:armhf ;
  dpkg -x umockdev_*armhf*.deb . ;
  dpkg -x libumockdev0_*armhf*.deb .
)

adb push build-android-arm/bin /home/phablet/mir/bin
adb push build-android-arm/lib /home/phablet/mir/lib
adb push build-android-arm/usr /home/phablet/mir/usr

adb shell "LD_LIBRARY_PATH=/home/phablet/mir/usr/lib/arm-linux-gnueabihf/:/home/phablet/mir/lib PATH=$PATH:/home/phablet/mir/usr/bin bash -c \"cd /home/phablet/mir/usr/bin ; umockdev-run /home/phablet/mir/bin/mir_unit_tests\""
adb shell "LD_LIBRARY_PATH=/home/phablet/mir/usr/lib/arm-linux-gnueabihf/:/home/phablet/mir/lib PATH=$PATH:/home/phablet/mir/usr/bin bash -c \"cd /home/phablet/mir/usr/bin ; umockdev-run /home/phablet/mir/bin/mir_integration_tests\""
adb shell "LD_LIBRARY_PATH=/home/phablet/mir/usr/lib/arm-linux-gnueabihf/:/home/phablet/mir/lib PATH=$PATH:/home/phablet/mir/usr/bin bash -c \"cd /home/phablet/mir/usr/bin ; umockdev-run /home/phablet/mir/bin/mir_acceptance_tests\""
