# Script to help in cross compiling for Android.
# Assumes that a standalone ndk is available, that
# contains all of mir's dependencies in its subfolder
# sysroot.
# Please adjust ANDROID_STANDALONE_TOOLCHAIN to your own setup.

cwd=`pwd`
mkdir $1
cd $1
export ANDROID_STANDALONE_TOOLCHAIN=$MIR_ANDROID_NDK_DIR
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/android.toolchain.cmake \
-DCMAKE_VERBOSE_MAKEFILE=ON \
-DBoost_COMPILER=-gcc \
-DBOOST_LIBRARYDIR=/tmp/ndk-standalone-9/sysroot/usr/lib \
-DMIR_ENABLE_DEATH_TESTS=NO \
-DMIR_INPUT_ENABLE_EVEMU=NO \
-Dgtest_disable_pthreads=YES \
$cwd

make 

$MIR_ANDROID_SDK_DIR/platform-tools/adb push ../bin/acceptance-tests /data/user
$MIR_ANDROID_SDK_DIR/platform-tools/adb shell 'cd /data/user && ./acceptance-tests'

$MIR_ANDROID_SDK_DIR/platform-tools/adb push ../bin/integration-tests /data/user
$MIR_ANDROID_SDK_DIR/platform-tools/adb shell 'cd /data/user && ./integration-tests'

$MIR_ANDROID_SDK_DIR/platform-tools/adb push ../bin/unit-tests /data/user
$MIR_ANDROID_SDK_DIR/platform-tools/adb shell 'cd /data/user && ./unit-tests'

