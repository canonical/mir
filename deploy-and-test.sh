#!/bin/sh
#
# Warning: This script is very slow.
#
# If you need to run tests repeatedly, you'd be better off setting your
# device up this way:
#
# One time setup:
#   adb push ~/.ssh/id_rsa.pub /home/phablet/.ssh/authorized_keys
#   adb shell
#      chmod -R go-w ~/.ssh
#      chown phablet ~/.ssh
#      mkdir testrundir
#      sudo start ssh
#      sudo mount -o remount,rw /
#      sudo apt-get install umockdev
#
# Push and test binaries much faster than adb shell:
#   cd build-android-arm
#   rsync -avH lib bin phablet@${phone_ip}:testrundir/
#   adb shell  (or ssh phablet@${phone_ip})
#      cd testrundir
#      umockdev-run bin/mir_unit-tests
#      ...
# 

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

rundir=/home/phablet/mir/testing
adb push build-android-arm/bin $rundir/bin
adb push build-android-arm/lib $rundir/lib
adb push build-android-arm/usr $rundir/usr

libpath=$rundir/usr/lib/arm-linux-gnueabihf/:$rundir/lib
run_with_env="LD_LIBRARY_PATH=$libpath $rundir/usr/bin/umockdev-run $rundir/bin/"

adb shell "${run_with_env}mir_unit_tests"
adb shell "${run_with_env}mir_integration_tests"
adb shell "${run_with_env}mir_acceptance_tests"
