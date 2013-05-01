Installing pre-built packages on an Android device {#installing_prebuilt_on_android}
==================================================

Supported Devices
-----------------

The following devices have been tested with Mir:

| Device             | Status |
| ------------------ | ------ |
| Galaxy Nexus (SGX) | tested |
| Nexus 7 (nvidia)   | tested - needs special hybris for nvidia atm |
| Nexus 4 (qualcomm) | tested |

Installing Mir
--------------

1. First you must install a phablet image on your device, by following the
   directions at https://wiki.ubuntu.com/Touch/Install on one of the supported
   devices. Once you have booted into the device with Ubuntu Touch ensure you
   then connect the device to the internet.

2. With your device connected via USB, type:

       $ adb devices

   just to ensure you are connected, then open the android shell:

       $ adb root
       $ adb shell

3. In the adb shell, stop SurfaceFlinger, Android's compositing engine that
   owns the display, and open the Ubuntu Touch shell:

       # stop
       # ubuntu_chroot shell

4. Add the Mir staging PPA to your device's sources list:

       # apt-get install software-properties-common (to get the add-apt-repository command)
       # add-apt-repository ppa:mir-team/staging
       # apt-get update

5. Install Mir:

       # apt-get install mir-demos
