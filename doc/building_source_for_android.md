Building the source for Android {#building_source_for_android}
===============================

There are a few ways to compile for a target arm device. Only armhf (not armel)
is supported at this time.

Native Compile
--------------

If you have a target device, you should be able to compile directly on the ARM
device. This will be probably be slow, given the relative desktop/embedded CPU
speeds these days.

Emulated Compile
----------------

This method uses QEMU ARM emulation and an armhf chroot on a amd64/ia32 system
to produce armhf binaries. This is useful in packaging, but is still slower
than a cross compile.

There are many ways to do this, depending on your workflow.

You can set up a qemu pbuilder armhf setup to do compile with. This is typically
useful for packaging when you only have a desktop computer.

Alternatively, you can set up an armhf chroot on your device. Typically this
involves getting an armhf base image, and installing qemu emulation. This guide
gives an overview of how to get emulation in the armhf chroot:
http://wiki.debian.org/QemuUserEmulation

Native Compile or Emulated Compile Instructions
-----------------------------------------------

From within the armhf system:

1. Set up Dependencies

       $ apt-get install devscripts equivs cmake
       $ mk-build-deps --install --tool "apt-get -y" --build-dep debian/control

2. Build

       $ bzr branch lp:mir
       $ mkdir mir/build; cd mir/build
       $ cmake -DBoost_COMPILER=-gcc -DMIR_ENABLE_DEATH_TESTS=NO -DMIR_PLATFORM=android -DMIR_DISABLE_INPUT=yes ..

Cross Compile
-------------

This method uses a cross compiler (e.g., the `g++-4.7-arm-linux-gnueabihf`
ubuntu package) to produce armhf code. This is typically the quickest way to
compile and run code, and is well suited for a development workflow.

1. Be sure that the cross compiler that you are using matches the target
   environment. (eg, make sure you're using the quantal toolchain if you're
   targeting a quantal phablet image)

2. Set up a chroot with the mir dependencies installed. At the moment, you
   can look at the script and instructions in lp:mir as an
   example of how to set up a partial chroot you can build mir against.

3. There are a few ways to do this, but here is an example of how to build mir for android

        $ bzr branch lp:mir
        $ mkdir mir/build; cd mir/build
        $ MIR_NDK_PATH=/path/to/depenendcies/chroot cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/LinuxCrossCompile.cmake -DBoost_COMPILER=-gcc -DMIR_ENABLE_DEATH_TESTS=NO -DMIR_PLATFORM=android -DMIR_DISABLE_INPUT=yes ..
        $ make

N.B. The `cross-compile-android.sh` script in mir's top level directory
provides a scripting example of how to cross compile.
The 'setup-partial-armhf-chroot.sh' will attempt to download all the arm
dependencies you need. You have to have your APT sources.list files pointed at
arm repositories.
