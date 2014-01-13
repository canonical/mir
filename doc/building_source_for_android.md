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

-  Set up Dependencies

       $ apt-get install devscripts equivs cmake
       $ mk-build-deps --install --tool "apt-get -y" --build-dep debian/control

-  Build

       $ bzr branch lp:mir
       $ mkdir mir/build; cd mir/build
       $ cmake -DBoost_COMPILER=-gcc -DMIR_PLATFORM=android ..

Cross Compile
-------------

This method uses a cross compiler (e.g., the `g++-arm-linux-gnueabihf`
ubuntu package) to produce armhf code. This is typically the quickest way to
compile and run code, and is well suited for a development workflow.

Initial setup of a desktop machine for cross-compiling to armhf is simple:

    $ sudo apt-get install g++-arm-linux-gnueabihf/trusty debootstrap
    $ sudo sh -c 'echo "deb [arch=armhf] http://ports.ubuntu.com/ubuntu-ports/ trusty main restricted universe multiverse" > /etc/apt/sources.list.d/armhf-xcompile.list'
    $ sudo apt-get update

Now to test that everything is working you can try downloading a package like
this:

    $ apt-get download gcc:armhf

Once you're able to download armhf packages from the repository, the 
cross-compile-chroot.sh script provides an example of how to build Mir for
armhf:

    $ ./cross-compile-chroot.sh
    $ ls -l build-android-arm/*  # binaries to copy to your device

To speed up the process for future branches you may wish to cache the files
downloaded by setting environment variable MIR_NDK_PATH to point to a directory
that cross-compile-chroot.sh should reuse each time.
