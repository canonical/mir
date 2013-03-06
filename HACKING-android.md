Mir hacking guide for Android/ARM devices
=========================================

Getting mir
-----------

If you're reading this file then you've probably solved this one. ;)

However, for completeness mir is a project on LaunchPad (https://launchpad.net/mir)
to grab a copy use the command:

    $ bzr branch lp:mir

Setting up your device
----------------------
Mir is designed to be ran in a gnu/Linux environment on an arm device with android gpu driver support. There are many ways to set up a device, but the easiest way is to start with the Ubuntu Touch project images on a supported device: https://wiki.ubuntu.com/Touch/Install
We base our instructions for running the device assuming we are running the Ubuntu Touch image at this time.

Compiling mir for armhf
-----------------------
There are a few ways to compile for a target arm device. armel is not supported at this time.

1) Native Compile
If you have a target device, you should be able to compile directly on the ARM device. This will be probably be slow, given the relative desktop/embedded cpu speeds these days.

2) Emulated Compile
This method uses qemu ARM emulation and an armhf chroot on a amd64/ia32 system to produce armhf binaries. This is useful in packaging, but is still slower than a cross compile.
3) Cross Compile
This method uses a cross compiler (eg, ubuntu package "g++-4.7-arm-linux-gnueabihf") to produce armhf code. This typically the quickest way to compile and run code, and is well suited for a development workflow. 

Native Compile
--------------

 
Emulated Compile
----------------
There are many ways to do this, depending on your workflow. 

You can set up a qemu pbuilder armhf setup to do compile with. This is typically useful for packaging when you only have a desktop computer. 

Alternatively, you can set up an armhf chroot on your device. Typically this involves getting an armhf base image, and installing qemu emulation. This guide gives an overview of how to get emulation in the armhf chroot: http://wiki.debian.org/QemuUserEmulation

Cross Compile
-------------
We have a set of scripts that makes this easier to set up. You can execute 'cross-compile-android.sh' to execute a cross compile based against a partial chroot

Uploading to Device
-------------------
There are a lot of ways to copy the files to the device so that they can run.

If you have debian packages, you put them on your device and install.

If you have built from source, you will have to upload the files to the device via your favorite method (eg, 'adb push' or using scp). If you simply push files to the device, you must ensure that you have the dependencies for mir installed on the device as well. (There is a bug that, when solved, should make this easier: https://bugs.launchpad.net/mir/+bug/1149907 )
 
Running on Device
-----------------
1) Ensure that no other program 'owns' the display. If another mir instance or surfaceflinger is active, mir will fail to display anything on the screen. Beware that Android can be particularly agressive about restarting surfaceflinger after it has died/been killed. 

2) Log into the Ubuntu chroot on the device as root. This can be done via ssh, or by 'adb shell', followed by 'ubuntu_chroot shell'

3) at this point (assuming you have installed mir), you can run mir via 'mir'. 
