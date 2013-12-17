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

-  Be sure that the cross compiler that you are using matches the target
   environment. (eg, make sure you're using the trusty toolchain if you're
   targeting a trusty phablet image) You can specify the toolchain version
   thusly:

        $ apt-get install g++-arm-linux-gnueabihf/trusty

-  Get access to armhf packages via apt-get. On an amd64/ia32 system, you can
   do this by adding a file like the one below to /etc/apt/sources.list.d/

        #example sources.list with armhf dependencies
        deb [arch=armhf] http://ports.ubuntu.com/ubuntu-ports/ trusty main restricted universe multiverse
    
    Then you should run:

        $ apt-get update

    To test, try downloading a package like this:

        $ apt-get download my-package:armhf

-  Once you're able to download armhf packages from the repository, the 
   cross-compile-chroot.sh script provides an example of how to download
   a partial chroot with the mir dependencies, and compile the source for
   android targets.

   The script sets up a partial chroot via tools/setup-partial-armhf-chroot.sh
   and then runs build commands similar to this:

        $ mkdir mir/build; cd mir/build
        $ MIR_NDK_PATH=/path/to/depenendcies/chroot cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/LinuxCrossCompile.cmake -DBoost_COMPILER=-gcc -DMIR_PLATFORM=android ..
        $ make
