# Script to help in cross compiling for Android.
# Assumes that a standalone ndk is available, that
# contains all of mir's dependencies in its subfolder
# sysroot.
# Please adjust ANDROID_STANDALONE_TOOLCHAIN to your own setup.

cwd=`pwd`
mkdir $1
cd $1
export MIR_NDK_PATH=$MIR_ANDROID_NDK_DIR

cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/AndroidCrossCompile.cmake \
-DBoost_COMPILER=-gcc \
-DMIR_ENABLE_DEATH_TESTS=NO \
-DMIR_INPUT_ENABLE_EVEMU=NO \
-DMIR_PLATFORM=android \
-Dgtest_disable_pthreads=YES \
$cwd

make 

$MIR_ANDROID_SDK_DIR/platform-tools/adb push bin/acceptance-tests /data/user
$MIR_ANDROID_SDK_DIR/platform-tools/adb shell 'cd /data/user && ./acceptance-tests'

$MIR_ANDROID_SDK_DIR/platform-tools/adb push bin/integration-tests /data/user
$MIR_ANDROID_SDK_DIR/platform-tools/adb shell 'cd /data/user && ./integration-tests'

$MIR_ANDROID_SDK_DIR/platform-tools/adb push bin/unit-tests /data/user
$MIR_ANDROID_SDK_DIR/platform-tools/adb shell 'cd /data/user && ./unit-tests'

